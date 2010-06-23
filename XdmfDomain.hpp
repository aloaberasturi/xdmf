#ifndef XDMFDOMAIN_HPP_
#define XDMFDOMAIN_HPP_

// Forward Declarations
class XdmfGrid;

// Includes
#include "XdmfItem.hpp"

/**
 * @brief The root XdmfItem that contains XdmfGrids.
 *
 * XdmfDomain is the top XdmfItem in an Xdmf structure.  It can store a number of grids and
 * provides methods to insert, retrieve, and remove these grids.
 */
class XdmfDomain : public XdmfItem {

public:

	XdmfNewMacro(XdmfDomain);
	virtual ~XdmfDomain();

	LOKI_DEFINE_VISITABLE(XdmfDomain, XdmfItem)
	static std::string ItemTag;

	/**
	 * Get a XdmfGrid attached to the domain.
	 *
	 * @param index an unsigned int of the grid to retrieve.
	 * @return smart pointer to the grid at the specified index attached to this domain.
	 */
	boost::shared_ptr<XdmfGrid> getGrid(const unsigned int index);

	/**
	 * Get a XdmfGrid attached to this domain (const version).
	 *
	 * @param index of the grid to retrieve.
	 * @return pointer to the grid attached to this domain.
	 */
	boost::shared_ptr<const XdmfGrid> getGrid(const unsigned int index) const;

	std::map<std::string, std::string> getItemProperties() const;

	std::string getItemTag() const;

	/**
	 * Get the number of XdmfGrids attached to this domain.
	 *
	 * @return unsigned int containing the number of XdmfGrids attached to this domain.
	 */
	unsigned int getNumberOfGrids() const;

	/**
	 * Insert a XdmfGrid into the domain.
	 *
	 * @param grid a shared pointer to an XdmfGrid to insert into the domain.
	 */
	void insert(const boost::shared_ptr<XdmfGrid> grid);

	/**
	 * Remove an XdmfGrid from the domain.
	 *
	 * @param index of the XdmfGrid to remove.
	 */
	void removeGrid(const unsigned int index);

	virtual void traverse(const boost::shared_ptr<XdmfBaseVisitor> visitor) const;

protected:

	XdmfDomain();
	virtual void populateItem(const std::map<std::string, std::string> & itemProperties, std::vector<boost::shared_ptr<XdmfItem> > & childItems);

private:

	XdmfDomain(const XdmfDomain & domain);  // Not implemented.
	void operator=(const XdmfDomain & domain);  // Not implemented.

	std::vector<boost::shared_ptr<XdmfGrid> > mGrids;
};

#endif /* XDMFDOMAIN_HPP_ */
