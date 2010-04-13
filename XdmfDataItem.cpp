/*
 * XdmfDataItem.cpp
 *
 *  Created on: Jan 29, 2010
 *      Author: kleiter
 */

#include "XdmfArray.hpp"
#include "XdmfDataItem.hpp"

XdmfDataItem::XdmfDataItem() :
	mArray(XdmfArray::New())
{
	std::cout << "Created DataItem " << this << std::endl;
}

XdmfDataItem::~XdmfDataItem()
{
	std::cout << "Deleted DataItem " << this << std::endl;
}

std::string XdmfDataItem::printSelf() const
{
	return "XdmfDataItem";
}

void XdmfDataItem::setArray(boost::shared_ptr<XdmfArray> array)
{
	mArray = array;
}

boost::shared_ptr<XdmfArray> XdmfDataItem::getArray()
{
	return mArray;
}

boost::shared_ptr<const XdmfArray> XdmfDataItem::getArray() const
{
	return mArray;
}

void XdmfDataItem::traverse(boost::shared_ptr<XdmfVisitor> visitor) const
{
	mArray->write(visitor);
}
