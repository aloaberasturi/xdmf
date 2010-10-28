#ifndef XDMFDOMAIN_HPP_
#define XDMFDOMAIN_HPP_

// Forward Declarations
class XdmfCurvilinearGrid;
class XdmfGridCollection;
class XdmfRectilinearGrid;
class XdmfRegularGrid;
class XdmfUnstructuredGrid;

// Includes
#include "XdmfItem.hpp"

/**
 * @brief The root XdmfItem that contains XdmfGrids.
 *
 * XdmfDomain is the top XdmfItem in an Xdmf structure.  It can store a number of grids and
 * provides methods to insert, retrieve, and remove these grids.
 */
class XdmfDomain : public virtual XdmfItem {

public:

	/**
	 * Create a new XdmfDomain.
	 *
	 * @return constructed XdmfDomain.
	 */
	static boost::shared_ptr<XdmfDomain> New();
	virtual ~XdmfDomain();

	LOKI_DEFINE_VISITABLE(XdmfDomain, XdmfItem)
	XDMF_CHILDREN(XdmfGridCollection, GridCollection, Name)
	XDMF_CHILDREN(XdmfCurvilinearGrid, CurvilinearGrid, Name)
	XDMF_CHILDREN(XdmfRectilinearGrid, RectilinearGrid, Name)
	XDMF_CHILDREN(XdmfRegularGrid, RegularGrid, Name)
	XDMF_CHILDREN(XdmfUnstructuredGrid, UnstructuredGrid, Name)
	static const std::string ItemTag;

	std::map<std::string, std::string> getItemProperties() const;

	virtual std::string getItemTag() const;

	using XdmfItem::insert;

	virtual void traverse(const boost::shared_ptr<XdmfBaseVisitor> visitor);

protected:

	XdmfDomain();
	virtual void populateItem(const std::map<std::string, std::string> & itemProperties, std::vector<boost::shared_ptr<XdmfItem> > & childItems, const XdmfCoreReader * const reader);

private:

	XdmfDomain(const XdmfDomain & domain);  // Not implemented.
	void operator=(const XdmfDomain & domain);  // Not implemented.

};

#endif /* XDMFDOMAIN_HPP_ */
