#ifndef XDMFGRID_HPP_
#define XDMFGRID_HPP_

// Forward Declarations
class XdmfAttribute;
class XdmfGeometry;
class XdmfMap;
class XdmfSet;
class XdmfTime;
class XdmfTopology;

// Includes
#include "XdmfItem.hpp"

/**
 * @brief A mesh containing elements, points, and fields attached to the mesh.
 *
 * XdmfGrid represents a mesh.  It is required to contain two other Xdmf data structures, an XdmfGeometry
 * that stores point locations and an XdmfTopology that store connectivity information.  XdmfAttributes can be inserted
 * into the XdmfGrid to specify fields centered on various parts of the mesh.  XdmfSets can be inserted into XdmfGrids
 * to specify collections of mesh elements.
 */
class XdmfGrid : public virtual XdmfItem {

public:

	/**
	 * Create a new XdmfGrid.
	 *
	 * @return constructed XdmfGrid.
	 */
	static boost::shared_ptr<XdmfGrid> New();

	virtual ~XdmfGrid();

	LOKI_DEFINE_VISITABLE(XdmfGrid, XdmfItem)
	XDMF_CHILDREN(XdmfAttribute, Attribute, Name)
	XDMF_CHILDREN(XdmfSet, Set, Name)
	static const std::string ItemTag;

	/**
	 * Get the geometry associated with this grid.
	 *
	 * @return the geometry associated with this grid.
	 */
	boost::shared_ptr<XdmfGeometry> getGeometry();

	/**
	 * Get the geometry associated with this grid (const version).
	 *
	 * @return the geometry associated with this grid.
	 */
	boost::shared_ptr<const XdmfGeometry> getGeometry() const;

	std::map<std::string, std::string> getItemProperties() const;

	virtual std::string getItemTag() const;

	/**
	 * Get the boundary communicator map associated with this grid.
	 *
	 * @return the boundary communicator map associated with this grid.
	 */
	boost::shared_ptr<XdmfMap > getMap();

	/**
	 * Get the boundary communicator map associated with this grid (const version).
	 *
	 * @return the boundary communicator map associated with this grid.
	 */
	boost::shared_ptr<const XdmfMap> getMap() const;

	/**
	 * Get the name of the grid.
	 *
	 * @return the name of the grid.
	 */
	std::string getName() const;

	/**
	 * Get the time associated with this grid.
	 *
	 * @return pointer to the XdmfTime attached to this grid.  If no XdmfTime is attached, return a NULL pointer.
	 */
	boost::shared_ptr<XdmfTime> getTime();

	/**
	 * Get the time associated with this grid (const version).
	 *
	 * @return pointer to the XdmfTime attached to this grid.  If no XdmfTime is attached, return a NULL pointer.
	 */
	boost::shared_ptr<const XdmfTime> getTime() const;

	/**
	 * Get the topology associated with this grid.
	 *
	 * @return the topology associated with this grid.
	 */
	boost::shared_ptr<XdmfTopology> getTopology();

	/**
	 * Get the topology associated with this grid (const version).
	 *
	 * @return the topology associated with this grid.
	 */
	boost::shared_ptr<const XdmfTopology> getTopology() const;

	using XdmfItem::insert;

	/**
	 * Set the geometry associated with this grid.
	 *
	 * @param geometry an XdmfGeometry to associate with this grid.
	 */
	void setGeometry(const boost::shared_ptr<XdmfGeometry> geometry);

	/**
	 * Set the boundary communicator map associated with this grid.
	 *
	 * @param map a XdmfMap to associate with this grid.
	 */
	void setMap(boost::shared_ptr<XdmfMap> map);

	/**
	 * Set the name of the grid.
	 *
	 * @param name of the grid to set.
	 */
	void setName(const std::string & name);

	/**
	 * Set the time associated with this grid.
	 *
	 * @param time an XdmfTime to associate with this grid.
	 */
	void setTime(const boost::shared_ptr<XdmfTime> time);

	/**
	 * Set the topology associated with this grid.
	 *
	 * @param topology an XdmfTopology to associate with this grid.
	 */
	void setTopology(const boost::shared_ptr<XdmfTopology> topology);

	virtual void traverse(const boost::shared_ptr<XdmfBaseVisitor> visitor);

protected:

	XdmfGrid();
	virtual void populateItem(const std::map<std::string, std::string> & itemProperties, std::vector<boost::shared_ptr<XdmfItem> > & childItems, const XdmfCoreReader * const reader);

	std::string mName;

private:

	XdmfGrid(const XdmfGrid & grid);  // Not implemented.
	void operator=(const XdmfGrid & grid);  // Not implemented.

	boost::shared_ptr<XdmfGeometry> mGeometry;
	boost::shared_ptr<XdmfMap> mMap;
	boost::shared_ptr<XdmfTime> mTime;
	boost::shared_ptr<XdmfTopology> mTopology;
};

#endif /* XDMFGRID_HPP_ */
