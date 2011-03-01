/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfTopology.cpp                                                    */
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

#include <sstream>
#include "XdmfTopology.hpp"
#include "XdmfTopologyType.hpp"

boost::shared_ptr<XdmfTopology>
XdmfTopology::New()
{
  boost::shared_ptr<XdmfTopology> p(new XdmfTopology());
  return p;
}

XdmfTopology::XdmfTopology() :
  mType(XdmfTopologyType::NoTopologyType())
{
}

XdmfTopology::~XdmfTopology()
{
}

const std::string XdmfTopology::ItemTag = "Topology";

std::string
XdmfTopology::getItemTag() const
{
  return ItemTag;
}

std::map<std::string, std::string>
XdmfTopology::getItemProperties() const
{
  std::map<std::string, std::string> topologyProperties;
  mType->getProperties(topologyProperties);
  if(mType->getCellType() != XdmfTopologyType::Structured &&
     mType != XdmfTopologyType::Polyvertex()) {
    std::stringstream numElements;
    numElements << this->getNumberElements();
    topologyProperties["Dimensions"] = numElements.str();
  }
  return topologyProperties;
}

unsigned int
XdmfTopology::getNumberElements() const
{
  if(mType->getNodesPerElement() == 0) {
    return 0;
  }
  return this->getSize() / mType->getNodesPerElement();
}

boost::shared_ptr<const XdmfTopologyType>
XdmfTopology::getType() const
{
  return mType;
}

void
XdmfTopology::populateItem(const std::map<std::string, std::string> & itemProperties,
                           std::vector<boost::shared_ptr<XdmfItem> > & childItems,
                           const XdmfCoreReader * const reader)
{
  XdmfItem::populateItem(itemProperties, childItems, reader);
  mType = XdmfTopologyType::New(itemProperties);
  for(std::vector<boost::shared_ptr<XdmfItem> >::const_iterator iter = childItems.begin();
      iter != childItems.end();
      ++iter) {
    if(boost::shared_ptr<XdmfArray> array =
       boost::shared_dynamic_cast<XdmfArray>(*iter)) {
      this->swap(array);
    }
  }
}

void
XdmfTopology::setType(const boost::shared_ptr<const XdmfTopologyType> type)
{
  mType = type;
}
