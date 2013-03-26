/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopologyConverter.cpp                                           */
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

#include <cmath>
#include <map>
#include <iostream>
#include <vector>
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfSet.hpp"
#include "XdmfSetType.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyConverter.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "XdmfError.hpp"

//
// local methods
//
namespace {
  
  typedef std::pair<std::vector<unsigned int>, 
                    std::vector<unsigned int> > OldFaceToNewFaceMap ;
  typedef std::vector<std::vector<OldFaceToNewFaceMap> > FaceHash;

  void handleSetConversion(const shared_ptr<XdmfUnstructuredGrid> gridToConvert,
			   const shared_ptr<XdmfUnstructuredGrid> toReturn,
			   const std::vector<int> & oldIdToNewId,
			   const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) 
  {

    for(unsigned int i=0; i<gridToConvert->getNumberSets(); ++i) {
      const shared_ptr<XdmfSet> set = gridToConvert->getSet(i);
      const shared_ptr<const XdmfSetType> setType = set->getType();
      if(setType == XdmfSetType::Cell()) {
	toReturn->insert(set);
      }
      else if(setType == XdmfSetType::Node()) {
	bool releaseSet = false;
	if(!set->isInitialized()) {
	  set->read();
	  releaseSet = true;
	}
	shared_ptr<XdmfSet> toReturnSet = XdmfSet::New();
	toReturnSet->setName(set->getName());
	toReturnSet->setType(set->getType());
	toReturnSet->initialize(set->getArrayType(),
				set->getSize());
	  
	for(unsigned int i=0; i<set->getSize(); ++i) {
	  const unsigned int nodeId = set->getValue<unsigned int>(i);
          const int newNodeId = oldIdToNewId[nodeId];
	  toReturnSet->insert(i, newNodeId);
	}
	if(releaseSet) {
	  set->release();
	}
	  
	toReturn->insert(toReturnSet);
	  
	if(heavyDataWriter) {
	  toReturnSet->accept(heavyDataWriter);
	  toReturnSet->release();
	}
      }
    }
  }

  template <unsigned int ORDER>
  void remapTopology(shared_ptr<XdmfTopology> topology)
  {
    return;
  }

  template <>
  void remapTopology<2>(shared_ptr<XdmfTopology> topology)
  {
    const unsigned int numberElements = topology->getNumberElements();
    unsigned int oldElementIds[27];
    unsigned int newElementIds[27];
    unsigned int offset = 0;
    for(unsigned int i=0; i<numberElements; ++i) {
      topology->getValues(offset,
                          oldElementIds,
                          27);
      newElementIds[0] = oldElementIds[0];
      newElementIds[1] = oldElementIds[18];
      newElementIds[2] = oldElementIds[24];
      newElementIds[3] = oldElementIds[6];
      newElementIds[4] = oldElementIds[2];
      newElementIds[5] = oldElementIds[20];
      newElementIds[6] = oldElementIds[26];
      newElementIds[7] = oldElementIds[8];
      newElementIds[8] = oldElementIds[9];
      newElementIds[9] = oldElementIds[21];
      newElementIds[10] = oldElementIds[15];
      newElementIds[11] = oldElementIds[3];
      newElementIds[12] = oldElementIds[11];
      newElementIds[13] = oldElementIds[23];
      newElementIds[14] = oldElementIds[17];
      newElementIds[15] = oldElementIds[5];
      newElementIds[16] = oldElementIds[1];
      newElementIds[17] = oldElementIds[19];
      newElementIds[18] = oldElementIds[25];
      newElementIds[19] = oldElementIds[7];
      newElementIds[20] = oldElementIds[4];
      newElementIds[21] = oldElementIds[22];
      newElementIds[22] = oldElementIds[10];
      newElementIds[23] = oldElementIds[16];
      newElementIds[24] = oldElementIds[12];
      newElementIds[25] = oldElementIds[14];
      newElementIds[26] = oldElementIds[13];
      topology->insert(offset,
                       newElementIds,
                       27);
      offset += 27;
    }
  }

  // Classes that perform topology conversions. Converter is the root
  // base class.  Tessellator is a subclass of Converter that deals
  // with cases where the mesh only needs to be tessellated to carry
  // out the conversion (e.g. Hexahedron_64 to Hexahedron).

  class Converter {

  public:

    Converter()
    {
    }

    virtual ~Converter()
    {
    }

    virtual shared_ptr<XdmfUnstructuredGrid>
    convert(const shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const shared_ptr<const XdmfTopologyType> topologyType,
            const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const = 0;

  };

  class Tessellator : public Converter {

  public:

    virtual ~Tessellator()
    {
    }

    shared_ptr<XdmfUnstructuredGrid>
    convert(const shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const shared_ptr<const XdmfTopologyType> topologyType,
            const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
    {
      shared_ptr<XdmfUnstructuredGrid> toReturn =
        XdmfUnstructuredGrid::New();
      toReturn->setName(gridToConvert->getName());
      toReturn->setGeometry(gridToConvert->getGeometry());

      if(heavyDataWriter) {
        if(!toReturn->getGeometry()->isInitialized()) {
          toReturn->getGeometry()->read();
        }
        toReturn->getGeometry()->accept(heavyDataWriter);
        toReturn->getGeometry()->release();
      }

      bool releaseTopology;
      if(!gridToConvert->getTopology()->isInitialized()) {
        gridToConvert->getTopology()->read();
        releaseTopology = true;
      }

      this->tesselateTopology(gridToConvert->getTopology(),
                              toReturn->getTopology());

      if(releaseTopology) {
        gridToConvert->getTopology()->release();
      }

      if(heavyDataWriter) {
        toReturn->getTopology()->accept(heavyDataWriter);
        toReturn->getTopology()->release();
      }

      for(unsigned int i=0; i<gridToConvert->getNumberAttributes(); ++i) {
        shared_ptr<XdmfAttribute> currAttribute =
          gridToConvert->getAttribute(i);
        shared_ptr<XdmfAttribute> createdAttribute =
          shared_ptr<XdmfAttribute>();
        if(currAttribute->getCenter() == XdmfAttributeCenter::Node()) {
          createdAttribute = currAttribute;
        }
        else if(currAttribute->getCenter() == XdmfAttributeCenter::Cell()) {
          bool releaseAttribute = false;
          if(!currAttribute->isInitialized()) {
            currAttribute->read();
            releaseAttribute = true;
          }

          createdAttribute = XdmfAttribute::New();
          createdAttribute->setName(currAttribute->getName());
          createdAttribute->setType(currAttribute->getType());
          createdAttribute->setCenter(currAttribute->getCenter());
          createdAttribute->initialize(currAttribute->getArrayType(),
                                       currAttribute->getSize() * mNumTesselations);
          for(unsigned int j=0; j<currAttribute->getSize(); ++j) {
            createdAttribute->insert(j * mNumTesselations,
                                     currAttribute,
                                     j,
                                     mNumTesselations,
                                     1,
                                     0);
          }

          if(releaseAttribute) {
            currAttribute->release();
          }
        }
        if(createdAttribute) {
          toReturn->insert(createdAttribute);
          if(heavyDataWriter) {
            if(!createdAttribute->isInitialized()) {
              createdAttribute->read();
            }
            createdAttribute->accept(heavyDataWriter);
            createdAttribute->release();
          }
        }
      }
      return toReturn;
    }

    virtual void
    tesselateTopology(shared_ptr<XdmfTopology> topologyToConvert,
                      shared_ptr<XdmfTopology> topologyToReturn) const = 0;

  protected:

    Tessellator(const unsigned int numTesselations) :
      mNumTesselations(numTesselations)
    {
    }

    const unsigned int mNumTesselations;

  };

  template <unsigned int ORDER, bool ISSPECTRAL>
  class HexahedronToHighOrderHexahedron : public Converter {

  public:

    HexahedronToHighOrderHexahedron()
    {
    }

    virtual ~HexahedronToHighOrderHexahedron()
    {
    }

    int
    reorder(unsigned int & a,
            unsigned int & b,
            unsigned int & c,
            unsigned int & d) const
    {
      
      int tmp;
      
      // Reorder to get smallest id in a.
      if (b < a && b < c && b < d) {
        tmp = a;
        a = b;
        b = c;
        c = d;
        d = tmp;
        return 1;
      }
      else if (c < a && c < b && c < d) {
        tmp = a;
        a = c;
        c = tmp;
        tmp = b;
        b = d;
        d = tmp;
        return 2;
      }
      else if (d < a && d < b && d < c) {
        tmp = a;
        a = d;
        d = c;
        c = b;
        b = tmp;
        return 3;
      }
      
      return 0;

    }               

    void
    rotateQuad(unsigned int reorderVal,
               const std::vector<unsigned int> & face,
               std::vector<unsigned int> & newFace) const
    {
      
      switch (reorderVal) {
      case 0: {
        std::copy(face.begin(),
                  face.end(),
                  newFace.begin());
        return;
      }
      case 1: {
        unsigned int index = 0;
        for(unsigned int i=mNodesPerEdge; i>0; --i) {
          for(unsigned int j=i-1; j<mNodesPerFace; j+=mNodesPerEdge) {
            newFace[index++] = face[j];
          }
        }
        return;
      }
      case 2: {
        for(unsigned int i=0; i<mNodesPerFace; ++i) {
          newFace[i] = face[mNodesPerFace - 1 - i];
        }
        return;
      }
      case 3: {
        unsigned int index = 0;
        for(unsigned int i=mNodesPerFace-mNodesPerEdge; i<mNodesPerFace; ++i) {
          for(int j=i; j>=0; j-=mNodesPerEdge) {
            newFace[index++] = face[j];
          }
        }
        return;
      }
      }
    }

    void
    addFaceToHash(unsigned int a,
                  unsigned int b,
                  unsigned int c,
                  unsigned int d,
                  FaceHash & hash,
                  std::vector<unsigned int> & face) const
    {
      const unsigned int reorderVal = reorder(a, b, c, d);
      
      std::vector<unsigned int> newFace(face.size());
      rotateQuad(reorderVal,
                 face,
                 newFace);
      
      // Look for existing cell in the hash;
      FaceHash::value_type & currHash = hash[a];
      std::vector<unsigned int> currFace(3);
      currFace[0] = b;
      currFace[1] = c;
      currFace[2] = d;
      currHash.push_back(std::make_pair(currFace, newFace));
      
    }

    std::vector<unsigned int>
    getFace(unsigned int a,
            unsigned int b,
            unsigned int c,
            unsigned int d,
            FaceHash & hash) const
    {
      
      unsigned int reorderVal = reorder(a, b, c, d);   

      // need to rotate opposite of what we put in
      if(reorderVal == 1) {
        reorderVal = 3;
      }
      else if(reorderVal == 3) {
        reorderVal = 1;
      }
            
      // Look for existing cell in the hash;
      FaceHash::value_type & currHash = hash[a];
      for(FaceHash::value_type::iterator iter = currHash.begin(); 
          iter != currHash.end(); ++iter) {
        std::vector<unsigned int> & currFace = iter->first;
        // 3 because b + c + d
        if(currFace.size() == 3) {
          if(b == currFace[0] && d == currFace[2]) {
            
            const std::vector<unsigned int> & face = iter->second;
            std::vector<unsigned int> returnValue(face.size());
            rotateQuad(reorderVal,
                       face,
                       returnValue);
            currHash.erase(iter);
            return returnValue;
            
          }
        }
      }

      return std::vector<unsigned int>();
      
    }

    void
    calculateIntermediatePoint(double result[3],
                               const double point1[3],
                               const double point2[3],
                               int index) const
    {
      const double scalar = points[index];
      for (int i=0; i<3; ++i) {
        result[i] = point1[i]+scalar*(point2[i]-point1[i]);
      }
    }

    shared_ptr<XdmfUnstructuredGrid>
    convert(const shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const shared_ptr<const XdmfTopologyType> topologyType,
            const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
    {

      shared_ptr<XdmfUnstructuredGrid> toReturn = XdmfUnstructuredGrid::New();
      toReturn->setName(gridToConvert->getName());

      shared_ptr<XdmfGeometry> geometry = gridToConvert->getGeometry();
      shared_ptr<XdmfGeometry> toReturnGeometry = toReturn->getGeometry();

      toReturnGeometry->setType(geometry->getType());
      toReturnGeometry->initialize(geometry->getArrayType());

      bool releaseGeometry = false;
      if(!geometry->isInitialized()) {
        geometry->read();
        releaseGeometry = true;
      }

      shared_ptr<XdmfTopology> topology = gridToConvert->getTopology();
      shared_ptr<XdmfTopology> toReturnTopology = toReturn->getTopology();

      toReturn->getTopology()->setType(topologyType);
      toReturnTopology->initialize(topology->getArrayType());
      toReturnTopology->reserve(topologyType->getNodesPerElement() *
                                topology->getNumberElements());

      bool releaseTopology = false;
      if(!topology->isInitialized()) {
        topology->read();
        releaseTopology = true;
      }

      // allocate storage for values used in loop
      unsigned int zeroIndex;
      unsigned int oneIndex;
      unsigned int twoIndex;
      unsigned int threeIndex;
      unsigned int fourIndex;
      unsigned int fiveIndex;
      unsigned int sixIndex;
      unsigned int sevenIndex;
      double elementCorner0[3];
      double elementCorner1[3];
      double elementCorner2[3];
      double elementCorner3[3];
      double elementCorner4[3];
      double elementCorner5[3];
      double elementCorner6[3];
      double elementCorner7[3];
      double planeCorner0[3];
      double planeCorner1[3];
      double planeCorner2[3];
      double planeCorner3[3];
      double lineEndPoint0[3];
      double lineEndPoint1[3];
      double (*newPoints)[3] = new double[mNumberPoints][3];

      unsigned int largestId = 0;
      for(unsigned int i=0; i<topology->getSize(); ++i) {
        const unsigned int val = topology->getValue<unsigned int>(i);
        largestId = std::max(val, largestId);
      }

      FaceHash hash(largestId + 1);
      std::vector<int> oldIdToNewId(largestId + 1, -1);

      unsigned int offset = 0;
      for(unsigned int elem = 0; elem<topology->getNumberElements(); ++elem) {

        // get indices of coner vertices of the element
        zeroIndex = topology->getValue<unsigned int>(offset++);
        oneIndex = topology->getValue<unsigned int>(offset++);
        twoIndex = topology->getValue<unsigned int>(offset++);
        threeIndex = topology->getValue<unsigned int>(offset++);
        fourIndex = topology->getValue<unsigned int>(offset++);
        fiveIndex = topology->getValue<unsigned int>(offset++);
        sixIndex = topology->getValue<unsigned int>(offset++);
        sevenIndex = topology->getValue<unsigned int>(offset++);       

        // get previously added faces
        std::vector<unsigned int> bottomFace = 
          getFace(zeroIndex, threeIndex, twoIndex, oneIndex, hash);
        std::vector<unsigned int> frontFace = 
          getFace(zeroIndex, oneIndex, fiveIndex, fourIndex, hash);
        std::vector<unsigned int> leftFace = 
          getFace(zeroIndex, fourIndex, sevenIndex, threeIndex, hash);
        std::vector<unsigned int> rightFace = 
          getFace(oneIndex, twoIndex, sixIndex, fiveIndex, hash);
        std::vector<unsigned int> backFace = 
          getFace(threeIndex, sevenIndex, sixIndex, twoIndex, hash);
        std::vector<unsigned int> topFace = 
          getFace(fourIndex, fiveIndex, sixIndex, sevenIndex, hash);
                                                   
        // get locations of corner vertices of the element
        geometry->getValues(zeroIndex * 3,
                            &(elementCorner0[0]),
                            3);
        geometry->getValues(oneIndex * 3,
                            &(elementCorner1[0]),
                            3);
        geometry->getValues(twoIndex * 3,
                            &(elementCorner2[0]),
                            3);
        geometry->getValues(threeIndex * 3,
                            &(elementCorner3[0]),
                            3);
        geometry->getValues(fourIndex * 3,
                            &(elementCorner4[0]),
                            3);
        geometry->getValues(fiveIndex * 3,
                            &(elementCorner5[0]),
                            3);
        geometry->getValues(sixIndex * 3,
                            &(elementCorner6[0]),
                            3);
        geometry->getValues(sevenIndex * 3,
                            &(elementCorner7[0]),
                            3);
        
        std::vector<int> newIds(mNumberPoints, -1);

        // set new ids if they have been found previously
        newIds[mZeroIndex] = oldIdToNewId[zeroIndex];
        newIds[mOneIndex] = oldIdToNewId[oneIndex];
        newIds[mTwoIndex] = oldIdToNewId[twoIndex];
        newIds[mThreeIndex] = oldIdToNewId[threeIndex];
        newIds[mFourIndex] = oldIdToNewId[fourIndex];
        newIds[mFiveIndex] = oldIdToNewId[fiveIndex];
        newIds[mSixIndex] = oldIdToNewId[sixIndex];
        newIds[mSevenIndex] = oldIdToNewId[sevenIndex];

        // loop over i, j, k directions of element isolation i, j, and
        // k planes
        int pointIndex = 0;
        for(unsigned int i=0; i<mNodesPerEdge; ++i){
          // calculate corners of i plane
          calculateIntermediatePoint(planeCorner0,
                                     elementCorner0,
                                     elementCorner1,
                                     i);
          calculateIntermediatePoint(planeCorner1,
                                     elementCorner4,
                                     elementCorner5,
                                     i);
          calculateIntermediatePoint(planeCorner2,
                                     elementCorner3,
                                     elementCorner2,
                                     i);
          calculateIntermediatePoint(planeCorner3,
                                     elementCorner7,
                                     elementCorner6,
                                     i);
          for(unsigned int j=0; j<mNodesPerEdge; ++j) {
            // calculate endpoints of j slice of i plane
            calculateIntermediatePoint(lineEndPoint0,
                                       planeCorner0,
                                       planeCorner2,
                                       j);
            calculateIntermediatePoint(lineEndPoint1,
                                       planeCorner1,
                                       planeCorner3,
                                       j);
            for(unsigned int k=0; k<mNodesPerEdge; ++k) {
               // calculate point to add to mesh
              calculateIntermediatePoint(newPoints[pointIndex++],
                                         lineEndPoint0,
                                         lineEndPoint1,
                                         k);
            }
          }
        }
        
        if(bottomFace.size() > 0) {
          unsigned int index = 0;
          for(unsigned int i=0; i<mNumberPoints; i+=mNodesPerEdge) {
            newIds[i] = bottomFace[index++];
          }
        }
        if(frontFace.size() > 0) {
          unsigned int index = 0;
          for(unsigned int i=0; i<mNodesPerEdge; ++i) {
            for(unsigned int j=i; j<mNumberPoints; j+=mNodesPerFace) {
              newIds[j] = frontFace[index++];
            }
          }
        }
        std::copy(leftFace.begin(),
                  leftFace.end(),
                  &(newIds[0]));
        if(rightFace.size() > 0) {
          unsigned int index = 0;
          for(unsigned int i=mNumberPoints - mNodesPerFace; 
              i<mNumberPoints - mNodesPerFace + mNodesPerEdge; ++i) {
            for(unsigned int j=i; j<mNumberPoints; j+=mNodesPerEdge) {
              newIds[j] = rightFace[index++];
            }
          }
        }
        if(backFace.size() > 0) {          
          unsigned int index = 0;
          for(unsigned int i=mNodesPerFace - mNodesPerEdge; 
              i<mNumberPoints; i+=mNodesPerFace) {
            for(unsigned int j=i; j<i+mNodesPerEdge; ++j) {
              newIds[j] = backFace[index++];
            }
          }
        }
        if(topFace.size() > 0) {
          unsigned int index = 0;
          for(unsigned int i=mNodesPerEdge-1; 
              i<mNodesPerFace; i+=mNodesPerEdge) {
            for(unsigned int j=i; j<mNumberPoints; j+=mNodesPerFace) {
              newIds[j] = topFace[index++];
            }
          }
        }
        
        // finally, add the points and ids to the new topology and geometry
        for(unsigned int i=0; i<mNumberPoints; ++i) {
          const int id = newIds[i];
          if(id == -1) {
            // need to add the new point
            const unsigned int newId = toReturnGeometry->getNumberPoints();
            newIds[i] = newId;
            toReturnGeometry->insert(newId * 3,
                                     &(newPoints[i][0]),
                                     3);
            toReturnTopology->pushBack(newId);
          }
          else {
            // point added previously, so just add the found id
            toReturnTopology->pushBack(id);
          }
        }
   
        // add all faces to hash (mirror to match face on another element)
        if(bottomFace.size() == 0) {
          bottomFace.resize(mNodesPerFace);
          unsigned int index = 0;
          for(unsigned int i=0; i<mNodesPerFace; i+=mNodesPerEdge) {
            for(unsigned int j=i; j<mNumberPoints; j+=mNodesPerFace) {
              bottomFace[index++] = newIds[j];
            }
          }
          addFaceToHash(zeroIndex, oneIndex, twoIndex, threeIndex, hash, 
                        bottomFace);
        }
        if(frontFace.size() == 0) {
          frontFace.resize(mNodesPerFace);
          unsigned int index = 0;
          for(unsigned int i=0; i<mNumberPoints; i+=mNodesPerFace) {
            for(unsigned int j=i; j<i+mNodesPerEdge; ++j) {
              frontFace[index++] = newIds[j];
            }
          }
          addFaceToHash(zeroIndex, fourIndex, fiveIndex, oneIndex, hash, 
                        frontFace);
        }
        if(leftFace.size() == 0) {
          leftFace.resize(mNodesPerFace);
          unsigned int index = 0;
          for(unsigned int i=0; i<mNodesPerEdge; ++i) {
            for(unsigned int j=i; j<mNodesPerFace; j+=mNodesPerEdge) {
              leftFace[index++] = newIds[j];
            }
          }
          addFaceToHash(zeroIndex, threeIndex, sevenIndex, fourIndex, hash, 
                        leftFace);
        } 
        if(rightFace.size() == 0) {
          rightFace.resize(mNodesPerFace);
          std::copy(&(newIds[mOneIndex]),
                    &(newIds[mNumberPoints]),
                    &(rightFace[0]));
          addFaceToHash(oneIndex, fiveIndex, sixIndex, twoIndex, hash, 
                        rightFace);
        }
        if(backFace.size() == 0) {
          backFace.resize(mNodesPerFace);
          unsigned int index = 0;
          for(unsigned int i=mNodesPerFace - mNodesPerEdge; 
              i<mNodesPerFace; ++i) {
            for(unsigned int j=i; j<mNumberPoints; j+=mNodesPerFace) {
              backFace[index++] = newIds[j];
            }
          }
          addFaceToHash(threeIndex, twoIndex, sixIndex, sevenIndex, hash, 
                        backFace);
        }
        if(topFace.size() == 0) {
          topFace.resize(mNodesPerFace);
          unsigned int index = 0;
          for(unsigned int i=mNodesPerEdge-1; 
              i<mNumberPoints; i+=mNodesPerEdge) {
            topFace[index++] = newIds[i];
          }
          addFaceToHash(fourIndex, sevenIndex, sixIndex, fiveIndex, hash, 
                        topFace);
        }

        // add ids to map
        oldIdToNewId[zeroIndex] = newIds[mZeroIndex];
        oldIdToNewId[oneIndex] = newIds[mOneIndex];
        oldIdToNewId[twoIndex] = newIds[mTwoIndex];
        oldIdToNewId[threeIndex] = newIds[mThreeIndex];
        oldIdToNewId[fourIndex] = newIds[mFourIndex];
        oldIdToNewId[fiveIndex] = newIds[mFiveIndex];
        oldIdToNewId[sixIndex] = newIds[mSixIndex];
        oldIdToNewId[sevenIndex] = newIds[mSevenIndex];
   
      }

      delete [] newPoints;

      if(releaseTopology) {
        topology->release();
      }
      if(releaseGeometry) {
        geometry->release();
      }

      remapTopology<ORDER>(toReturnTopology);

      if(heavyDataWriter) {
        toReturnTopology->accept(heavyDataWriter);
        toReturnTopology->release();
        toReturnGeometry->accept(heavyDataWriter);
        toReturnGeometry->release();
      }

      handleSetConversion(gridToConvert,
			  toReturn,
			  oldIdToNewId,
			  heavyDataWriter);
      return toReturn;
    }

  private:
    static const unsigned int mNodesPerEdge = ORDER + 1;
    static const unsigned int mNodesPerFace = (ORDER + 1) * (ORDER + 1);
    static const unsigned int mNumberPoints = 
      (ORDER + 1) * (ORDER + 1) * (ORDER + 1);
    static const unsigned int mZeroIndex = 
      0;
    static const unsigned int mOneIndex = 
      (ORDER + 1) * (ORDER + 1) * (ORDER + 1) - (ORDER + 1) * (ORDER + 1);
    static const unsigned int mTwoIndex = 
      (ORDER + 1) * (ORDER + 1) * (ORDER + 1) - (ORDER + 1);
    static const unsigned int mThreeIndex = 
      (ORDER + 1) * (ORDER + 1) - (ORDER + 1);
    static const unsigned int mFourIndex =
      ORDER;
    static const unsigned int mFiveIndex = 
      (ORDER + 1) * (ORDER + 1) * (ORDER + 1) - (ORDER + 1) * (ORDER + 1) + 
      ORDER;
    static const unsigned int mSixIndex = 
      (ORDER + 1) * (ORDER + 1) * (ORDER + 1) - 1;
    static const unsigned int mSevenIndex =
      (ORDER + 1) * (ORDER + 1) - 1;
    static const double points[];

  };

  template <>
  const double HexahedronToHighOrderHexahedron<3, true>::points[] = {
    0.0,
    0.5-0.1*sqrt(5.0),
    0.5+0.1*sqrt(5.0),
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<4, true>::points[] = {
    0.0,
    0.5-sqrt(21.0)/14.0,
    0.5,
    0.5+sqrt(21.0)/14.0,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<5, true>::points[] = {
    0.0,
    0.5-sqrt((7.0+2.0*sqrt(7.0))/84.0),
    0.5-sqrt((7.0-2.0*sqrt(7.0))/84.0),
    0.5+sqrt((7.0-2.0*sqrt(7.0))/84.0),
    0.5+sqrt((7.0+2.0*sqrt(7.0))/84.0),
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<6, true>::points[] = {
    0.0,
    0.5-sqrt((15.0+2.0*sqrt(15.0))/132.0),
    0.5-sqrt((15.0-2.0*sqrt(15.0))/132.0),
    0.5,
    0.5+sqrt((15.0-2.0*sqrt(15.0))/132.0),
    0.5+sqrt((15.0+2.0*sqrt(15.0))/132.0),
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<7, true>::points[] = {
    0.0,
    0.064129925745196714,
    0.20414990928342885,
    0.39535039104876057,
    0.60464960895123943,
    0.79585009071657109,
    0.93587007425480329,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<8, true>::points[] = {
    0.0,
    0.050121002294269912,
    0.16140686024463108,
    0.31844126808691087,
    0.5,
    0.68155873191308913,
    0.83859313975536898,
    0.94987899770573003,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<9, true>::points[] = {
    0.0,
    0.040233045916770627,
    0.13061306744724743,
    0.26103752509477773,
    0.4173605211668065,
    0.58263947883319345,
    0.73896247490522227,
    0.86938693255275257,
    0.95976695408322943,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<10, true>::points[] = {
    0.0,
    0.032999284795970474,
    0.10775826316842779,
    0.21738233650189748,
    0.35212093220653029,
    0.5,
    0.64787906779346971,
    0.78261766349810258,
    0.89224173683157226,
    0.96700071520402953,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<2, false>::points[] = {
    0.0,
    0.5,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<3, false>::points[] = {
    0.0,
    1.0/3.0,
    2.0/3.0,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<4, false>::points[] = {
    0.0,
    0.25,
    0.5,
    0.75,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<5, false>::points[] = {
    0.0,
    0.2,
    0.4,
    0.6,
    0.8,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<6, false>::points[] = {
    0.0,
    1.0/6.0,
    1.0/3.0,
    0.5,
    2.0/3.0,
    5.0/6.0,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<7, false>::points[] = {
    0.0,
    1.0/7.0,
    2.0/7.0,
    3.0/7.0,
    4.0/7.0,
    5.0/7.0,
    6.0/7.0,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<8, false>::points[] = {
    0.0,
    0.125,
    0.25,
    0.375,
    0.5,
    0.625,
    0.75,
    0.875,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<9, false>::points[] = {
    0.0,
    1.0/9.0,
    2.0/9.0,
    1.0/3.0,
    4.0/9.0,
    5.0/9.0,
    2.0/3.0,
    7.0/9.0,
    8.0/9.0,
    1.0
  };

  template <>
  const double HexahedronToHighOrderHexahedron<10, false>::points[] = {
    0.0,
    0.1,
    0.2,
    0.3,
    0.4,
    0.5,
    0.6,
    0.7,
    0.8,
    0.9,
    1.0
  };

  template <unsigned int ORDER>
  class HighOrderHexahedronToHexahedron : public Tessellator {

  public:

    HighOrderHexahedronToHexahedron() :
      Tessellator(ORDER * ORDER * ORDER)
    {
    }

    void
    tesselateTopology(shared_ptr<XdmfTopology> topologyToConvert,
                      shared_ptr<XdmfTopology> topologyToReturn) const
    {
      topologyToReturn->setType(XdmfTopologyType::Hexahedron());
      topologyToReturn->initialize(topologyToConvert->getArrayType(),
                                   8 * ORDER * ORDER * ORDER * topologyToConvert->getNumberElements());

      unsigned int newIndex = 0;
      int indexA = 0;
      int indexB = mNodesPerEdge * mNodesPerEdge;
      int indexC = mNodesPerEdge * mNodesPerEdge + mNodesPerEdge;
      int indexD = mNodesPerEdge;
      for(unsigned int i=0; i<topologyToConvert->getNumberElements(); ++i) {
        for(unsigned int j=0; j<ORDER; ++j) {
          for(unsigned int k=0; k<ORDER; ++k) {
            for(unsigned int l=0; l<ORDER; ++l){
              topologyToReturn->insert(newIndex++, topologyToConvert, indexA++);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexB++);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexC++);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexD++);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexA);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexB);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexC);
              topologyToReturn->insert(newIndex++, topologyToConvert, indexD);
            }
            indexA++;
            indexB++;
            indexC++;
            indexD++;
          }
          indexA += mNodesPerEdge;
          indexB += mNodesPerEdge;
          indexC += mNodesPerEdge;
          indexD += mNodesPerEdge;
        }
        indexA += mNodesPerEdge * mNodesPerEdge;
        indexB += mNodesPerEdge * mNodesPerEdge;
        indexC += mNodesPerEdge * mNodesPerEdge;
        indexD += mNodesPerEdge * mNodesPerEdge;
      }
    }

  private:
    static const unsigned int mNodesPerEdge = (ORDER + 1);

  };

}

shared_ptr<XdmfTopologyConverter>
XdmfTopologyConverter::New()
{
  shared_ptr<XdmfTopologyConverter> p(new XdmfTopologyConverter());
  return p;
}

XdmfTopologyConverter::XdmfTopologyConverter()
{
}

XdmfTopologyConverter::~XdmfTopologyConverter()
{
}

shared_ptr<XdmfUnstructuredGrid>
XdmfTopologyConverter::convert(const shared_ptr<XdmfUnstructuredGrid> gridToConvert,
                               const shared_ptr<const XdmfTopologyType> topologyType,
                               const shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
{
  // Make sure geometry and topology are non null
  if(!(gridToConvert->getGeometry() && gridToConvert->getTopology()))
    XdmfError::message(XdmfError::FATAL,
                       "Current grid's geometry or topology is null "
                       "in XdmfTopologyConverter::convert");

  shared_ptr<const XdmfTopologyType> topologyTypeToConvert =
    gridToConvert->getTopology()->getType();
  if(topologyTypeToConvert == topologyType) {
    // No conversion necessary
    return gridToConvert;
  }

  if(gridToConvert->getGeometry()->getType() != XdmfGeometryType::XYZ()) {
    XdmfError::message(XdmfError::FATAL,
                       "Grid to convert's type is not 'XYZ' in "
                       "XdmfTopologyConverter::convert");
  }

  Converter * converter = NULL;
  if(topologyTypeToConvert == XdmfTopologyType::Hexahedron()) {
    if(topologyType == XdmfTopologyType::Hexahedron_27()) {
      converter = new HexahedronToHighOrderHexahedron<2, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_64()) {
      converter = new HexahedronToHighOrderHexahedron<3, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_64()) {
      converter = new HexahedronToHighOrderHexahedron<3, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_125()) {
      converter = new HexahedronToHighOrderHexahedron<4, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_125()) {
      converter = new HexahedronToHighOrderHexahedron<4, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_216()) {
      converter = new HexahedronToHighOrderHexahedron<5, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_216()) {
      converter = new HexahedronToHighOrderHexahedron<5, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_343()) {
      converter = new HexahedronToHighOrderHexahedron<6, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_343()) {
      converter = new HexahedronToHighOrderHexahedron<6, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_512()) {
      converter = new HexahedronToHighOrderHexahedron<7, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_512()) {
      converter = new HexahedronToHighOrderHexahedron<7, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_729()) {
      converter = new HexahedronToHighOrderHexahedron<8, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_729()) {
      converter = new HexahedronToHighOrderHexahedron<8, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_1000()) {
      converter = new HexahedronToHighOrderHexahedron<9, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_1000()) {
      converter = new HexahedronToHighOrderHexahedron<9, true>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_1331()) {
      converter = new HexahedronToHighOrderHexahedron<10, false>();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_Spectral_1331()) {
      converter = new HexahedronToHighOrderHexahedron<10, true>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_64() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_64()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<3>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_125() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_125()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<4>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_216() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_216()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<5>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_343() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_343()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<6>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_512() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_512()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<7>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_729() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_729()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<8>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_1000() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_1000()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<9>();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_1331() ||
          topologyTypeToConvert == 
          XdmfTopologyType::Hexahedron_Spectral_1331()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new HighOrderHexahedronToHexahedron<10>();
    }
  }

  if(converter) {
    if(heavyDataWriter) {
      heavyDataWriter->openFile();
    }

    shared_ptr<XdmfUnstructuredGrid> toReturn =
      converter->convert(gridToConvert,
                         topologyType,
                         heavyDataWriter);

    if(heavyDataWriter) {
      heavyDataWriter->closeFile();
    }

    delete converter;
    return toReturn;
  }
  else {
    XdmfError::message(XdmfError::FATAL,
                       "Cannot convert topology type in "
                       "XdmfTopologyConverter::convert");
  }

  // not reached
  return shared_ptr<XdmfUnstructuredGrid>();

}
