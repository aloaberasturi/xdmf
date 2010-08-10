/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*  Date : $Date$ */
/*  Version : $Revision$ */
/*                                                                 */
/*  Author:                                                        */
/*     Kenneth Leiter                                              */
/*     kenneth.leiter@arl.army.mil                                 */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2010 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/

#ifndef BUILD_EXE

extern "C"
{
	#include <metis.h>
}

#include <iostream>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfGrid.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfGridCollectionType.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfMap.hpp"
#include "XdmfPartitioner.hpp"
#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

XdmfPartitioner::XdmfPartitioner()
{
}

XdmfPartitioner::~XdmfPartitioner()
{
}

boost::shared_ptr<XdmfGridCollection> XdmfPartitioner::partition(const boost::shared_ptr<XdmfGrid> gridToPartition, const unsigned int numberOfPartitions,
		boost::shared_ptr<XdmfHDF5Writer> heavyDataWriter) const
{
	int metisElementType;
	int nodesPerElement;

	boost::shared_ptr<const XdmfTopologyType> topologyType = gridToPartition->getTopology()->getType();
	if(topologyType == XdmfTopologyType::Triangle() || topologyType == XdmfTopologyType::Triangle_6())
	{
		metisElementType = 1;
		nodesPerElement = 3;
	}
	else if(topologyType == XdmfTopologyType::Quadrilateral() || topologyType == XdmfTopologyType::Quadrilateral_8())
	{
		metisElementType = 4;
		nodesPerElement = 4;
	}
	else if(topologyType == XdmfTopologyType::Tetrahedron() || topologyType == XdmfTopologyType::Tetrahedron_10())
	{
		metisElementType = 2;
		nodesPerElement = 4;
	}
	else if(topologyType == XdmfTopologyType::Hexahedron() || topologyType == XdmfTopologyType::Hexahedron_20() ||
			topologyType == XdmfTopologyType::Hexahedron_24() || topologyType == XdmfTopologyType::Hexahedron_27() ||
			topologyType == XdmfTopologyType::Hexahedron_64())
	{
		metisElementType = 3;
		nodesPerElement = 8;
	}
	else
	{
		assert(false);
	}

	if(!gridToPartition->getTopology()->isInitialized())
	{
		gridToPartition->getTopology()->read();
	}

	int numElements = gridToPartition->getTopology()->getNumberElements();
	idxtype * metisConnectivity = new idxtype[nodesPerElement * numElements];
	for(unsigned int i=0; i<numElements; ++i)
	{
		gridToPartition->getTopology()->getValuesCopy(i*topologyType->getNodesPerElement(), &metisConnectivity[i*nodesPerElement], nodesPerElement);
	}

	int numNodes = gridToPartition->getGeometry()->getNumberPoints();

	// Need to remap connectivity for nonlinear elements so that metis handles it properly.
	std::map<idxtype, idxtype> xdmfIdToMetisId;
	if(nodesPerElement != topologyType->getNodesPerElement())
	{
		unsigned int index = 0;
		for (unsigned int i=0; i<numElements * nodesPerElement; ++i)
		{
			std::map<idxtype, idxtype>::const_iterator val = xdmfIdToMetisId.find(metisConnectivity[i]);
			if (val != xdmfIdToMetisId.end())
			{
				metisConnectivity[i] = val->second;
			}
			else
			{
				// Haven't seen this id before, map to index and set to new id
				xdmfIdToMetisId[metisConnectivity[i]] = index;
				metisConnectivity[i] = index;
				index++;
			}
		}
		numNodes = index;
	}

	int startIndex = 0;
	int numCutEdges = 0;

	idxtype * elementsPartition = new idxtype[numElements];
	idxtype * nodesPartition = new idxtype[numNodes];

	std::cout << "Entered METIS" << std::endl;
	METIS_PartMeshDual(&numElements, &numNodes, metisConnectivity, &metisElementType, &startIndex, (int*)&numberOfPartitions, &numCutEdges, elementsPartition, nodesPartition);
	//METIS_PartMeshNodal(&numElements, &numNodes, metisConnectivity, &metisElementType, &startIndex, &numPartitions, &numCutEdges, elementsPartition, nodesPartition);
	std::cout << "Exited METIS" << std::endl;

	delete [] metisConnectivity;
	delete [] nodesPartition;

	// For each partition, map global to local node id
	std::vector<std::map<unsigned int, unsigned int> > globalToLocalNodeIdMap;
	// For each partition, list global element id.
	std::vector<std::vector<unsigned int> > globalElementIds;
	for(unsigned int i=0; i<numberOfPartitions; ++i)
	{
		std::map<unsigned int, unsigned int> nodeMap;
		globalToLocalNodeIdMap.push_back(nodeMap);
		std::vector<unsigned int> elementIds;
		globalElementIds.push_back(elementIds);
	}

	// Fill in globalNodeId for each partition
	unsigned int totalIndex = 0;
	for (unsigned int i=0; i<numElements; ++i)
	{
		unsigned int partitionId = elementsPartition[i];
		for (unsigned int j=0; j<topologyType->getNodesPerElement(); ++j)
		{
			unsigned int globalNodeId = gridToPartition->getTopology()->getValueCopy<unsigned int>(totalIndex);
			if (globalToLocalNodeIdMap[partitionId].count(globalNodeId) == 0)
			{
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

	boost::shared_ptr<XdmfGridCollection> partitionedGrids = XdmfGridCollection::New();
	partitionedGrids->setType(XdmfGridCollectionType::Spatial());

	if(!gridToPartition->getGeometry()->isInitialized())
	{
		gridToPartition->getGeometry()->read();
	}

	// Split geometry and topology into proper partitions
	for(unsigned int i=0; i<numberOfPartitions; ++i)
	{
		std::map<unsigned int, unsigned int> & currNodeMap = globalToLocalNodeIdMap[i];
		std::vector<unsigned int> & currElemIds = globalElementIds[i];

		if(currElemIds.size() > 0)
		{
			std::stringstream name;
			name << gridToPartition->getName() << "_" << i;

			boost::shared_ptr<XdmfGrid> partitioned = XdmfGrid::New();
			partitioned->setName(name.str());
			partitionedGrids->insert(partitioned);

			// Fill in geometry for this partition
			partitioned->getGeometry()->setType(gridToPartition->getGeometry()->getType());
			boost::shared_ptr<XdmfArray> geometryVals = partitioned->getGeometry();
			unsigned int numDimensions = partitioned->getGeometry()->getType()->getDimensions();
			geometryVals->initialize(gridToPartition->getGeometry()->getArrayType());
			geometryVals->resize<unsigned int>(currNodeMap.size() * numDimensions);

			for(std::map<unsigned int, unsigned int>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
			{
				geometryVals->copyValues(iter->second * numDimensions, gridToPartition->getGeometry(), iter->first * numDimensions, numDimensions);
			}

			if(heavyDataWriter)
			{
				geometryVals->accept(heavyDataWriter);
				geometryVals->release();
			}

			// Fill in topology for this partition
			partitioned->getTopology()->setType(gridToPartition->getTopology()->getType());
			boost::shared_ptr<XdmfArray> topologyVals = partitioned->getTopology();
			topologyVals->initialize(gridToPartition->getTopology()->getArrayType());
			topologyVals->resize<unsigned int>(currElemIds.size() * topologyType->getNodesPerElement());

			unsigned int index = 0;
			for(std::vector<unsigned int>::const_iterator iter = currElemIds.begin(); iter != currElemIds.end(); ++iter)
			{
				// Translate these global node ids to local node ids
				for(unsigned int j=0; j<topologyType->getNodesPerElement(); ++j)
				{
					unsigned int globalNodeId = currNodeMap[gridToPartition->getTopology()->getValueCopy<unsigned int>(*iter * topologyType->getNodesPerElement() + j)];
					topologyVals->copyValues(index, &globalNodeId, 1);
					index++;
				}
			}

			if(heavyDataWriter)
			{
				topologyVals->accept(heavyDataWriter);
				topologyVals->release();
			}

			if (generateGlobalNodeIds)
			{
				boost::shared_ptr<XdmfAttribute> globalNodeIds = XdmfAttribute::New();
				globalNodeIds->setName("GlobalNodeId");
				globalNodeIds->setType(XdmfAttributeType::GlobalId());
				globalNodeIds->setCenter(XdmfAttributeCenter::Node());

				boost::shared_ptr<XdmfArray> globalNodeIdsVals = globalNodeIds;
				globalNodeIdsVals->resize<unsigned int>(currNodeMap.size());
				for (std::map<unsigned int, unsigned int>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
				{
					globalNodeIdsVals->copyValues(iter->second, &iter->first, 1);
				}
				partitioned->insert(globalNodeIds);

				if(heavyDataWriter)
				{
					globalNodeIdsVals->accept(heavyDataWriter);
					globalNodeIdsVals->release();
				}
			}
		}
	}

	gridToPartition->getGeometry()->release();
	gridToPartition->getTopology()->release();

	// Split attributes into proper partitions
	for(unsigned int i=0; i<gridToPartition->getNumberAttributes(); ++i)
	{
		boost::shared_ptr<XdmfAttribute> currAttribute = gridToPartition->getAttribute(i);
		if(!currAttribute->isInitialized())
		{
			currAttribute->read();
		}
		unsigned int partitionId = 0;
		for(unsigned int j=0; j<numberOfPartitions; ++j)
		{
			std::map<unsigned int, unsigned int> & currNodeMap = globalToLocalNodeIdMap[j];
			std::vector<unsigned int> & currElemIds = globalElementIds[j];
			if(currElemIds.size() > 0)
			{
				boost::shared_ptr<XdmfGrid> partitioned = partitionedGrids->getGrid(partitionId);
				partitionId++;
				boost::shared_ptr<XdmfAttribute> createdAttribute;
				if(currAttribute->getCenter() == XdmfAttributeCenter::Grid())
				{
					if(partitionId == 0)
					{
						createdAttribute = currAttribute;
					}
				}
				else if(currAttribute->getCenter() == XdmfAttributeCenter::Cell() ||
						currAttribute->getCenter() == XdmfAttributeCenter::Face() ||
						currAttribute->getCenter() == XdmfAttributeCenter::Edge())
				{
					createdAttribute = XdmfAttribute::New();
					createdAttribute->setName(currAttribute->getName());
					createdAttribute->setCenter(currAttribute->getCenter());
					createdAttribute->setType(currAttribute->getType());
					unsigned int index = 0;
					unsigned int numValsPerComponent = currAttribute->size() / gridToPartition->getTopology()->getNumberElements();
					createdAttribute->initialize(currAttribute->getArrayType());
					createdAttribute->resize<unsigned int>(currElemIds.size() * numValsPerComponent);
					for(std::vector<unsigned int>::const_iterator iter = currElemIds.begin(); iter != currElemIds.end(); ++iter)
					{
						createdAttribute->copyValues(index, currAttribute, *iter * numValsPerComponent, numValsPerComponent);
						index += numValsPerComponent;
					}
				}
				else if(currAttribute->getCenter() == XdmfAttributeCenter::Node())
				{
					createdAttribute = XdmfAttribute::New();
					createdAttribute->setName(currAttribute->getName());
					createdAttribute->setCenter(currAttribute->getCenter());
					createdAttribute->setType(currAttribute->getType());
					createdAttribute->initialize(currAttribute->getArrayType());
					createdAttribute->resize<unsigned int>(currNodeMap.size());
					for(std::map<unsigned int, unsigned int>::const_iterator iter = currNodeMap.begin(); iter != currNodeMap.end(); ++iter)
					{
						createdAttribute->copyValues(iter->second, currAttribute, iter->first, 1);
					}
				}
				if(createdAttribute != NULL)
				{
					partitioned->insert(createdAttribute);
					if(heavyDataWriter)
					{
						createdAttribute->accept(heavyDataWriter);
						createdAttribute->release();
					}
				}
			}
		}
		currAttribute->release();
	}

	// Split sets into proper partitions
	for(unsigned int i=0; i<gridToPartition->getNumberSets(); ++i)
	{
		boost::shared_ptr<XdmfSet> currSet = gridToPartition->getSet(i);
		if(currSet->size() == 0)
		{
			currSet->read();
		}
		unsigned int partitionId = 0;
		for(unsigned int j=0; j<numberOfPartitions; ++j)
		{
			std::map<unsigned int, unsigned int> & currNodeMap = globalToLocalNodeIdMap[j];
			std::vector<unsigned int> & currElemIds = globalElementIds[j];
			if(currElemIds.size() > 0)
			{
				boost::shared_ptr<XdmfGrid> partitioned = partitionedGrids->getGrid(partitionId);
				partitionId++;

				boost::shared_ptr<XdmfSet> partitionedSet = XdmfSet::New();

				if(currSet->getType() == XdmfSetType::Cell() || currSet->getType() == XdmfSetType::Face() || currSet->getType() == XdmfSetType::Edge())
				{
					for(XdmfSet::const_iterator iter = currSet->begin(); iter != currSet->end(); ++iter)
					{
						std::vector<unsigned int>::const_iterator val = std::find(currElemIds.begin(), currElemIds.end(), *iter);
						if(val != currElemIds.end())
						{
							unsigned int valToPush = val - currElemIds.begin();
							partitionedSet->insert(valToPush);
						}
					}
				}
				else if(currSet->getType() == XdmfSetType::Node())
				{
					for(XdmfSet::const_iterator iter = currSet->begin(); iter != currSet->end(); ++iter)
					{
						std::map<unsigned int, unsigned int>::const_iterator val = currNodeMap.find(*iter);
						if(val != currNodeMap.end())
						{
							partitionedSet->insert(val->second);
						}
					}
				}
				if(partitionedSet->size() > 0)
				{
					partitionedSet->setName(currSet->getName());
					partitionedSet->setType(currSet->getType());
					if(heavyDataWriter)
					{
						partitionedSet->accept(heavyDataWriter);
						partitionedSet->release();
					}
				}
			}
		}
		currSet->release();
	}

	// Add XdmfMap to map boundary nodes between partitions
	std::vector<boost::shared_ptr<XdmfAttribute> > globalNodeIds;
	for(unsigned int i=0; i<partitionedGrids->getNumberGrids(); ++i)
	{
		boost::shared_ptr<XdmfAttribute> globalNodeId = partitionedGrids->getGrid(i)->getAttribute("GlobalNodeId");
		globalNodeId->read();
		globalNodeIds.push_back(globalNodeId);
	}

	std::vector<boost::shared_ptr<XdmfMap> > maps = XdmfMap::New(globalNodeIds);
	for(unsigned int i=0; i<partitionedGrids->getNumberGrids(); ++i)
	{
		boost::shared_ptr<XdmfMap> map = maps[i];
		partitionedGrids->getGrid(i)->setMap(map);
		if(heavyDataWriter)
		{
			globalNodeIds[i]->release();
			map->accept(heavyDataWriter);
			map->release();
		}
	}

	return partitionedGrids;
}

#else

#include <iostream>
#include <sstream>
#include "XdmfDomain.hpp"
#include "XdmfGridCollection.hpp"
#include "XdmfHDF5Writer.hpp"
#include "XdmfPartitioner.hpp"
#include "XdmfReader.hpp"
#include "XdmfWriter.hpp"

/**
 * XdmfPartitioner is a command line utility for partitioning Xdmf grids.
 * The XdmfPartitioner uses the metis library to partition Triangular, Quadrilateral, Tetrahedral,
 * and Hexahedral XdmfGrids.
 *
 * Usage:
 *     XdmfPartitioner <path-of-file-to-partition> <num-partitions> (Optional: <path-to-output-file>)
 *
 */
int main(int argc, char* argv[])
{
	std::string usage = "Partitions an Xdmf grid using the metis library: \n \n Usage: \n \n   XdmfPartitioner <path-of-file-to-partition> <num-partitions> (Optional: <path-to-output-file>)";
	std::string meshName = "";

	if (argc < 3)
	{
		std::cout << usage << std::endl;
		return 1;
	}

	FILE * refFile = fopen(argv[1], "r");
	if (refFile)
	{
		// Success
		meshName = argv[1];
		fclose(refFile);
	}
	else
	{
		std::cout << "Cannot open file: " << argv[1] << std::endl;
		return 1;
	}

	unsigned int numPartitions = atoi(argv[2]);

	if (argc >= 4)
	{
		meshName = argv[3];
	}

	if(meshName.find_last_of("/\\") != std::string::npos)
	{
		meshName = meshName.substr(meshName.find_last_of("/\\")+1, meshName.length());
	}

	if (meshName.rfind(".") != std::string::npos)
	{
		meshName = meshName.substr(0, meshName.rfind("."));
	}

	if(argc < 4)
	{
		meshName = meshName + "-partitioned";
	}

	boost::shared_ptr<XdmfReader> reader = XdmfReader::New();
	boost::shared_ptr<XdmfDomain> domain = boost::shared_dynamic_cast<XdmfDomain>(reader->read(argv[1]));

	if(domain->getNumberGrids() <= 0)
	{
		std::cout << "No grids to partition" << std::endl;
		return 1;
	}

	std::stringstream heavyFileName;
	heavyFileName << meshName << ".h5";
	boost::shared_ptr<XdmfHDF5Writer> heavyDataWriter = XdmfHDF5Writer::New(heavyFileName.str());

	boost::shared_ptr<XdmfPartitioner> partitioner = XdmfPartitioner::New();
	boost::shared_ptr<XdmfGridCollection> toWrite = partitioner->partition(domain->getGrid(0), numPartitions, heavyDataWriter);

	boost::shared_ptr<XdmfDomain> newDomain = XdmfDomain::New();
	newDomain->insert(toWrite);

	std::stringstream xmlFileName;
	xmlFileName << meshName << ".xmf";
	boost::shared_ptr<XdmfWriter> writer = XdmfWriter::New(xmlFileName.str(), heavyDataWriter);
	newDomain->accept(writer);

	std::cout << "Wrote: " << xmlFileName.str() << std::endl;
}

#endif //BUILD_EXE
