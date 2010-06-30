/*
 * XdmfGrid.cpp
 *
 *  Created on: Jan 25, 2010
 *      Author: kleiter
 */

#include "XdmfDomain.hpp"
#include "XdmfGrid.hpp"

XdmfDomain::XdmfDomain()
{
	std::cout << "Created Domain " << this << std::endl;
}

XdmfDomain::~XdmfDomain()
{
  std::cout << "Deleted Domain " << this << std::endl;
}

const std::string XdmfDomain::ItemTag = "Domain";

void XdmfDomain::insert(const boost::shared_ptr<XdmfGrid> grid)
{
	mGrids.push_back(grid);
}

boost::shared_ptr<XdmfGrid> XdmfDomain::getGrid(const unsigned int index)
{
	return boost::const_pointer_cast<XdmfGrid>(static_cast<const XdmfDomain &>(*this).getGrid(index));
}

boost::shared_ptr<const XdmfGrid> XdmfDomain::getGrid(const unsigned int index) const
{
	if(index >= mGrids.size())
	{
		assert(false);
		// Out of range --- should we throw exceptions?
	}
	return mGrids[index];
}

std::map<std::string, std::string> XdmfDomain::getItemProperties() const
{
	std::map<std::string, std::string> domainProperties;
	return domainProperties;
}

std::string XdmfDomain::getItemTag() const
{
	return ItemTag;
}

unsigned int XdmfDomain::getNumberOfGrids() const
{
	return mGrids.size();
}

void XdmfDomain::populateItem(const std::map<std::string, std::string> & itemProperties, std::vector<boost::shared_ptr<XdmfItem> > & childItems)
{
	for(std::vector<boost::shared_ptr<XdmfItem> >::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter)
	{
		this->insert(boost::shared_dynamic_cast<XdmfGrid>(*iter));
	}
}

void XdmfDomain::removeGrid(const unsigned int index)
{
	if(index >= mGrids.size())
	{
		assert(false);
	}
	mGrids.erase(mGrids.begin() + index);
}

void XdmfDomain::traverse(const boost::shared_ptr<XdmfBaseVisitor> visitor) const
{
	for(std::vector<boost::shared_ptr<XdmfGrid> >::const_iterator iter = mGrids.begin(); iter != mGrids.end(); ++iter)
	{
		(*iter)->accept(visitor);
	}
}
