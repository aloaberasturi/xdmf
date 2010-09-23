/*
 * XdmfSetType.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: kleiter
 */

#include "XdmfSetType.hpp"

// Supported XdmfSetTypes
boost::shared_ptr<const XdmfSetType> XdmfSetType::NoSetType()
{
	static boost::shared_ptr<const XdmfSetType> p(new XdmfSetType("None"));
	return p;
}

boost::shared_ptr<const XdmfSetType> XdmfSetType::Node()
{
	static boost::shared_ptr<const XdmfSetType> p(new XdmfSetType("Node"));
	return p;
}

boost::shared_ptr<const XdmfSetType> XdmfSetType::Cell()
{
	static boost::shared_ptr<const XdmfSetType> p(new XdmfSetType("Cell"));
	return p;
}

boost::shared_ptr<const XdmfSetType> XdmfSetType::Face()
{
	static boost::shared_ptr<const XdmfSetType> p(new XdmfSetType("Face"));
	return p;
}

boost::shared_ptr<const XdmfSetType> XdmfSetType::Edge()
{
	static boost::shared_ptr<const XdmfSetType> p(new XdmfSetType("Edge"));
	return p;
}

XdmfSetType::XdmfSetType(const std::string & name) :
	mName(name)
{
}

XdmfSetType::~XdmfSetType()
{
}

boost::shared_ptr<const XdmfSetType> XdmfSetType::New(const std::map<std::string, std::string> & itemProperties)
{
	std::map<std::string, std::string>::const_iterator type = itemProperties.find("Type");
	if(type == itemProperties.end())
	{
		type = itemProperties.find("SetType");
	}
	if(type != itemProperties.end())
	{
		const std::string typeVal = type->second;
		if(typeVal.compare("None") == 0)
		{
			return NoSetType();
		}
		else if(typeVal.compare("Node") == 0)
		{
			return Node();
		}
		else if(typeVal.compare("Cell") == 0)
		{
			return Cell();
		}
		else if(typeVal.compare("Face") == 0)
		{
			return Face();
		}
		else if(typeVal.compare("Edge") == 0)
		{
			return Edge();
		}
		else
		{
			assert(false);
		}
	}
	assert(false);
}

bool XdmfSetType::operator==(const XdmfSetType & setType) const
{
	return mName.compare(setType.mName) == 0;
}

bool XdmfSetType::operator!=(const XdmfSetType & setType) const
{
	return !this->operator==(setType);
}

bool XdmfSetType::IsEqual(boost::shared_ptr<XdmfSetType> setType)
{
        if(setType == NULL) return false;
        if(this == setType.get()
           &&   mName == setType->mName
        ) return true;
        return false;
}


void XdmfSetType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
	collectedProperties["Type"] = this->mName;
}
