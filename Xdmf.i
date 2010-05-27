/*
XdmfPython.cpp:
swig -v -c++ -python -o XdmfPython.cpp Xdmf.i
*/

%module Xdmf
%{
	#include <XdmfArray.hpp>
	#include <XdmfAttribute.hpp>
	#include <XdmfAttributeCenter.hpp>
	#include <XdmfAttributeType.hpp>
	#include <XdmfDataItem.hpp>
	#include <XdmfDomain.hpp>
	#include <XdmfGeometry.hpp>
	#include <XdmfGeometryType.hpp>
	#include <XdmfGrid.hpp>
	#include <XdmfGridCollection.hpp>
	#include <XdmfGridCollectionType.hpp>
	#include <XdmfHDF5Controller.hpp>
	#include <XdmfHDF5Writer.hpp>
	#include <XdmfItem.hpp>
	#include <XdmfItemProperty.hpp>
	#include <XdmfObject.hpp>
	#include <XdmfReader.hpp>
	#include <XdmfSet.hpp>
	#include <XdmfSetType.hpp>
	#include <XdmfTopology.hpp>
	#include <XdmfTopologyType.hpp>
	#include <XdmfVisitor.hpp>
	#include <XdmfWriter.hpp>
%}

namespace boost {
	template<class T> class shared_ptr
	{
	public:
		T * operator-> () const;
	};
}

// Macro for defining shared pointer templates:
%define SWIG_SHARED_PTR_TEMPLATE(CLASS...)
%template(CLASS ## _Ptr) boost::shared_ptr< CLASS >;
%template(CLASS ## _ConstPtr) boost::shared_ptr< const CLASS >;
%enddef

// Macro for defining shared pointer inheritances:
%define SWIG_SHARED_PTR_DERIVED(CLASS, BASECLASS...)
%types(boost::shared_ptr< CLASS > = boost::shared_ptr< BASECLASS > ) %{
  boost::shared_ptr< BASECLASS > p = *(boost::shared_ptr< CLASS >*)$from;
  boost::shared_ptr< BASECLASS > * pPtr = &p;
  return pPtr;
%}
%enddef

%include std_string.i
%include std_vector.i
%include loki/Visitor.h

// Shared Pointer Templates
SWIG_SHARED_PTR_TEMPLATE(XdmfArray);
SWIG_SHARED_PTR_TEMPLATE(XdmfAttribute);
SWIG_SHARED_PTR_TEMPLATE(XdmfAttributeCenter);
SWIG_SHARED_PTR_TEMPLATE(XdmfAttributeType);
SWIG_SHARED_PTR_TEMPLATE(XdmfBaseVisitor);
SWIG_SHARED_PTR_TEMPLATE(XdmfDataItem);
SWIG_SHARED_PTR_TEMPLATE(XdmfDomain);
SWIG_SHARED_PTR_TEMPLATE(XdmfGeometry);
SWIG_SHARED_PTR_TEMPLATE(XdmfGeometryType);
SWIG_SHARED_PTR_TEMPLATE(XdmfGrid);
SWIG_SHARED_PTR_TEMPLATE(XdmfGridCollection);
SWIG_SHARED_PTR_TEMPLATE(XdmfGridCollectionType);
SWIG_SHARED_PTR_TEMPLATE(XdmfHDF5Controller);
SWIG_SHARED_PTR_TEMPLATE(XdmfHDF5Writer);
SWIG_SHARED_PTR_TEMPLATE(XdmfItem);
SWIG_SHARED_PTR_TEMPLATE(XdmfItemProperty);
SWIG_SHARED_PTR_TEMPLATE(XdmfObject);
SWIG_SHARED_PTR_TEMPLATE(XdmfReader);
SWIG_SHARED_PTR_TEMPLATE(XdmfSet);
SWIG_SHARED_PTR_TEMPLATE(XdmfSetType);
SWIG_SHARED_PTR_TEMPLATE(XdmfTopology);
SWIG_SHARED_PTR_TEMPLATE(XdmfTopologyType);
SWIG_SHARED_PTR_TEMPLATE(XdmfVisitor);
SWIG_SHARED_PTR_TEMPLATE(XdmfWriter);

// Abstract Base Classes
%template() Loki::BaseVisitable<void>;
%template() Loki::Visitor<XdmfArray>;
%template() Loki::Visitor<XdmfAttribute>;
%template() Loki::Visitor<XdmfDomain>;
%template() Loki::Visitor<XdmfGeometry>;
%template() Loki::Visitor<XdmfGrid>;
%template() Loki::Visitor<XdmfItem>;
%template() Loki::Visitor<XdmfTopology>;

SWIG_SHARED_PTR_DERIVED(XdmfArray, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfArray, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfAttribute, XdmfDataItem);
SWIG_SHARED_PTR_DERIVED(XdmfAttribute, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfAttribute, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfAttributeCenter, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfAttributeCenter, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfAttributeType, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfAttributeType, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfDataItem, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfDataItem, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfDomain, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfDomain, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfGeometry, XdmfDataItem);
SWIG_SHARED_PTR_DERIVED(XdmfGeometry, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfGeometry, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfGeometryType, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfGeometryType, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfGrid, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfGrid, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfGridCollection, XdmfGrid);
SWIG_SHARED_PTR_DERIVED(XdmfGridCollection, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfGridCollection, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfGridCollectionType, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfGridCollectionType, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfHDF5Controller, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfHDF5Writer, XdmfVisitor);
SWIG_SHARED_PTR_DERIVED(XdmfHDF5Writer, Loki::BaseVisitor);
SWIG_SHARED_PTR_DERIVED(XdmfHDF5Writer, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfItem, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfItemProperty, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfReader, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfSet, XdmfDataItem);
SWIG_SHARED_PTR_DERIVED(XdmfSet, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfSet, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfSetType, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfSetType, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfTopology, XdmfDataItem);
SWIG_SHARED_PTR_DERIVED(XdmfTopology, XdmfItem);
SWIG_SHARED_PTR_DERIVED(XdmfTopology, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfTopologyType, XdmfItemProperty)
SWIG_SHARED_PTR_DERIVED(XdmfTopologyType, XdmfObject)
SWIG_SHARED_PTR_DERIVED(XdmfVisitor, XdmfBaseVisitor);
SWIG_SHARED_PTR_DERIVED(XdmfVisitor, XdmfObject);
SWIG_SHARED_PTR_DERIVED(XdmfWriter, XdmfVisitor);
SWIG_SHARED_PTR_DERIVED(XdmfWriter, XdmfBaseVisitor);
SWIG_SHARED_PTR_DERIVED(XdmfWriter, XdmfObject);

%include XdmfObject.hpp

%include XdmfItem.hpp
%include XdmfDataItem.hpp
%include XdmfItemProperty.hpp
%include XdmfVisitor.hpp

%include XdmfHDF5Controller.hpp
%include XdmfHDF5Writer.hpp
%include XdmfReader.hpp
%include XdmfWriter.hpp

%include XdmfAttribute.hpp
%include XdmfAttributeCenter.hpp
%include XdmfAttributeType.hpp
%include XdmfArray.hpp
%include XdmfDomain.hpp
%include XdmfGeometry.hpp
%include XdmfGeometryType.hpp
%include XdmfGrid.hpp
%include XdmfSet.hpp
%include XdmfSetType.hpp
%include XdmfTopology.hpp
%include XdmfTopologyType.hpp

%include XdmfGridCollection.hpp
%include XdmfGridCollectionType.hpp

// Provide accessors from python lists to XdmfArrays
%extend XdmfArray {
	void copyValueAsChar(int index, char value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsShort(int index, short value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsInt(int index, int value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsLong(int index, long value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsFloat(int index, float value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsDouble(int index, double value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUChar(int index, unsigned char value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUShort(int index, unsigned short value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUInt(int index, unsigned int value) {
		$self->copyValues(index, &value);
	}
};

%extend boost::shared_ptr<XdmfArray> {
	%pythoncode {
		def copyValuesAsChar(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsChar(i+startIndex, values[i])

		def copyValuesAsShort(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsShort(i+startIndex, values[i])

		def copyValuesAsInt(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsInt(i+startIndex, values[i])

		def copyValuesAsLong(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsLong(i+startIndex, values[i])

		def copyValuesAsFloat(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsFloat(i+startIndex, values[i])

		def copyValuesAsDouble(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsDouble(i+startIndex, values[i])

		def copyValuesAsUChar(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUChar(i+startIndex, values[i])

		def copyValuesAsUShort(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUShort(i+startIndex, values[i])

		def copyValuesAsUInt(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUInt(i+startIndex, values[i])
	};
};
