#ifndef XDMFARRAYTYPE_HPP_
#define XDMFARRAYTYPE_HPP_

// Includes
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing what types of values an XdmfArray contains.
 *
 * XdmfArrayType is a property used by XdmfArray to specify the types of values stored in an
 * XdmfArray.  A specific XdmfArrayType can be created by calling one of the static methods
 * in the class, i.e. XdmfArrayType::Int32().
 *
 * Xdmf supports the following attribute types:
 * 	Uninitialized
 * 	Int8
 * 	Int16
 * 	Int32
 * 	Int64
 *	Float32
 * 	Float64
 * 	UInt8
 * 	UInt16
 * 	UInt32
 */
class XdmfArrayType : public XdmfItemProperty {

public:

	friend class XdmfArray;
	template <typename T> friend void boost::checked_delete(T * x);

	// Supported XdmfArrayTypes
	static boost::shared_ptr<const XdmfArrayType> Uninitialized();
	static boost::shared_ptr<const XdmfArrayType> Int8();
	static boost::shared_ptr<const XdmfArrayType> Int16();
	static boost::shared_ptr<const XdmfArrayType> Int32();
	static boost::shared_ptr<const XdmfArrayType> Int64();
	static boost::shared_ptr<const XdmfArrayType> Float32();
	static boost::shared_ptr<const XdmfArrayType> Float64();
	static boost::shared_ptr<const XdmfArrayType> UInt8();
	static boost::shared_ptr<const XdmfArrayType> UInt16();
	static boost::shared_ptr<const XdmfArrayType> UInt32();

	void getProperties(std::map<std::string, std::string> & collectedProperties) const;

	/*
	 * Compare two XdmfArrayTypes for equality.
	 *
	 * @param arrayType an XdmfArrayType to compare equality to.
	 * @return true if the XdmfArrayTypes are equal.
	 */
	bool operator==(const XdmfArrayType & arrayType) const;

	/**
	 * Compare two XdmfArrayTypes for inequality.
	 *
	 * @param arrayType an XdmfArrayType to compare inequality to.
	 * @return true if the XdmfArrayTypes are not equal.
	 */
	bool operator!=(const XdmfArrayType & arrayType) const;

protected:

	/**
	 * Protected constructor for XdmfArrayType.  The constructor is protected because all array types supported
	 * by Xdmf should be accessed through more specific static methods that construct XdmfArrayTypes - i.e.
	 * XdmfArrayType::Float64().
	 *
	 * @param name the name of the XdmfArrayType to construct.
	 * @param precision the precision, in bytes, of the XdmfArrayType to construct.
	 */
	XdmfArrayType(const std::string & name, const unsigned int precision);
	~XdmfArrayType();

private:

	XdmfArrayType(const XdmfArrayType & arrayType); // Not implemented.
	void operator=(const XdmfArrayType & arrayType); // Not implemented.

	static boost::shared_ptr<const XdmfArrayType> New(const std::map<std::string, std::string> & itemProperties);

	unsigned int mPrecision;
	std::string mName;
};

#endif /* XDMFARRAYTYPE_HPP_ */
