/*
XdmfPython.cpp:
swig -v -c++ -python -o XdmfPython.cpp Xdmf.i
*/

%module Xdmf
%{
	#include <XdmfArray.hpp>
	#include <XdmfArrayType.hpp>
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

%include boost_shared_ptr.i
%include std_string.i
%include std_vector.i
%include loki/Visitor.h

// Shared Pointer Templates
%shared_ptr(XdmfArray)
%shared_ptr(XdmfAttribute)
%shared_ptr(XdmfBaseVisitor)
%shared_ptr(XdmfDataItem)
%shared_ptr(XdmfDomain)
%shared_ptr(XdmfGeometry)
%shared_ptr(XdmfGrid)
%shared_ptr(XdmfGridCollection)
%shared_ptr(XdmfHDF5Controller)
%shared_ptr(XdmfHDF5Writer)
%shared_ptr(XdmfItem)
%shared_ptr(XdmfItemProperty)
%shared_ptr(XdmfObject)
%shared_ptr(XdmfReader)
%shared_ptr(XdmfSet)
%shared_ptr(XdmfTopology)
%shared_ptr(XdmfVisitor)
%shared_ptr(XdmfWriter)

%shared_ptr(XdmfArrayType)
%shared_ptr(XdmfAttributeCenter)
%shared_ptr(XdmfAttributeType)
%shared_ptr(XdmfGeometryType)
%shared_ptr(XdmfGridCollectionType)
%shared_ptr(XdmfSetType)
%shared_ptr(XdmfTopologyType)

// Abstract Base Classes
%template() Loki::BaseVisitable<void>;
%template() Loki::Visitor<XdmfArray>;
%template() Loki::Visitor<XdmfAttribute>;
%template() Loki::Visitor<XdmfDomain>;
%template() Loki::Visitor<XdmfGeometry>;
%template() Loki::Visitor<XdmfGrid>;
%template() Loki::Visitor<XdmfItem>;
%template() Loki::Visitor<XdmfTopology>;

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
%include XdmfArrayType.hpp
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

%template(getValueCopyAsInt8) XdmfArray::getValueCopy<char>;
%template(getValueCopyAsInt16) XdmfArray::getValueCopy<short>;
%template(getValueCopyAsInt32) XdmfArray::getValueCopy<int>;
%template(getValueCopyAsInt64) XdmfArray::getValueCopy<long>;
%template(getValueCopyAsFloat32) XdmfArray::getValueCopy<float>;
%template(getValueCopyAsFloat64) XdmfArray::getValueCopy<double>;
%template(getValueCopyAsUInt8) XdmfArray::getValueCopy<unsigned char>;
%template(getValueCopyAsUInt16) XdmfArray::getValueCopy<unsigned short>;
%template(getValueCopyAsUInt32) XdmfArray::getValueCopy<unsigned int>;

%template(pushBackAsInt8) XdmfArray::pushBack<char>;
%template(pushBackAsInt16) XdmfArray::pushBack<short>;
%template(pushBackAsInt32) XdmfArray::pushBack<int>;
%template(pushBackAsInt64) XdmfArray::pushBack<long>;
%template(pushBackAsFloat32) XdmfArray::pushBack<float>;
%template(pushBackAsFloat64) XdmfArray::pushBack<double>;
%template(pushBackAsUInt8) XdmfArray::pushBack<unsigned char>;
%template(pushBackAsUInt16) XdmfArray::pushBack<unsigned short>;
%template(pushBackAsUInt32) XdmfArray::pushBack<unsigned int>;

%template(resizeAsInt8) XdmfArray::resize<char>;
%template(resizeAsInt16) XdmfArray::resize<short>;
%template(resizeAsInt32) XdmfArray::resize<int>;
%template(resizeAsInt64) XdmfArray::resize<long>;
%template(resizeAsFloat32) XdmfArray::resize<float>;
%template(resizeAsFloat64) XdmfArray::resize<double>;
%template(resizeAsUInt8) XdmfArray::resize<unsigned char>;
%template(resizeAsUInt16) XdmfArray::resize<unsigned short>;
%template(resizeAsUInt32) XdmfArray::resize<unsigned int>;

// Provide accessors from python lists to XdmfArrays
%extend XdmfArray {
	void copyValueAsInt8(int index, char value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsInt16(int index, short value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsInt32(int index, int value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsInt64(int index, long value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsFloat32(int index, float value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsFloat64(int index, double value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUInt8(int index, unsigned char value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUInt16(int index, unsigned short value) {
		$self->copyValues(index, &value);
	}

	void copyValueAsUInt32(int index, unsigned int value) {
		$self->copyValues(index, &value);
	}
};

%extend XdmfArray {
	%pythoncode {
		def copyValuesAsInt8(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsInt8(i+startIndex, values[i])

		def copyValuesAsInt16(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsInt16(i+startIndex, values[i])

		def copyValuesAsInt32(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsInt32(i+startIndex, values[i])

		def copyValuesAsInt64(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsInt64(i+startIndex, values[i])

		def copyValuesAsFloat32(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsFloat32(i+startIndex, values[i])

		def copyValuesAsFloat64(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsFloat64(i+startIndex, values[i])

		def copyValuesAsUInt8(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUInt8(i+startIndex, values[i])

		def copyValuesAsUInt16(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUInt16(i+startIndex, values[i])

		def copyValuesAsUInt32(self, startIndex, values):
			for i in range(0, len(values)):
				self.copyValueAsUInt32(i+startIndex, values[i])
	};
};
