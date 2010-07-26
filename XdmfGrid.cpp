/*
 * XdmfGrid.cpp
 *
 *  Created on: Jan 25, 2010
 *      Author: kleiter
 */

#include "XdmfAttribute.hpp"
#include "XdmfGrid.hpp"
#include "XdmfGeometry.hpp"
#include "XdmfSet.hpp"
#include "XdmfTime.hpp"
#include "XdmfTopology.hpp"

boost::shared_ptr<XdmfGrid> XdmfGrid::New()
{
	boost::shared_ptr<XdmfGrid> p(new XdmfGrid());
	return p;
}

XdmfGrid::XdmfGrid() :
	mGeometry(XdmfGeometry::New()),
	mTime(boost::shared_ptr<XdmfTime>()),
	mTopology(XdmfTopology::New()),
	mName("Grid")
{
}

XdmfGrid::~XdmfGrid()
{
}

const std::string XdmfGrid::ItemTag = "Grid";

boost::shared_ptr<XdmfAttribute> XdmfGrid::getAttribute(const unsigned int index)
{
	return boost::const_pointer_cast<XdmfAttribute>(static_cast<const XdmfGrid &>(*this).getAttribute(index));
}

boost::shared_ptr<const XdmfAttribute> XdmfGrid::getAttribute(const unsigned int index) const
{
	if(index >= mAttributes.size())
	{
		assert(false);
	}
	return mAttributes[index];
}

boost::shared_ptr<XdmfAttribute> XdmfGrid::getAttribute(const std::string & name)
{
	return boost::const_pointer_cast<XdmfAttribute>(static_cast<const XdmfGrid &>(*this).getAttribute(name));
}

boost::shared_ptr<const XdmfAttribute> XdmfGrid::getAttribute(const std::string & name) const
{
	for(std::vector<boost::shared_ptr<XdmfAttribute> >::const_iterator iter = mAttributes.begin(); iter != mAttributes.end(); ++iter)
	{
		if((*iter)->getName().compare(name) == 0)
		{
			return *iter;
		}
	}
	return boost::shared_ptr<XdmfAttribute>();
}

boost::shared_ptr<XdmfGeometry> XdmfGrid::getGeometry()
{
	return boost::const_pointer_cast<XdmfGeometry>(static_cast<const XdmfGrid &>(*this).getGeometry());
}

boost::shared_ptr<const XdmfGeometry> XdmfGrid::getGeometry() const
{
	return mGeometry;
}

std::map<std::string, std::string> XdmfGrid::getItemProperties() const
{
	std::map<std::string, std::string> gridProperties;
	gridProperties["Name"] = mName;
	return gridProperties;
}

std::string XdmfGrid::getItemTag() const
{
	return ItemTag;
}

std::string XdmfGrid::getName() const
{
	return mName;
}

unsigned int XdmfGrid::getNumberOfAttributes() const
{
	return mAttributes.size();
}

unsigned int XdmfGrid::getNumberOfSets() const
{
	return mSets.size();
}

boost::shared_ptr<XdmfSet> XdmfGrid::getSet(const unsigned int index)
{
	return boost::const_pointer_cast<XdmfSet>(static_cast<const XdmfGrid &>(*this).getSet(index));
}

boost::shared_ptr<const XdmfSet> XdmfGrid::getSet(const unsigned int index) const
{
	if(index >= mSets.size())
	{
		assert(false);
	}
	return mSets[index];
}

boost::shared_ptr<XdmfSet> XdmfGrid::getSet(const std::string & name)
{
	return boost::const_pointer_cast<XdmfSet>(static_cast<const XdmfGrid &>(*this).getSet(name));
}

boost::shared_ptr<const XdmfSet> XdmfGrid::getSet(const std::string & name) const
{
	for(std::vector<boost::shared_ptr<XdmfSet> >::const_iterator iter = mSets.begin(); iter != mSets.end(); ++iter)
	{
		if((*iter)->getName().compare(name) == 0)
		{
			return *iter;
		}
	}
	return boost::shared_ptr<XdmfSet>();
}

boost::shared_ptr<XdmfTime> XdmfGrid::getTime()
{
	return boost::const_pointer_cast<XdmfTime>(static_cast<const XdmfGrid &>(*this).getTime());
}

boost::shared_ptr<const XdmfTime> XdmfGrid::getTime() const
{
	return mTime;
}

boost::shared_ptr<XdmfTopology> XdmfGrid::getTopology()
{
	return boost::const_pointer_cast<XdmfTopology>(static_cast<const XdmfGrid &>(*this).getTopology());
}

boost::shared_ptr<const XdmfTopology> XdmfGrid::getTopology() const
{
	return mTopology;
}

void XdmfGrid::insert(const boost::shared_ptr<XdmfAttribute> attribute)
{
	mAttributes.push_back(attribute);
}

void XdmfGrid::insert(const boost::shared_ptr<XdmfSet> set)
{
	mSets.push_back(set);
}

void XdmfGrid::populateItem(const std::map<std::string, std::string> & itemProperties, std::vector<boost::shared_ptr<XdmfItem> > & childItems)
{
	std::map<std::string, std::string>::const_iterator name = itemProperties.find("Name");
	if(name != itemProperties.end())
	{
		mName = name->second;
	}
	else
	{
		mName = "";
	}
	for(std::vector<boost::shared_ptr<XdmfItem> >::const_iterator iter = childItems.begin(); iter != childItems.end(); ++iter)
	{
		if(boost::shared_ptr<XdmfAttribute> attribute = boost::shared_dynamic_cast<XdmfAttribute>(*iter))
		{
			this->insert(attribute);
		}
		else if(boost::shared_ptr<XdmfGeometry> geometry = boost::shared_dynamic_cast<XdmfGeometry>(*iter))
		{
			mGeometry = geometry;
		}
		else if(boost::shared_ptr<XdmfSet> set = boost::shared_dynamic_cast<XdmfSet>(*iter))
		{
			this->insert(set);
		}
		else if(boost::shared_ptr<XdmfTime> time = boost::shared_dynamic_cast<XdmfTime>(*iter))
		{
			mTime = time;
		}
		else if(boost::shared_ptr<XdmfTopology> topology = boost::shared_dynamic_cast<XdmfTopology>(*iter))
		{
			mTopology = topology;
		}
	}
}

void XdmfGrid::removeAttribute(const unsigned int index)
{
	if(index >= mAttributes.size())
	{
		assert(false);
	}
	mAttributes.erase(mAttributes.begin() + index);
}

void XdmfGrid::removeAttribute(const std::string & name)
{
	for(std::vector<boost::shared_ptr<XdmfAttribute> >::iterator iter = mAttributes.begin(); iter != mAttributes.end(); ++iter)
	{
		if((*iter)->getName().compare(name) == 0)
		{
			mAttributes.erase(iter);
			return;
		}
	}
	return;
}

void XdmfGrid::removeSet(const unsigned int index)
{
	if(index >= mSets.size())
	{
		assert(false);
	}
	mSets.erase(mSets.begin() + index);
}

void XdmfGrid::removeSet(const std::string & name)
{
	for(std::vector<boost::shared_ptr<XdmfSet> >::iterator iter = mSets.begin(); iter != mSets.end(); ++iter)
	{
		if((*iter)->getName().compare(name) == 0)
		{
			mSets.erase(iter);
			return;
		}
	}
	return;
}

void XdmfGrid::setGeometry(const boost::shared_ptr<XdmfGeometry> geometry)
{
	mGeometry = geometry;
}

void XdmfGrid::setName(const std::string & name)
{
	mName = name;
}

void XdmfGrid::setTime(const boost::shared_ptr<XdmfTime> time)
{
	mTime = time;
}

void XdmfGrid::setTopology(const boost::shared_ptr<XdmfTopology> topology)
{
	mTopology = topology;
}

void XdmfGrid::traverse(const boost::shared_ptr<XdmfBaseVisitor> visitor) const
{
	if(mTime != NULL)
	{
		mTime->accept(visitor);
	}
	mGeometry->accept(visitor);
	mTopology->accept(visitor);
	for(std::vector<boost::shared_ptr<XdmfAttribute> >::const_iterator iter = mAttributes.begin(); iter != mAttributes.end(); ++iter)
	{
		(*iter)->accept(visitor);
	}
	for(std::vector<boost::shared_ptr<XdmfSet> >::const_iterator iter = mSets.begin(); iter != mSets.end(); ++iter)
	{
		(*iter)->accept(visitor);
	}
}
