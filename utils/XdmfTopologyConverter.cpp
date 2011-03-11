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
#include "XdmfAttribute.hpp"
#include "XdmfAttributeCenter.hpp"
#include "XdmfAttributeType.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfGeometryType.hpp"
#include "XdmfHeavyDataWriter.hpp"
#include "XdmfTopology.hpp"
#include "XdmfTopologyConverter.hpp"
#include "XdmfTopologyType.hpp"
#include "XdmfUnstructuredGrid.hpp"
#include "XdmfError.hpp"

//
// local methods
//
namespace {

  // Classes that perform topology conversions. Converter is the root base
  // class.  Tessellator is a subclass of Converter that deals with cases where
  // the mesh only needs to be tessellated to carry out the conversion
  // (e.g. Hexahedron_64 to Hexahedron.

  class Converter;
  class Tessellator;
  class HexahedronToHexahedron_64;
  class HexahedronToHexahedron_64_GLL;
  class HexahedronToHexahedron_125;
  class HexahedronToHexahedron_125_GLL;
  class Hexahedron_64ToHexahedron;
  class Hexahedron_125ToHexahedron;

  class Converter {

  public:

    Converter()
    {
    }

    virtual ~Converter()
    {
    }

    virtual boost::shared_ptr<XdmfUnstructuredGrid>
    convert(const boost::shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const = 0;

  protected:

    struct PointComparison {
      bool
      operator()(const std::vector<double> & point1,
                 const std::vector<double> & point2) const
      {
        double epsilon = 1e-6;
        for(unsigned int i=0; i<3; ++i) {
          if(fabs(point1[i] - point2[i]) > epsilon) {
            return point1[i] < point2[i];
          }
        }
        return false;
      }
    };

    void
    insertPointWithoutCheck(const std::vector<double> & newPoint,
                            const boost::shared_ptr<XdmfTopology> & newConnectivity,
                            const boost::shared_ptr<XdmfGeometry> & newPoints) const
    {
      newConnectivity->pushBack<unsigned int>(newPoints->getSize() / 3);
      newPoints->pushBack(newPoint[0]);
      newPoints->pushBack(newPoint[1]);
      newPoints->pushBack(newPoint[2]);
    }

    void
    insertPointWithCheck(const std::vector<double> & newPoint,
                         std::map<std::vector<double>, unsigned int, PointComparison> & coordToIdMap,
                         const boost::shared_ptr<XdmfTopology> & newConnectivity,
                         const boost::shared_ptr<XdmfGeometry> & newPoints) const
    {
      std::map<std::vector<double>, unsigned int>::const_iterator iter =
        coordToIdMap.find(newPoint);
      if(iter == coordToIdMap.end()) {
        // Not inserted before
        coordToIdMap[newPoint] = newPoints->getSize() / 3;;
        insertPointWithoutCheck(newPoint, newConnectivity, newPoints);
      }
      else {
        newConnectivity->pushBack(iter->second);
      }
    }

  };

  class Tessellator : public Converter {

  public:

    virtual ~Tessellator()
    {
    }

    boost::shared_ptr<XdmfUnstructuredGrid>
    convert(const boost::shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
    {
      boost::shared_ptr<XdmfUnstructuredGrid> toReturn =
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
        boost::shared_ptr<XdmfAttribute> currAttribute =
          gridToConvert->getAttribute(i);
          boost::shared_ptr<XdmfAttribute> createdAttribute =
            boost::shared_ptr<XdmfAttribute>();
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
    tesselateTopology(boost::shared_ptr<XdmfTopology> topologyToConvert,
                      boost::shared_ptr<XdmfTopology> topologyToReturn) const = 0;

  protected:

    Tessellator(const unsigned int numTesselations) :
      mNumTesselations(numTesselations)
    {
    }

    const unsigned int mNumTesselations;

  };

  class HexahedronToHexahedron_64 : public Converter {

  public:

    HexahedronToHexahedron_64()
    {
    }

    virtual ~HexahedronToHexahedron_64()
    {
    }

    virtual void
    computeInteriorPoints(std::vector<double> & leftPoint,
                          std::vector<double> & rightPoint,
                          const std::vector<double> & point1,
                          const std::vector<double> & point2) const
    {
      this->computeLeftPoint(leftPoint, point1, point2);
      this->computeRightPoint(rightPoint, point1, point2);
    }

    virtual void
    computeLeftPoint(std::vector<double> & leftPoint,
                     const std::vector<double> & point1,
                     const std::vector<double> & point2) const
    {
      leftPoint[0] = (1.0/3.0)*(point2[0] + 2*point1[0]);
      leftPoint[1] = (1.0/3.0)*(point2[1] + 2*point1[1]);
      leftPoint[2] = (1.0/3.0)*(point2[2] + 2*point1[2]);
    }

    virtual void
    computeRightPoint(std::vector<double> & rightPoint,
                      const std::vector<double> & point1,
                      const std::vector<double> & point2) const
    {
      rightPoint[0] = (1.0/3.0)*(2*point2[0] + point1[0]);
      rightPoint[1] = (1.0/3.0)*(2*point2[1] + point1[1]);
      rightPoint[2] = (1.0/3.0)*(2*point2[2] + point1[2]);
    }

    boost::shared_ptr<XdmfUnstructuredGrid>
    convert(const boost::shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
    {
      boost::shared_ptr<XdmfUnstructuredGrid> toReturn =
        XdmfUnstructuredGrid::New();
      toReturn->setName(gridToConvert->getName());

      boost::shared_ptr<XdmfGeometry> toReturnGeometry =
        toReturn->getGeometry();
      toReturnGeometry->setType(gridToConvert->getGeometry()->getType());
      toReturnGeometry->initialize(gridToConvert->getGeometry()->getArrayType(),
                                   gridToConvert->getGeometry()->getSize());

      bool releaseGeometry = false;
      if(!gridToConvert->getGeometry()->isInitialized()) {
        gridToConvert->getGeometry()->read();
        releaseGeometry = true;
      }

      // Copy all geometry values from old grid into new grid because we are
      // keeping all old points.
      toReturnGeometry->insert(0,
                               gridToConvert->getGeometry(),
                               0,
                               gridToConvert->getGeometry()->getSize());

      if(releaseGeometry) {
        gridToConvert->getGeometry()->release();
      }

      boost::shared_ptr<XdmfTopology> toReturnTopology =
        toReturn->getTopology();
      toReturnTopology->setType(XdmfTopologyType::Hexahedron_64());
      toReturnTopology->initialize(gridToConvert->getTopology()->getArrayType());
      toReturnTopology->reserve(64 * gridToConvert->getTopology()->getNumberElements());

      bool releaseTopology = false;
      if(!gridToConvert->getTopology()->isInitialized()) {
        gridToConvert->getTopology()->read();
      }

      std::vector<double> leftPoint(3);
      std::vector<double> rightPoint(3);
      std::map<std::vector<double>, unsigned int, PointComparison> coordToIdMap;

      std::vector<std::vector<double> > localNodes(44, std::vector<double>(3));

      for(unsigned int i=0;
          i<gridToConvert->getTopology()->getNumberElements();
          ++i) {
        // Fill localNodes with original coordinate information.
        for(int j=0; j<8; ++j) {
          toReturnGeometry->getValues(gridToConvert->getTopology()->getValue<unsigned int>(8*i + j) * 3,
                                      &localNodes[j][0], 3);
        }

        // Add old connectivity information to newConnectivity.
        toReturnTopology->resize(toReturnTopology->getSize() + 8, 0);
        toReturnTopology->insert(64*i, gridToConvert->getTopology(), 8*i, 8);

        // Case 0
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[0],
                                    localNodes[1]);
        localNodes[8] = leftPoint;
        localNodes[9] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 1
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[1],
                                    localNodes[2]);
        localNodes[10] = leftPoint;
        localNodes[11] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 2
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[2],
                                    localNodes[3]);
        localNodes[12] = leftPoint;
        localNodes[13] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 3
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[3],
                                    localNodes[0]);
        localNodes[14] = leftPoint;
        localNodes[15] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 4
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[4],
                                    localNodes[5]);
        localNodes[16] = leftPoint;
        localNodes[17] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 5
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[5],
                                    localNodes[6]);
        localNodes[18] = leftPoint;
        localNodes[19] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 6
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[6],
                                    localNodes[7]);
        localNodes[20] = leftPoint;
        localNodes[21] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 7
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[7],
                                    localNodes[4]);
        localNodes[22] = leftPoint;
        localNodes[23] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 8
        computeLeftPoint(leftPoint, localNodes[0], localNodes[4]);
        localNodes[24] = leftPoint;
        insertPointWithCheck(leftPoint,
                             coordToIdMap,
                             toReturnTopology,
                             toReturnGeometry);

        // Case 9
        this->computeLeftPoint(leftPoint, localNodes[1], localNodes[5]);
        localNodes[25] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 10
        this->computeLeftPoint(leftPoint, localNodes[2], localNodes[6]);
        localNodes[26] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 11
        this->computeLeftPoint(leftPoint, localNodes[3], localNodes[7]);
        localNodes[27] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 12
        this->computeRightPoint(leftPoint, localNodes[0], localNodes[4]);
        localNodes[28] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 13
        this->computeRightPoint(leftPoint, localNodes[1], localNodes[5]);
        localNodes[29] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 14
        this->computeRightPoint(leftPoint, localNodes[2], localNodes[6]);
        localNodes[30] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 15
        this->computeRightPoint(leftPoint, localNodes[3], localNodes[7]);
        localNodes[31] = leftPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 16
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[27],
                                    localNodes[24]);
        localNodes[32] = leftPoint;
        localNodes[33] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 17
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[25],
                                    localNodes[26]);
        localNodes[34] = leftPoint;
        localNodes[35] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 18
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[24],
                                    localNodes[25]);
        localNodes[36] = leftPoint;
        localNodes[37] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 19
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[26],
                                    localNodes[27]);
        localNodes[38] = leftPoint;
        localNodes[39] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 20
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[31],
                                    localNodes[28]);
        localNodes[40] = leftPoint;
        localNodes[41] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 21
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[29],
                                    localNodes[30]);
        localNodes[42] = leftPoint;
        localNodes[43] = rightPoint;
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 22
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[28],
                                    localNodes[29]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 23
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[30],
                                    localNodes[31]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 24
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[15],
                                    localNodes[10]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 25
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[11],
                                    localNodes[14]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 26
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[23],
                                    localNodes[18]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 27
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[19],
                                    localNodes[22]);
        this->insertPointWithCheck(leftPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(rightPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 28
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[33],
                                    localNodes[34]);
        this->insertPointWithoutCheck(leftPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(rightPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 29
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[35],
                                    localNodes[32]);
        this->insertPointWithoutCheck(leftPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(rightPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 30
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[41],
                                    localNodes[42]);
        this->insertPointWithoutCheck(leftPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(rightPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 31
        this->computeInteriorPoints(leftPoint,
                                    rightPoint,
                                    localNodes[43],
                                    localNodes[40]);
        this->insertPointWithoutCheck(leftPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(rightPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
      }
      if(releaseTopology) {
        gridToConvert->getTopology()->release();
      }
      if(heavyDataWriter) {
        toReturnTopology->accept(heavyDataWriter);
        toReturnTopology->release();
        toReturnGeometry->accept(heavyDataWriter);
        toReturnGeometry->release();
      }
      return toReturn;
    }
  };

  class HexahedronToHexahedron_64_GLL : public HexahedronToHexahedron_64 {

  public:

    HexahedronToHexahedron_64_GLL()
    {
    }

    void
    computeLeftPoint(std::vector<double> & leftPoint,
                     const std::vector<double> & point1,
                     const std::vector<double> & point2) const
    {
      leftPoint[0] = (1.0/2.0)*((1-C)*point2[0] + (1+C)*point1[0]);
      leftPoint[1] = (1.0/2.0)*((1-C)*point2[1] + (1+C)*point1[1]);
      leftPoint[2] = (1.0/2.0)*((1-C)*point2[2] + (1+C)*point1[2]);
    }

    void computeRightPoint(std::vector<double> & rightPoint,
                           const std::vector<double> & point1,
                           const std::vector<double> & point2) const
    {
      rightPoint[0] = (1.0/2.0)*((1+C)*point2[0] + (1-C)*point1[0]);
      rightPoint[1] = (1.0/2.0)*((1+C)*point2[1] + (1-C)*point1[1]);
      rightPoint[2] = (1.0/2.0)*((1+C)*point2[2] + (1-C)*point1[2]);
    }

  private:

    static const double C;

  };

  const double HexahedronToHexahedron_64_GLL::C = 1 / std::sqrt(5.0);

  class HexahedronToHexahedron_125 : public Converter {

  public:

    HexahedronToHexahedron_125()
    {
    }

    virtual ~HexahedronToHexahedron_125()
    {
    }

    void
    computeInteriorPoints(std::vector<double> & quarterPoint,
                          std::vector<double> & midPoint,
                          std::vector<double> & threeQuarterPoint,
                          const std::vector<double> & point1,
                          const std::vector<double> & point2) const
    {
      this->computeQuarterPoint(quarterPoint, point1, point2);
      this->computeMidPoint(midPoint, point1, point2);
      this->computeThreeQuarterPoint(threeQuarterPoint, point1, point2);
    }

    virtual
    void computeQuarterPoint(std::vector<double> & quarterPoint,
                             const std::vector<double> & point1,
                             const std::vector<double> & point2) const
    {
      quarterPoint[0] = (1.0/4.0)*(point2[0] + 3*point1[0]);
      quarterPoint[1] = (1.0/4.0)*(point2[1] + 3*point1[1]);
      quarterPoint[2] = (1.0/4.0)*(point2[2] + 3*point1[2]);
    }

    void
    computeMidPoint(std::vector<double> & midPoint,
                    const std::vector<double> & point1,
                    const std::vector<double> & point2) const
    {
      midPoint[0] = (1.0/2.0)*(point2[0] + point1[0]);
      midPoint[1] = (1.0/2.0)*(point2[1] + point1[1]);
      midPoint[2] = (1.0/2.0)*(point2[2] + point1[2]);
    }

    virtual void
    computeThreeQuarterPoint(std::vector<double> & threeQuarterPoint,
                             const std::vector<double> & point1,
                             const std::vector<double> & point2) const
    {
      threeQuarterPoint[0] = (1.0/4.0)*(3.0*point2[0] + point1[0]);
      threeQuarterPoint[1] = (1.0/4.0)*(3.0*point2[1] + point1[1]);
      threeQuarterPoint[2] = (1.0/4.0)*(3.0*point2[2] + point1[2]);
    }

    boost::shared_ptr<XdmfUnstructuredGrid>
    convert(const boost::shared_ptr<XdmfUnstructuredGrid> gridToConvert,
            const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
    {
      boost::shared_ptr<XdmfUnstructuredGrid> toReturn =
        XdmfUnstructuredGrid::New();
      toReturn->setName(gridToConvert->getName());

      boost::shared_ptr<XdmfGeometry> toReturnGeometry =
        toReturn->getGeometry();
      toReturnGeometry->setType(gridToConvert->getGeometry()->getType());
      toReturnGeometry->initialize(gridToConvert->getGeometry()->getArrayType(),
                                   gridToConvert->getGeometry()->getSize());

      bool releaseGeometry = false;
      if(!gridToConvert->getGeometry()->isInitialized()) {
        gridToConvert->getGeometry()->read();
        releaseGeometry = true;
      }

      // Copy all geometry values from old grid into new grid because we are
      // keeping all old points.
      toReturnGeometry->insert(0,
                               gridToConvert->getGeometry(),
                               0,
                               gridToConvert->getGeometry()->getSize());

      if(releaseGeometry) {
        gridToConvert->getGeometry()->release();
      }

      boost::shared_ptr<XdmfTopology> toReturnTopology = toReturn->getTopology();
      toReturn->getTopology()->setType(XdmfTopologyType::Hexahedron_125());
      toReturnTopology->initialize(gridToConvert->getTopology()->getArrayType());
      toReturnTopology->reserve(125 * gridToConvert->getTopology()->getNumberElements());

      bool releaseTopology = false;
      if(!gridToConvert->getTopology()->isInitialized()) {
        gridToConvert->getTopology()->read();
      }

      std::vector<double> quarterPoint(3);
      std::vector<double> midPoint(3);
      std::vector<double> threeQuarterPoint(3);
      std::map<std::vector<double>, unsigned int, PointComparison> coordToIdMap;

      std::vector<std::vector<double> > localNodes(80, std::vector<double>(3));

      for(unsigned int i=0; i<gridToConvert->getTopology()->getNumberElements(); ++i) {
        // Fill localNodes with original coordinate information.

        for(int j=0; j<8; ++j) {
          toReturnGeometry->getValues(gridToConvert->getTopology()->getValue<unsigned int>(8*i + j) * 3,
                                      &localNodes[j][0], 3);
        }

        // Add old connectivity information to toReturnTopology.
        toReturnTopology->resize(toReturnTopology->getSize() + 8, 0);
        toReturnTopology->insert(125*i, gridToConvert->getTopology(), 8*i, 8);

        // Case 0
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[0],
                                    localNodes[1]);
        localNodes[8] = quarterPoint;
        localNodes[9] = midPoint;
        localNodes[10] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 1
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[1],
                                    localNodes[2]);
        localNodes[11] = quarterPoint;
        localNodes[12] = midPoint;
        localNodes[13] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 2
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[2],
                                    localNodes[3]);
        localNodes[14] = quarterPoint;
        localNodes[15] = midPoint;
        localNodes[16] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 3
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[3],
                                    localNodes[0]);
        localNodes[17] = quarterPoint;
        localNodes[18] = midPoint;
        localNodes[19] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 4
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[4],
                                    localNodes[5]);
        localNodes[20] = quarterPoint;
        localNodes[21] = midPoint;
        localNodes[22] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 5
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[5],
                                    localNodes[6]);
        localNodes[23] = quarterPoint;
        localNodes[24] = midPoint;
        localNodes[25] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 6
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[6],
                                    localNodes[7]);
        localNodes[26] = quarterPoint;
        localNodes[27] = midPoint;
        localNodes[28] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 7
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[7],
                                    localNodes[4]);
        localNodes[29] = quarterPoint;
        localNodes[30] = midPoint;
        localNodes[31] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 8
        this->computeQuarterPoint(quarterPoint,
                                  localNodes[0],
                                  localNodes[4]);
        localNodes[32] = quarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 9
        this->computeQuarterPoint(quarterPoint,
                                  localNodes[1],
                                  localNodes[5]);
        localNodes[33] = quarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 10
        this->computeQuarterPoint(quarterPoint,
                                  localNodes[2],
                                  localNodes[6]);
        localNodes[34] = quarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 11
        this->computeQuarterPoint(quarterPoint,
                                  localNodes[3],
                                  localNodes[7]);
        localNodes[35] = quarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 12
        this->computeThreeQuarterPoint(threeQuarterPoint,
                                       localNodes[0],
                                       localNodes[4]);
        localNodes[36] = threeQuarterPoint;
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 13
        this->computeThreeQuarterPoint(threeQuarterPoint,
                                       localNodes[1],
                                       localNodes[5]);
        localNodes[37] = threeQuarterPoint;
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 14
        this->computeThreeQuarterPoint(threeQuarterPoint,
                                       localNodes[2],
                                       localNodes[6]);
        localNodes[38] = threeQuarterPoint;
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 15
        this->computeThreeQuarterPoint(threeQuarterPoint,
                                       localNodes[3],
                                       localNodes[7]);
        localNodes[39] = threeQuarterPoint;
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 16
        this->computeMidPoint(midPoint,
                              localNodes[0],
                              localNodes[4]);
        localNodes[40] = midPoint;
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 17
        this->computeMidPoint(midPoint,
                              localNodes[1],
                              localNodes[5]);
        localNodes[41] = midPoint;
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 18
        this->computeMidPoint(midPoint,
                              localNodes[2],
                              localNodes[6]);
        localNodes[42] = midPoint;
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 19
        this->computeMidPoint(midPoint,
                              localNodes[3],
                              localNodes[7]);
        localNodes[43] = midPoint;
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 20
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[32],
                                    localNodes[33]);
        localNodes[44] = quarterPoint;
        localNodes[45] = midPoint;
        localNodes[46] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 21
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[33],
                                    localNodes[34]);
        localNodes[47] = quarterPoint;
        localNodes[48] = midPoint;
        localNodes[49] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 22
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[34],
                                    localNodes[35]);
        localNodes[50] = quarterPoint;
        localNodes[51] = midPoint;
        localNodes[52] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 23
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[35],
                                    localNodes[32]);
        localNodes[53] = quarterPoint;
        localNodes[54] = midPoint;
        localNodes[55] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 24
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[36],
                                    localNodes[37]);
        localNodes[56] = quarterPoint;
        localNodes[57] = midPoint;
        localNodes[58] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 25
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[37],
                                    localNodes[38]);
        localNodes[59] = quarterPoint;
        localNodes[60] = midPoint;
        localNodes[61] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 26
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[38],
                                    localNodes[39]);
        localNodes[62] = quarterPoint;
        localNodes[63] = midPoint;
        localNodes[64] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 27
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[39],
                                    localNodes[36]);
        localNodes[65] = quarterPoint;
        localNodes[66] = midPoint;
        localNodes[67] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 28
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[40],
                                    localNodes[41]);
        localNodes[68] = quarterPoint;
        localNodes[69] = midPoint;
        localNodes[70] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 29
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[41],
                                    localNodes[42]);
        localNodes[71] = quarterPoint;
        localNodes[72] = midPoint;
        localNodes[73] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 30
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[42],
                                    localNodes[43]);
        localNodes[74] = quarterPoint;
        localNodes[75] = midPoint;
        localNodes[76] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 31
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[43],
                                    localNodes[40]);
        localNodes[77] = quarterPoint;
        localNodes[78] = midPoint;
        localNodes[79] = threeQuarterPoint;
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 32
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[19],
                                    localNodes[11]);
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 33
        this->computeMidPoint(midPoint,
                              localNodes[10],
                              localNodes[14]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 34
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[13],
                                    localNodes[17]);
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 35
        this->computeMidPoint(midPoint, localNodes[16], localNodes[8]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 36
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[31],
                                    localNodes[23]);
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 37
        this->computeMidPoint(midPoint, localNodes[22], localNodes[26]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 38
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[25],
                                    localNodes[29]);
        this->insertPointWithCheck(quarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);
        this->insertPointWithCheck(threeQuarterPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 39
        this->computeMidPoint(midPoint, localNodes[28], localNodes[20]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 40
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[55],
                                    localNodes[47]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 41
        this->computeMidPoint(midPoint, localNodes[46], localNodes[50]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 42
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[49],
                                    localNodes[53]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 43
        this->computeMidPoint(midPoint, localNodes[52], localNodes[44]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 44
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[67],
                                    localNodes[59]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 45
        this->computeMidPoint(midPoint, localNodes[62], localNodes[58]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 46
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[61],
                                    localNodes[65]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 47
        this->computeMidPoint(midPoint, localNodes[56], localNodes[64]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 48
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[79],
                                    localNodes[71]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 49
        this->computeMidPoint(midPoint, localNodes[70], localNodes[74]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 50
        this->computeInteriorPoints(quarterPoint,
                                    midPoint,
                                    threeQuarterPoint,
                                    localNodes[73],
                                    localNodes[77]);
        this->insertPointWithoutCheck(quarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
        this->insertPointWithoutCheck(threeQuarterPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 51
        this->computeMidPoint(midPoint, localNodes[76], localNodes[68]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 52
        this->computeMidPoint(midPoint, localNodes[12], localNodes[18]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 53
        this->computeMidPoint(midPoint, localNodes[24], localNodes[30]);
        this->insertPointWithCheck(midPoint,
                                   coordToIdMap,
                                   toReturnTopology,
                                   toReturnGeometry);

        // Case 54
        this->computeMidPoint(midPoint, localNodes[48], localNodes[54]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 55
        this->computeMidPoint(midPoint, localNodes[60], localNodes[66]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);

        // Case 56
        this->computeMidPoint(midPoint, localNodes[72], localNodes[78]);
        this->insertPointWithoutCheck(midPoint,
                                      toReturnTopology,
                                      toReturnGeometry);
      }
      if(releaseTopology) {
        gridToConvert->getTopology()->release();
      }
      if(heavyDataWriter) {
        toReturnTopology->accept(heavyDataWriter);
        toReturnTopology->release();
        toReturnGeometry->accept(heavyDataWriter);
        toReturnGeometry->release();
      }
      return toReturn;
    }
  };

  class HexahedronToHexahedron_125_GLL : public HexahedronToHexahedron_125 {

  public:

    HexahedronToHexahedron_125_GLL()
    {
    }

    void
    computeQuarterPoint(std::vector<double> & quarterPoint,
                        const std::vector<double> & point1,
                        const std::vector<double> & point2) const
    {
      quarterPoint[0] = (1.0/2.0)*((1-C) * point2[0] + (1+C) * point1[0]);
      quarterPoint[1] = (1.0/2.0)*((1-C) * point2[1] + (1+C) * point1[1]);
      quarterPoint[2] = (1.0/2.0)*((1-C) * point2[2] + (1+C) * point1[2]);
    }

    void
    computeThreeQuarterPoint(std::vector<double> & threeQuarterPoint,
                             const std::vector<double> & point1,
                             const std::vector<double> & point2) const
    {
      threeQuarterPoint[0] = (1.0/2.0)*((1+C) * point2[0] + (1-C) * point1[0]);
      threeQuarterPoint[1] = (1.0/2.0)*((1+C) * point2[1] + (1-C) * point1[1]);
      threeQuarterPoint[2] = (1.0/2.0)*((1+C) * point2[2] + (1-C) * point1[2]);
    }

  private:

    static const double C;

  };

  const double HexahedronToHexahedron_125_GLL::C = std::sqrt(3.0/7.0);

  class Hexahedron_64ToHexahedron : public Tessellator {

  public:

    Hexahedron_64ToHexahedron() :
      Tessellator(27)
    {
    }

    void
    tesselateTopology(boost::shared_ptr<XdmfTopology> topologyToConvert,
                      boost::shared_ptr<XdmfTopology> topologyToReturn) const
    {
      topologyToReturn->setType(XdmfTopologyType::Hexahedron());
      topologyToReturn->initialize(topologyToConvert->getArrayType(),
                                   216 * topologyToConvert->getNumberElements());

      unsigned int newIndex = 0;
      for(unsigned int i=0; i<topologyToConvert->getNumberElements(); ++i) {
        const unsigned int index = 64 * i;
        topologyToReturn->insert(newIndex++, topologyToConvert, 0 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 8 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 15 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 24 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 8 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 9 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 9 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 1 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 10 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 25 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 15 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 14 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 10 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 11 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 14 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 13 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 3 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 27 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 12 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 13 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 11 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 2 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 12 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 26 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 24 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 28 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 25 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 29 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 27 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 31 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 26 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 30 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 28 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 4 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 16 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 23 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 16 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 17 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 29 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 17 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 5 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 18 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 23 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 22 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 18 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 19 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 31 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 22 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 21 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 7 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 20 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 21 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 30 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 19 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 6 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 20 + index);
      }
    }
  };

  class Hexahedron_125ToHexahedron : public Tessellator {

  public:

    Hexahedron_125ToHexahedron() :
      Tessellator(64)
    {
    }

    void
    tesselateTopology(boost::shared_ptr<XdmfTopology> topologyToConvert,
                      boost::shared_ptr<XdmfTopology> topologyToReturn) const
    {
      topologyToReturn->setType(XdmfTopologyType::Hexahedron());
      topologyToReturn->initialize(topologyToConvert->getArrayType(),
                                   512 * topologyToConvert->getNumberElements());

      unsigned int newIndex = 0;
      for(unsigned int i=0; i<topologyToConvert->getNumberElements(); ++i) {
        const unsigned int index = 125 * i;
        topologyToReturn->insert(newIndex++, topologyToConvert, 0 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 8 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 80 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 19 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 8 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 9 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 81 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 80 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 9 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 10 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 82 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 81 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 10 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 1 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 11 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 82 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 19 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 80 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 87 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 18 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 80 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 81 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 120 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 87 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 81 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 82 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 83 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 120 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 82 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 11 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 12 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 83 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 18 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 87 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 86 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 17 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 87 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 120 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 85 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 86 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 120 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 83 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 84 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 85 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 83 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 12 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 13 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 84 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 17 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 86 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 16 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 3 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 86 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 85 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 15 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 16 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 85 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 84 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 14 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 15 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 84 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 13 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 2 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 14 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 32 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 68 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 79 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 44 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 68 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 69 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 45 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 69 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 70 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 46 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 33 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 70 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 71 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 55 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 79 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 78 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 96 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 97 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 98 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 47 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 71 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 72 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 54 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 78 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 77 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 103 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 122 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 99 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 48 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 72 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 73 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 53 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 35 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 77 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 76 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 102 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 52 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 75 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 76 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 101 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 51 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 74 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 75 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 100 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 49 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 34 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 50 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 73 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 74 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 40 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 68 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 79 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 67 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 68 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 69 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 69 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 70 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 70 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 41 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 71 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 79 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 78 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 67 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 66 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 112 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 113 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 114 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 71 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 72 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 78 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 77 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 66 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 65 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 119 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 124 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 115 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 72 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 73 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 77 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 76 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 43 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 65 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 64 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 118 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 75 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 76 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 64 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 117 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 74 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 75 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 116 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 73 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 42 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 74 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 36 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 67 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 4 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 20 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 88 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 31 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 56 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 20 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 21 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 89 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 88 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 57 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 21 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 22 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 90 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 89 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 58 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 37 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 22 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 5 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 23 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 90 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 67 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 66 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 31 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 88 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 95 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 30 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 104 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 88 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 89 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 121 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 95 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 105 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 89 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 90 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 91 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 121 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 106 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 59 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 90 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 23 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 24 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 91 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 66 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 65 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 30 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 95 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 94 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 29 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 111 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 95 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 121 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 93 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 94 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 123 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 121 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 91 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 92 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 93 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 107 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 60 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 91 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 24 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 25 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 92 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 65 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 64 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 39 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 29 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 94 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 28 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 7 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 110 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 64 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 94 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 93 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 27 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 28 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 109 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 63 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 93 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 92 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 26 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 27 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 108 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 61 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 38 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 62 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 92 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 25 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 6 + index);
        topologyToReturn->insert(newIndex++, topologyToConvert, 26 + index);
      }
    }
  };
}

boost::shared_ptr<XdmfTopologyConverter>
XdmfTopologyConverter::New()
{
  boost::shared_ptr<XdmfTopologyConverter> p(new XdmfTopologyConverter());
  return p;
}

XdmfTopologyConverter::XdmfTopologyConverter()
{
}

XdmfTopologyConverter::~XdmfTopologyConverter()
{
}

boost::shared_ptr<XdmfUnstructuredGrid>
XdmfTopologyConverter::convert(const boost::shared_ptr<XdmfUnstructuredGrid> gridToConvert,
                               const boost::shared_ptr<const XdmfTopologyType> topologyType,
                               const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter) const
{
  // Make sure geometry and topology are non null
  if(!(gridToConvert->getGeometry() && gridToConvert->getTopology()))
    XdmfError::message(XdmfError::FATAL, "Current grid's geometry or topology is null in XdmfTopologyConverter::convert");

  boost::shared_ptr<const XdmfTopologyType> topologyTypeToConvert =
    gridToConvert->getTopology()->getType();
  if(topologyTypeToConvert == topologyType) {
    // No conversion necessary
    return gridToConvert;
  }

  if(gridToConvert->getGeometry()->getType() != XdmfGeometryType::XYZ()) {
    XdmfError::message(XdmfError::FATAL, "Grid to convert's type is not 'XYZ' in XdmfTopologyConverter::convert");
  }

  Converter * converter = NULL;
  if(topologyTypeToConvert == XdmfTopologyType::Hexahedron()) {
    if(topologyType == XdmfTopologyType::Hexahedron_64()) {
      converter = new HexahedronToHexahedron_64();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_64_GLL()) {
      converter = new HexahedronToHexahedron_64_GLL();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_125()) {
      converter = new HexahedronToHexahedron_125();
    }
    else if(topologyType == XdmfTopologyType::Hexahedron_125_GLL()) {
      converter = new HexahedronToHexahedron_125_GLL();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_64()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new Hexahedron_64ToHexahedron();
    }
  }
  else if(topologyTypeToConvert == XdmfTopologyType::Hexahedron_125()) {
    if(topologyType == XdmfTopologyType::Hexahedron()) {
      converter = new Hexahedron_125ToHexahedron();
    }
  }
  if(converter) {
    boost::shared_ptr<XdmfUnstructuredGrid> toReturn =
      converter->convert(gridToConvert, heavyDataWriter);
    delete converter;
    return toReturn;
  }
  else {
    XdmfError::message(XdmfError::FATAL, "Converter NULL because topology type to convert not of valid type in XdmfTopologyConverter::convert");
  }
}
