/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfPartitioner.cpp                                                 */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#ifndef BUILD_EXE

extern "C"
{
#include <metis.h>
}

#include <iostream>
#include <sstream>
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfError.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfMap.hpp"
#include "XdmfPartitioner.hpp"
#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfUnstructuredGrid.hpp"

//
// local methods
//
namespace {

  shared_ptr<XdmfGraph>
  addSymmetricEntries(const shared_ptr<XdmfGraph> graph)
  {
    const shared_ptr<XdmfArray> rowPointer = graph->getRowPointer();
    const shared_ptr<XdmfArray> columnIndex = graph->getColumnIndex();
    const unsigned int numberRows = graph->getNumberRows();

    std::set<std::pair<unsigned int, unsigned int> > entrySet;
    for(unsigned int i=0; i<numberRows; ++i) {
      for(unsigned int j=rowPointer->getValue<unsigned int>(i);
          j<rowPointer->getValue<unsigned int>(i+1);
          ++j) {
        const unsigned int k = columnIndex->getValue<unsigned int>(j);
        entrySet.insert(std::make_pair(i, k));
        entrySet.insert(std::make_pair(k, i));
      }
    }

    shared_ptr<XdmfGraph> toReturn = XdmfGraph::New(numberRows);
    shared_ptr<XdmfArray> toReturnRowPointer = toReturn->getRowPointer();
    shared_ptr<XdmfArray> toReturnColumnIndex = toReturn->getColumnIndex();
    shared_ptr<XdmfArray> toReturnValues = toReturn->getValues();

    unsigned int currentRow = 1;
    for(std::set<std::pair<unsigned int, unsigned int> >::const_iterator
          iter = entrySet.begin();
        iter != entrySet.end();
        ++iter) {
      const std::pair<unsigned int, unsigned int> & entry = *iter;
      const unsigned int row = entry.first;
      const unsigned int column = entry.second;
      for(unsigned int j = currentRow; j<row; ++j) {
        toReturnRowPointer->insert<unsigned int>(j,
                                                 toReturnColumnIndex->getSize());
      }

      currentRow = row + 1;
      toReturnColumnIndex->pushBack<unsigned int>(column);
      toReturnValues->pushBack<double>(1.0);
      toReturnRowPointer->insert(row+1,
                                 toReturnColumnIndex->getSize());
    }

    return toReturn;

  }

}

shared_ptr<XdmfPartitioner>
XdmfPartitioner::New()
{
  shared_ptr<XdmfPartitioner> p(new XdmfPartitioner());
  return p;
}

XdmfPartitioner::XdmfPartitioner()
{
}

XdmfPartitioner::~XdmfPartitioner()
{
}

void
XdmfPartitioner::ignore(const shared_ptr<const XdmfSet> set)
{
  mIgnoredSets.insert(set);
}

void
XdmfPartitioner::partition(const shared_ptr<XdmfGraph> graphToPartition,
                           const unsigned int numberOfPartitions) const
{

  // Make sure row pointer and column index are non null
  if(!(graphToPartition->getRowPointer() &&
       graphToPartition->getColumnIndex()))
    XdmfError::message(XdmfError::FATAL,
                       "Current graph's row pointer or column index is null "
                       "in XdmfPartitioner::partition");

  shared_ptr<XdmfArray> rowPointer = graphToPartition->getRowPointer();
  shared_ptr<XdmfArray> columnIndex = graphToPartition->getColumnIndex();

  idx_t numberVertices = graphToPartition->getNumberNodes();
  idx_t numberConstraints = 1;

  bool releaseRowPointer = false;
  if(!rowPointer->isInitialized()) {
    rowPointer->read();
    releaseRowPointer = true;
  }
  bool releaseColumnIndex = false;
  if(!columnIndex->isInitialized()) {
    columnIndex->read();
    releaseColumnIndex = true;
  }

  shared_ptr<XdmfGraph> graph = graphToPartition;

  // Check whether graph is directed, if so we need to make it undirected
  // in order to partition with metis. From metis FAQ:
  //
  // The partitioning routines in METIS can only partition undirected graphs
  // (i.e., graphs in which for each edge (v,u) there is also an edge (u,v)).
  // For partitioning purposes, the directionality of an edge does not play
  // any role because if edge (u,v) is cut so will the edge (v,u). For this
  // reason, a directed graph can be easily partitioned by METIS by first
  // converting it into the corresponding undirected graph. That is, create
  // a graph for each directed edge (u,v) also contains the (v,u) edge as
  // well.
  const unsigned int numberRows = graphToPartition->getNumberRows();
  for(unsigned int i=0; i<numberRows; ++i) {
    for(unsigned int j=rowPointer->getValue<unsigned int>(i);
        j<rowPointer->getValue<unsigned int>(i+1);
        ++j) {
      const unsigned int k = columnIndex->getValue<unsigned int>(j);
      bool symmetric = false;
      for(unsigned int l=rowPointer->getValue<unsigned int>(k);
          l<rowPointer->getValue<unsigned int>(k+1);
          ++l) {
        const unsigned int m = columnIndex->getValue<unsigned int>(l);
        if(i == m) {
          symmetric = true;
          break;
        }
      }
      if(!symmetric) {
        graph = addSymmetricEntries(graphToPartition);
        if(releaseRowPointer) {
          rowPointer->release();
        }
        if(releaseColumnIndex) {
          columnIndex->release();
        }
        rowPointer = graph->getRowPointer();
        columnIndex = graph->getColumnIndex();
        break;
      }
    }
  }

  // copy into metis data structures
  idx_t * xadj = new idx_t[rowPointer->getSize()];
  rowPointer->getValues(0,
                        xadj,
                        rowPointer->getSize());
  if(releaseRowPointer) {
    rowPointer->release();
  }
  idx_t * adjncy = new idx_t[columnIndex->getSize()];
  columnIndex->getValues(0,
                         adjncy,
                         columnIndex->getSize());
  if(releaseColumnIndex) {
    columnIndex->release();
  }

  idx_t * vwgt = NULL; // equal vertex weights
  idx_t * vsize = NULL; // equal vertex sizes
  idx_t * adjwgt = NULL; // equal edge weights
  idx_t numParts = numberOfPartitions;
  real_t * tpwgts = NULL; // equal constraints and partition weights
  real_t * ubvec = NULL; // default load imbalance tolerance
  idx_t * options = NULL; // default options
  idx_t objval;
  idx_t * part = new idx_t[numberVertices];

  METIS_PartGraphRecursive(&numberVertices,
                           &numberConstraints,
                           xadj,
                           adjncy,
                           vwgt,
                           vsize,
                           adjwgt,
                           &numParts,
                           tpwgts,
                           ubvec,
                           options,
                           &objval,
                           part);

  delete [] xadj;
  delete [] adjncy;

  graphToPartition->removeAttribute("Partition");

  shared_ptr<XdmfAttribute> attribute = XdmfAttribute::New();
  attribute->setName("Partition");
  attribute->setCenter(XdmfAttributeCenter::Node());
  attribute->setType(XdmfAttributeType::Scalar());
  attribute->insert(0,
                    part,
                    numberVertices);
  graphToPartition->insert(attribute);

  delete [] part;

  return;
}


shared_ptr<XdmfGridCollection>
XdmfPartitioner::partition(const shared_ptr<XdmfUnstructuredGrid> gridToPartition,
                           const unsigned int numberOfPartitions,
                           const MetisScheme metisScheme,
                           const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
{

  if(heavyDataWriter) {
    heavyDataWriter->openFile();
  }

  // Make sure geometry and topology are non null
  if(!(gridToPartition->getGeometry() && gridToPartition->getTopology()))
    XdmfError::message(XdmfError::FATAL,
                       "Current grid's geometry or topology is null in "
                       "XdmfPartitioner::partition");

  const shared_ptr<XdmfGeometry> geometry =
    gridToPartition->getGeometry();
  const shared_ptr<const XdmfGeometryType> geometryType =
    geometry->getType();
  const shared_ptr<XdmfTopology> topology =
    gridToPartition->getTopology();
  const shared_ptr<const XdmfTopologyType> topologyType =
    topology->getType();

  const unsigned int nodesPerElement = topologyType->getNodesPerElement();

  bool releaseTopology = false;
  if(!topology->isInitialized()) {
    topology->read();
    releaseTopology = true;
  }

  idx_t numElements = topology->getNumberElements();
  idx_t numNodes = geometry->getNumberPoints();

  // allocate metisConnectivity arrays
  idx_t * metisConnectivityEptr = new idx_t[numElements + 1];
  idx_t * metisConnectivityEind = new idx_t[nodesPerElement * numElements];

  metisConnectivityEptr[0] = 0;

  unsigned int metisConnectivityEptrValue = 0;
  unsigned int connectivityOffset = 0;
  idx_t * metisConnectivityPtr = metisConnectivityEind;
  for(int i=0; i<numElements; ++i) {
    metisConnectivityEptrValue += nodesPerElement;
    metisConnectivityEptr[i + 1] = metisConnectivityEptrValue;
    topology->getValues(connectivityOffset,
                        metisConnectivityPtr,
                        nodesPerElement);
    connectivityOffset += topologyType->getNodesPerElement();
    metisConnectivityPtr += nodesPerElement;
  }

  idx_t * vwgt = NULL; // equal weight
  idx_t * vsize = NULL; // equal size
  idx_t ncommon = 1; // FIXME
  idx_t nparts = numberOfPartitions;
  real_t * tpwgts = NULL;
  idx_t * options = NULL;
  idx_t objval;

  idx_t * elementsPartition = new idx_t[numElements];
  idx_t * nodesPartition = new idx_t[numNodes];

  if(metisScheme == DUAL_GRAPH) {
    METIS_PartMeshDual(&numElements,
                       &numNodes,
                       metisConnectivityEptr,
                       metisConnectivityEind,
                       vwgt,
                       vsize,
                       &ncommon,
                       &nparts,
                       tpwgts,
                       options,
                       &objval,
                       elementsPartition,
                       nodesPartition);
  }
  else if(metisScheme == NODAL_GRAPH) {
    METIS_PartMeshNodal(&numElements,
                        &numNodes,
                        metisConnectivityEptr,
                        metisConnectivityEind,
                        vwgt,
                        vsize,
                        &nparts,
                        tpwgts,
                        options,
                        &objval,
                        elementsPartition,
                        nodesPartition);
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Invalid metis partitioning scheme selected in "
                       "XdmfPartitioner::partition");

  }

  delete [] metisConnectivityEptr;
  delete [] metisConnectivityEind;
  delete [] nodesPartition;

  // For each partition, map global to local node id
  std::vector<std::map<unsigned int, unsigned int> > globalToLocalNodeIdMap;
  // For each partition, list global element id.
  std::vector<std::vector<unsigned int> > globalElementIds;
  for(unsigned int i=0; i<numberOfPartitions; ++i) {
    std::map<unsigned int, unsigned int> nodeMap;
    globalToLocalNodeIdMap.push_back(nodeMap);
    std::vector<unsigned int> elementIds;
    globalElementIds.push_back(elementIds);
  }

  // Fill in globalNodeId for each partition
  unsigned int totalIndex = 0;
  for (int i=0; i<numElements; ++i) {
    unsigned int partitionId = elementsPartition[i];
    for (unsigned int j=0; j<topologyType->getNodesPerElement(); ++j) {
      unsigned int globalNodeId = topology->getValue<unsigned int>(totalIndex);
      if (globalToLocalNodeIdMap[partitionId].count(globalNodeId) == 0) {
        // Have not seen this node, need to add to map
        unsigned int size = globalToLocalNodeIdMap[partitionId].size();
        globalToLocalNodeIdMap[partitionId][globalNodeId] = size;
      }
      totalIndex++;
    }
    globalElementIds[partitionId].push_back(i);
  }
  delete [] elementsPartition;

  bool generateGlobalNodeIds = !gridToPartition->getAttribute("GlobalNodeId");

  shared_ptr<XdmfGridCollection> partitionedGrids =
    XdmfGridCollection::New();
  partitionedGrids->setType(XdmfGridCollectionType::Spatial());

  bool releaseGeometry = false;
  if(!geometry->isInitialized()) {
    geometry->read();
    releaseGeometry = true;
  }

  // Split geometry and topology into proper partitions
  for(unsigned int i=0; i<numberOfPartitions; ++i) {
    std::map<unsigned int, unsigned int> & currNodeMap =
      globalToLocalNodeIdMap[i];
    std::vector<unsigned int> & currElemIds = globalElementIds[i];

    if(currElemIds.size() > 0) {
      std::stringstream name;
      name << gridToPartition->getName() << "_" << i;

      const shared_ptr<XdmfUnstructuredGrid> partitioned =
        XdmfUnstructuredGrid::New();
      partitioned->setName(name.str());
      partitionedGrids->insert(partitioned);

      // Fill in geometry for this partition
      partitioned->getGeometry()->setType(geometryType);
      unsigned int numDimensions = geometryType->getDimensions();
      partitioned->getGeometry()->initialize(geometry->getArrayType(),
                                             currNodeMap.size() * numDimensions);

      for(std::map<unsigned int, unsigned int>::const_iterator iter =
            currNodeMap.begin();
          iter != currNodeMap.end();
          ++iter) {
        partitioned->getGeometry()->insert(iter->second * numDimensions,
                                           geometry,
                                           iter->first * numDimensions,
                                           numDimensions);
      }

      if(heavyDataWriter) {
        partitioned->getGeometry()->accept(heavyDataWriter);
        partitioned->getGeometry()->release();
      }

      // Fill in topology for this partition
      partitioned->getTopology()->setType(topologyType);
      partitioned->getTopology()->initialize(topology->getArrayType(),
                                             currElemIds.size() * topologyType->getNodesPerElement());
      unsigned int index = 0;
      for(std::vector<unsigned int>::const_iterator iter = currElemIds.begin();
          iter != currElemIds.end();
          ++iter) {
        // Translate these global node ids to local node ids
        for(unsigned int j=0; j<topologyType->getNodesPerElement(); ++j) {
          unsigned int globalNodeId =
            currNodeMap[topology->getValue<unsigned int>(*iter * topologyType->getNodesPerElement() + j)];
          partitioned->getTopology()->insert(index, &globalNodeId, 1);
          index++;
        }
      }

      if(heavyDataWriter) {
        partitioned->getTopology()->accept(heavyDataWriter);
        partitioned->getTopology()->release();
      }

      if (generateGlobalNodeIds) {
        shared_ptr<XdmfAttribute> globalNodeIds = XdmfAttribute::New();
        globalNodeIds->setName("GlobalNodeId");
        globalNodeIds->setType(XdmfAttributeType::GlobalId());
        globalNodeIds->setCenter(XdmfAttributeCenter::Node());
        globalNodeIds->initialize<unsigned int>(currNodeMap.size());
        for(std::map<unsigned int, unsigned int>::const_iterator iter =
              currNodeMap.begin();
            iter != currNodeMap.end();
            ++iter) {
          globalNodeIds->insert(iter->second, &iter->first, 1);
        }
        partitioned->insert(globalNodeIds);

        if(heavyDataWriter) {
          globalNodeIds->accept(heavyDataWriter);
          globalNodeIds->release();
        }
      }
    }
  }

  if(releaseGeometry) {
    gridToPartition->getGeometry()->release();
  }
  if(releaseTopology) {
    gridToPartition->getTopology()->release();
  }

  // Split attributes into proper partitions
  for(unsigned int i=0; i<gridToPartition->getNumberAttributes(); ++i) {

    const shared_ptr<XdmfAttribute> currAttribute =
      gridToPartition->getAttribute(i);
    bool releaseAttribute = false;
    if(!currAttribute->isInitialized()) {
      currAttribute->read();
      releaseAttribute = true;
    }
    unsigned int partitionId = 0;
    for(unsigned int j=0; j<numberOfPartitions; ++j) {
      std::map<unsigned int, unsigned int> & currNodeMap =
        globalToLocalNodeIdMap[j];
      std::vector<unsigned int> & currElemIds = globalElementIds[j];
      if(currElemIds.size() > 0) {
        const shared_ptr<XdmfUnstructuredGrid> partitioned =
          partitionedGrids->getUnstructuredGrid(partitionId);
        partitionId++;
        shared_ptr<XdmfAttribute> createdAttribute =
          shared_ptr<XdmfAttribute>();
        if(currAttribute->getCenter() == XdmfAttributeCenter::Grid()) {
          // Insert into each partition
          createdAttribute = currAttribute;
        }
        else if(currAttribute->getCenter() == XdmfAttributeCenter::Cell()) {
          createdAttribute = XdmfAttribute::New();
          createdAttribute->setName(currAttribute->getName());
          createdAttribute->setCenter(currAttribute->getCenter());
          createdAttribute->setType(currAttribute->getType());
          unsigned int index = 0;
          const unsigned int numberComponents =
            currAttribute->getSize() / topology->getNumberElements();
          createdAttribute->initialize(currAttribute->getArrayType(),
                                       currElemIds.size() * numberComponents);
          for(std::vector<unsigned int>::const_iterator iter =
                currElemIds.begin();
              iter != currElemIds.end();
              ++iter) {
            createdAttribute->insert(index,
                                     currAttribute,
                                     *iter * numberComponents,
                                     numberComponents);
            index += numberComponents;
          }
        }
        else if(currAttribute->getCenter() == XdmfAttributeCenter::Node()) {
          createdAttribute = XdmfAttribute::New();
          createdAttribute->setName(currAttribute->getName());
          createdAttribute->setCenter(currAttribute->getCenter());
          createdAttribute->setType(currAttribute->getType());
          createdAttribute->initialize(currAttribute->getArrayType(),
                                       currNodeMap.size());
          const unsigned int numberComponents =
            currAttribute->getSize() / geometry->getNumberPoints();
          for(std::map<unsigned int, unsigned int>::const_iterator iter =
                currNodeMap.begin();
              iter != currNodeMap.end();
              ++iter) {
            createdAttribute->insert(iter->second * numberComponents,
                                     currAttribute,
                                     iter->first * numberComponents,
                                     numberComponents);
          }
        }
        if(createdAttribute) {
          partitioned->insert(createdAttribute);
          if(heavyDataWriter) {
            if(!createdAttribute->isInitialized()) {
              createdAttribute->read();
            }
            createdAttribute->accept(heavyDataWriter);
            createdAttribute->release();
          }
        }
      }
    }
    if(releaseAttribute) {
      currAttribute->release();
    }
  }

  // Split sets into proper partitions
  for(unsigned int i=0; i<gridToPartition->getNumberSets(); ++i) {
    shared_ptr<XdmfSet> currSet = gridToPartition->getSet(i);
    if(mIgnoredSets.find(currSet) == mIgnoredSets.end()) {
      bool releaseSet = false;
      if(!currSet->isInitialized()) {
        currSet->read();
        releaseSet = true;
      }
      unsigned int partitionId = 0;
      for(unsigned int j=0; j<numberOfPartitions; ++j) {
        const std::map<unsigned int, unsigned int> & currNodeMap =
          globalToLocalNodeIdMap[j];
        const std::vector<unsigned int> & currElemIds = globalElementIds[j];
        if(currElemIds.size() > 0) {
          shared_ptr<XdmfUnstructuredGrid> partitioned =
            partitionedGrids->getUnstructuredGrid(partitionId);
          partitionId++;

          shared_ptr<XdmfSet> partitionedSet = XdmfSet::New();
          std::vector<unsigned int> partitionedSetIndex;

          if(currSet->getType() == XdmfSetType::Cell()) {
            for(unsigned int k=0; k<currSet->getSize(); ++k) {
              std::vector<unsigned int>::const_iterator val =
                std::find(currElemIds.begin(),
                          currElemIds.end(),
                          currSet->getValue<unsigned int>(k));
              if(val != currElemIds.end()) {
                unsigned int valToPush = val - currElemIds.begin();
                partitionedSet->pushBack(valToPush);
                partitionedSetIndex.push_back(k);
              }
            }
          }
          else if(currSet->getType() == XdmfSetType::Node()) {
            for(unsigned int k=0; k<currSet->getSize(); ++k) {
              std::map<unsigned int, unsigned int>::const_iterator val =
                currNodeMap.find(currSet->getValue<unsigned int>(k));
              if(val != currNodeMap.end()) {
                partitionedSet->pushBack(val->second);
                partitionedSetIndex.push_back(k);
              }
            }
          }

          // Only insert set if contains values
          if(partitionedSet->getSize() > 0) {

            for(unsigned int k=0; k<currSet->getNumberAttributes(); ++k) {
              const shared_ptr<XdmfAttribute> currAttribute =
                currSet->getAttribute(k);
              bool releaseAttribute = false;
              if(!currAttribute->isInitialized()) {
                currAttribute->read();
                releaseAttribute = true;
              }
              if(currAttribute->getCenter() == XdmfAttributeCenter::Node() ||
                 currAttribute->getCenter() == XdmfAttributeCenter::Cell()) {
                const unsigned int numberComponents =
                  currAttribute->getSize() / currSet->getSize();

                const shared_ptr<XdmfAttribute> partitionedAttribute =
                  XdmfAttribute::New();
                partitionedAttribute->setCenter(currAttribute->getCenter());
                partitionedAttribute->setName(currAttribute->getName());
                partitionedAttribute->setType(currAttribute->getType());
                partitionedAttribute->initialize(currAttribute->getArrayType(),
                                                 partitionedSetIndex.size() * numberComponents);
                for(unsigned int l=0; l<partitionedSetIndex.size(); ++l) {
                  partitionedAttribute->insert(l * numberComponents,
                                               currAttribute,
                                               partitionedSetIndex[l] * numberComponents,
                                               numberComponents);
                }
                partitionedSet->insert(partitionedAttribute);
                if(heavyDataWriter) {
                  partitionedAttribute->accept(heavyDataWriter);
                  partitionedAttribute->release();
                }
              }
            }
            partitioned->insert(partitionedSet);
            partitionedSet->setName(currSet->getName());
            partitionedSet->setType(currSet->getType());
            if(heavyDataWriter) {
              partitionedSet->accept(heavyDataWriter);
              partitionedSet->release();
            }
          }
        }
      }
      if(releaseSet) {
        currSet->release();
      }
    }
  }

  // Add XdmfMap to map boundary nodes between partitions
  std::vector<shared_ptr<XdmfAttribute> > globalNodeIds;
  for(unsigned int i=0;
      i<partitionedGrids->getNumberUnstructuredGrids();
      ++i) {
    shared_ptr<XdmfAttribute> globalNodeId =
      partitionedGrids->getUnstructuredGrid(i)->getAttribute("GlobalNodeId");
    if(!globalNodeId->isInitialized()) {
      globalNodeId->read();
    }
    globalNodeIds.push_back(globalNodeId);
  }

  std::vector<shared_ptr<XdmfMap> > maps = XdmfMap::New(globalNodeIds);
  for(unsigned int i=0;
      i<partitionedGrids->getNumberUnstructuredGrids();
      ++i) {
    shared_ptr<XdmfMap> map = maps[i];
    map->setName("Subdomain Boundary");
    partitionedGrids->getUnstructuredGrid(i)->insert(map);
    if(heavyDataWriter) {
      globalNodeIds[i]->release();
      map->accept(heavyDataWriter);
      map->release();
    }
  }

  return partitionedGrids;
}

shared_ptr<XdmfUnstructuredGrid>
XdmfPartitioner::unpartition(const shared_ptr<XdmfGridCollection> gridToUnPartition) const
{

  const shared_ptr<XdmfUnstructuredGrid> returnValue =
    XdmfUnstructuredGrid::New();
  const shared_ptr<XdmfTopology> returnValueTopology =
    returnValue->getTopology();
  const shared_ptr<XdmfGeometry> returnValueGeometry =
    returnValue->getGeometry();

  const unsigned int numberUnstructuredGrids =
    gridToUnPartition->getNumberUnstructuredGrids();

  unsigned int elementOffset = 0;

  for(unsigned int i=0; i<numberUnstructuredGrids; ++i) {

    const shared_ptr<XdmfUnstructuredGrid> grid =
      gridToUnPartition->getUnstructuredGrid(i);

    const shared_ptr<XdmfAttribute> globalNodeIds =
      grid->getAttribute("GlobalNodeId");

    if(!globalNodeIds) {
      XdmfError::message(XdmfError::FATAL,
                         "Cannot find GlobalNodeId attribute in "
                         "XdmfPartitioner::unpartition");
    }

    bool releaseGlobalNodeIds = false;
    if(!globalNodeIds->isInitialized()) {
      globalNodeIds->read();
      releaseGlobalNodeIds = true;
    }

    // handle topology
    const shared_ptr<XdmfTopology> topology = grid->getTopology();

    if(i==0) {
      returnValueTopology->setType(topology->getType());
      returnValueTopology->initialize(topology->getArrayType());
    }

    returnValueTopology->reserve(returnValueTopology->getSize() +
                                 topology->getSize());

    bool releaseTopology = false;
    if(!topology->isInitialized()) {
      topology->read();
      releaseTopology = true;
    }

    for(unsigned int j=0; j<topology->getSize(); ++j) {
      const unsigned int localNodeId = topology->getValue<unsigned int>(j);
      const unsigned int globalNodeId =
        globalNodeIds->getValue<unsigned int>(localNodeId);
      returnValueTopology->pushBack(globalNodeId);
    }

    if(releaseTopology) {
      topology->release();
    }

    // handle geometry
    const shared_ptr<XdmfGeometry> geometry = grid->getGeometry();
    const shared_ptr<const XdmfGeometryType> geometryType =
      geometry->getType();
    const unsigned int geometryDimension = geometryType->getDimensions();

    if(i==0) {
      returnValueGeometry->setType(geometryType);
      returnValueGeometry->initialize(geometry->getArrayType());
    }

    bool releaseGeometry = false;
    if(!geometry->isInitialized()) {
      geometry->read();
      releaseGeometry = true;
    }

    for(unsigned int j=0; j<globalNodeIds->getSize(); ++j) {
      const unsigned int globalNodeId =
        globalNodeIds->getValue<unsigned int>(j);
      returnValueGeometry->insert(globalNodeId * geometryDimension,
                                  geometry,
                                  j * geometryDimension,
                                  geometryDimension);
    }

    if(releaseGeometry) {
      geometry->release();
    }

    // handle attributes
    for(unsigned int j=0; j<grid->getNumberAttributes(); ++j) {

      const shared_ptr<XdmfAttribute> attribute = grid->getAttribute(j);
      const shared_ptr<const XdmfAttributeCenter> attributeCenter =
        attribute->getCenter();

      bool releaseAttribute = false;
      if(!attribute->isInitialized()) {
        attribute->read();
        releaseAttribute = true;
      }

      shared_ptr<XdmfAttribute> returnValueAttribute;

      if(i==0) {
        returnValueAttribute = XdmfAttribute::New();
        returnValueAttribute->setName(attribute->getName());
        returnValueAttribute->setCenter(attributeCenter);
        returnValueAttribute->setType(attribute->getType());
        returnValueAttribute->initialize(attribute->getArrayType());
        returnValue->insert(returnValueAttribute);
      }
      else {
        returnValueAttribute = returnValue->getAttribute(attribute->getName());
      }


      if(attributeCenter == XdmfAttributeCenter::Grid()) {
        returnValueAttribute->insert(0,
                                     attribute,
                                     0,
                                     attribute->getSize());
      }
      else if(attributeCenter == XdmfAttributeCenter::Cell()) {
        returnValueAttribute->insert(returnValueAttribute->getSize(),
                                     attribute,
                                     0,
                                     attribute->getSize());
      }
      else if(attributeCenter == XdmfAttributeCenter::Node()) {

        const unsigned int numberComponents =
          attribute->getSize() / geometry->getNumberPoints();

        for(unsigned int k=0; k<globalNodeIds->getSize(); ++k) {
          const unsigned int globalNodeId =
            globalNodeIds->getValue<unsigned int>(k);
          returnValueAttribute->insert(globalNodeId * numberComponents,
                                       attribute,
                                       k * numberComponents,
                                       numberComponents);
        }

      }

      if(releaseAttribute) {
        attribute->release();
      }

    }

    // handle sets
    for(unsigned int j=0; j<grid->getNumberSets(); ++j) {

      const shared_ptr<XdmfSet> set = grid->getSet(j);
      const shared_ptr<const XdmfSetType> setType = set->getType();

      bool releaseSet = false;
      if(!set->isInitialized()) {
        set->read();
        releaseSet = true;
      }

      shared_ptr<XdmfSet> returnValueSet = returnValue->getSet(set->getName());
      if(!returnValueSet) {
        returnValueSet = XdmfSet::New();
        returnValueSet->setName(set->getName());
        returnValueSet->setType(setType);
        returnValue->insert(returnValueSet);
      }

      if(setType == XdmfSetType::Cell()) {
        for(unsigned int k=0; k<set->getSize(); ++k) {
          const unsigned int localCellId = set->getValue<unsigned int>(k);
          returnValueSet->pushBack(localCellId + elementOffset);
        }
      }
      else if(setType == XdmfSetType::Node()) {
        for(unsigned int k=0; k<set->getSize(); ++k){
          const unsigned int localNodeId = set->getValue<unsigned int>(k);
          const unsigned int globalNodeId =
            globalNodeIds->getValue<unsigned int>(localNodeId);
          returnValueSet->pushBack(globalNodeId);
        }
      }

      for(unsigned int k=0; k<set->getNumberAttributes(); ++k) {
        const shared_ptr<XdmfAttribute> attribute = set->getAttribute(k);
        const shared_ptr<const XdmfAttributeCenter> attributeCenter =
          attribute->getCenter();
        const shared_ptr<const XdmfAttributeType> attributeType =
          attribute->getType();

        shared_ptr<XdmfAttribute> returnValueAttribute =
          returnValueSet->getAttribute(attribute->getName());
        if(!returnValueAttribute) {
          returnValueAttribute = XdmfAttribute::New();
          returnValueAttribute->setName(attribute->getName());
          returnValueAttribute->setCenter(attributeCenter);
          returnValueAttribute->setType(attributeType);
          returnValueSet->insert(returnValueAttribute);
        }

        if(attributeCenter == XdmfAttributeCenter::Cell() ||
           attributeCenter == XdmfAttributeCenter::Node()) {
          returnValueAttribute->insert(returnValueAttribute->getSize(),
                                       attribute,
                                       0,
                                       attribute->getSize());
        }

      }

    }

    elementOffset += topology->getNumberElements();

    if(releaseGlobalNodeIds) {
      globalNodeIds->release();
    }

  }

  return returnValue;

}

#else

#include <cstdio>
#include <iostream>
#include <sstream>
#include "XdmfDomain.hpp"
#include "XdmfGraph.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfPartitioner.hpp"
#include "XdmfReader.hpp"
#include "XdmfWriter.hpp"

namespace {

  //
  // print usage
  //
  inline void
  printUsage(const char * programName)
  {

    std::cerr << "usage: " << programName << " "
              << "[-s metis_scheme] [-r] [-u]"
              << "<input file> <number of partitions> [output file]"
              << std::endl;
    std::cerr << "\t-s metis_scheme: 1 - Dual Graph" << std::endl;
    std::cerr << "\t-s metis_scheme: 2 - Node Graph" << std::endl;
    std::cerr << "\t-u unpartition file" << std::endl;

    //
    //
    //
    return;

  }

  //
  // process command line
  //
  void
  processCommandLine(std::string                  & inputFileName,
                     std::string                  & outputFileName,
                     unsigned int                 & numPartitions,
                     XdmfPartitioner::MetisScheme & metisScheme,
                     bool                         & unpartition,
                     int                            ac,
                     char                         * av[])
  {

    int c;
    bool errorFlag = false;

    while( (c=getopt(ac, av, "s:ur")) != -1 )
    switch(c){

    case 's': {
      const int value = std::atoi(optarg);
      if(value == 1) {
        metisScheme = XdmfPartitioner::DUAL_GRAPH;
      }
      else if(value == 2) {
        metisScheme = XdmfPartitioner::NODAL_GRAPH;
      }
      else {
        errorFlag = true;
      }
      break;
    }
    case 'u':
      unpartition = true;
      break;
    case '?':
      errorFlag = true;
      break;
    }

    if (optind >= ac)
      errorFlag = true;
    else {
      inputFileName = av[optind];
      ++optind;
    }

    if(!unpartition) {
      if (optind >= ac)
        errorFlag = true;
      else {
        numPartitions = atoi(av[optind]);
        ++optind;
      }
    }

    if (optind < ac) {
      outputFileName = av[optind];
      ++optind;
    }

    //
    // check errorFlag
    //
    if (errorFlag == true) {
      printUsage(av[0]);
      std::exit(EXIT_FAILURE);
    }

  }

}

/**
 * XdmfPartitioner is a command line utility for partitioning Xdmf grids.
 * The XdmfPartitioner uses the metis library to partition Triangular,
 * Quadrilateral, Tetrahedral, and Hexahedral XdmfGridUnstructureds.
 *
 * Usage:
 *     XdmfPartitioner <path-of-file-to-partition> <num-partitions>
 *                     (Optional: <path-to-output-file>)
 *
 */
int main(int argc, char* argv[])
{

  std::string inputFileName = "";
  std::string outputFileName = "";
  unsigned int numPartitions = 0;
  XdmfPartitioner::MetisScheme metisScheme = XdmfPartitioner::DUAL_GRAPH;
  bool unpartition = false;

  processCommandLine(inputFileName,
                     outputFileName,
                     numPartitions,
                     metisScheme,
                     unpartition,
                     argc,
                     argv);

  std::cout << inputFileName << std::endl;

  FILE * refFile = fopen(inputFileName.c_str(), "r");
  if (refFile) {
    // Success
    fclose(refFile);
  }
  else {
    std::cout << "Cannot open file: " << argv[1] << std::endl;
    return 1;
  }

  std::string meshName;
  if(outputFileName.compare("") == 0) {
    meshName = inputFileName;
  }
  else {
    meshName = outputFileName;
  }

  if(meshName.find_last_of("/\\") != std::string::npos) {
    meshName = meshName.substr(meshName.find_last_of("/\\") + 1,
                               meshName.length());
  }

  if (meshName.rfind(".") != std::string::npos) {
    meshName = meshName.substr(0, meshName.rfind("."));
  }

  if(outputFileName.compare("") == 0) {
    std::stringstream newMeshName;
    if(unpartition) {
      newMeshName << meshName << "_unpartitioned";
    }
    else {
      newMeshName << meshName << "_p" << numPartitions;
    }
    meshName = newMeshName.str();
  }

  shared_ptr<XdmfReader> reader = XdmfReader::New();
  shared_ptr<XdmfDomain> domain =
    shared_dynamic_cast<XdmfDomain>(reader->read(inputFileName));

  if(unpartition) {
    if(domain->getNumberGridCollections() == 0) {
      std::cout << "No grid collections to unpartition" << std::endl;
      return 1;
    }
  }
  else {
    if(domain->getNumberUnstructuredGrids() == 0 &&
       domain->getNumberGridCollections() == 0 &&
       domain->getNumberGraphs() == 0) {
      std::cout << "No grids or graphs to partition" << std::endl;
      return 1;
    }
  }

  std::stringstream heavyFileName;
  heavyFileName << meshName << ".h5";
  shared_ptr<XdmfHDF5Writer> heavyDataWriter =
    XdmfHDF5Writer::New(heavyFileName.str());

  shared_ptr<XdmfDomain> newDomain = XdmfDomain::New();

  shared_ptr<XdmfPartitioner> partitioner = XdmfPartitioner::New();

  if(unpartition) {
    shared_ptr<XdmfUnstructuredGrid> toWrite =
      partitioner->unpartition(domain->getGridCollection(0));
    newDomain->insert(toWrite);
  }
  else {
    if(domain->getNumberGraphs() == 0) {
      shared_ptr<XdmfUnstructuredGrid> gridToPartition;
      if(domain->getNumberUnstructuredGrids() == 0) {
        // repartition
        gridToPartition =
          partitioner->unpartition(domain->getGridCollection(0));
      }
      else {
        gridToPartition = domain->getUnstructuredGrid(0);
      }
      shared_ptr<XdmfGridCollection> toWrite =
        partitioner->partition(gridToPartition,
                               numPartitions,
                               metisScheme,
                               heavyDataWriter);
      newDomain->insert(toWrite);
    }
    else {
      shared_ptr<XdmfGraph> graphToPartition = domain->getGraph(0);
      partitioner->partition(graphToPartition,
                             numPartitions);
      newDomain->insert(graphToPartition);
    }
  }

  std::stringstream xmlFileName;
  xmlFileName << meshName << ".xmf";
  shared_ptr<XdmfWriter> writer = XdmfWriter::New(xmlFileName.str(),
                                                  heavyDataWriter);
  newDomain->accept(writer);

  std::cout << "Wrote: " << xmlFileName.str() << std::endl;
}

#endif // BUILD_EXE
