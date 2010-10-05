/*
 * XdmfGeometryType.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: kleiter
 */

#include "XdmfGeometryType.hpp"

// Supported XdmfGeometryTypes
boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::NoGeometryType()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("None", 0));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::XYZ()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("XYZ", 3));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::XY()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("XY", 2));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::X_Y_Z()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("X_Y_Z", 3));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::X_Y()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("X_Y", 2));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::VXVYVZ()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("VXVYVZ", 3));
	return p;
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::VXVY()
{
	static boost::shared_ptr<const XdmfGeometryType> p(new XdmfGeometryType("VXVY", 2));
	return p;
}

XdmfGeometryType::XdmfGeometryType(const std::string& name, const int& dimensions) :
	mDimensions(dimensions),
	mName(name)
{
}

XdmfGeometryType::~XdmfGeometryType()
{
}

boost::shared_ptr<const XdmfGeometryType> XdmfGeometryType::New(const std::map<std::string, std::string> & itemProperties)
{
	std::map<std::string, std::string>::const_iterator type = itemProperties.find("Type");
	if(type == itemProperties.end())
	{
		type = itemProperties.find("GeometryType");
	}

	if(type != itemProperties.end())
	{
		const std::string typeVal = type->second;
		if(typeVal.compare("None") == 0)
		{
			return NoGeometryType();
		}
		else if(typeVal.compare("XYZ") == 0)
		{
			return XYZ();
		}
		else if(typeVal.compare("XY") == 0)
		{
			return XY();
		}
		else if(typeVal.compare("X_Y_Z") == 0)
		{
			return X_Y_Z();
		}
		else if(typeVal.compare("X_Y") == 0)
		{
			return X_Y();
		}
		else if(typeVal.compare("VXVYVZ") == 0)
		{
			return VXVYVZ();
		}
		else if(typeVal.compare("VXVY") == 0)
		{
			return VXVY();
		}
		else
		{
			assert(false);
		}
	}
	assert(false);
}

unsigned int XdmfGeometryType::getDimensions() const
{
	return mDimensions;
}

std::string XdmfGeometryType::getName() const
{
	return mName;
}

void XdmfGeometryType::getProperties(std::map<std::string, std::string> & collectedProperties) const
{
	collectedProperties["Type"] = mName;
}
