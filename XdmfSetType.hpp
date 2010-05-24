#ifndef XDMFSETTYPE_HPP_
#define XDMFSETTYPE_HPP_

// Includes
#include <string>
#include "XdmfItemProperty.hpp"

/**
 * @brief Property describing the type of an XdmfSet.
 *
 * An XdmfSet consist of a collection of nodes, cells, faces, or edges that are part of an XdmfGrid.  This
 * property indicates which element type the set contains.
 *
 * Xdmf supports the following set types:
 * 	NoSetType
 * 	Node
 * 	Cell
 * 	Face
 * 	Edge
 */
class XdmfSetType : public XdmfItemProperty {

public:

	friend class XdmfSet;

	// Supported Xdmf Set Types
	static XdmfSetType NoSetType();
	static XdmfSetType Node();
	static XdmfSetType Cell();
	static XdmfSetType Face();
	static XdmfSetType Edge();

	void getProperties(std::map<std::string, std::string> & collectedProperties) const;

	/*
	 * Compare two XdmfSetTypes for equality.
	 *
	 * @param setType a XdmfSetType to compare equality to.
	 * @return true if the XdmfSetTypes are equal.
	 */
	bool operator==(const XdmfSetType & setType) const;

	/**
	 * Compare two XdmfSetTypes for inequality.
	 *
	 * @param setType a XdmfSetType to compare inequality to.
	 * @return true if the XdmfSetTypes are not equal.
	 */
	bool operator!=(const XdmfSetType & setType) const;

	XdmfSetType(const XdmfSetType & setType);
	XdmfSetType & operator=(const XdmfSetType & setType);

protected:

	/**
	 * Protected constructor for XdmfSetType.  The constructor is protected because all set types supported
	 * by Xdmf should be accessed through more specific static methods that construct XdmfSetTypes -
	 * i.e. XdmfSetType::Node().
	 *
	 * @param name a std::string containing the name of the XdmfSetType.
	 */
	XdmfSetType(const std::string & name);

private:

	static XdmfSetType New(const std::map<std::string, std::string> & itemProperties);

	std::string mName;
};

#endif /* XDMFGEOMETRYTYPE_HPP_ */
