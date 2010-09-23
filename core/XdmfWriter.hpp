#ifndef XDMFWRITER_HPP_
#define XDMFWRITER_HPP_

// Forward Declarations
class XdmfArray;
class XdmfHDF5Writer;

// Includes
#include "XdmfVisitor.hpp"

/**
 * @brief Traverse the Xdmf graph and write light and heavy data stored to disk.
 *
 * XdmfWriter visits each node of an Xdmf graph structure and writes data to disk.  Writing begins by calling the
 * accept() operation on any XdmfItem and supplying this writer as the parameter.  The XdmfItem as well as all children
 * attached to the XdmfItem are written to disk.  Heavy data is written to HDF5 format using the XdmfHDF5Writer and light
 * data is written to XML.
 *
 * By default, the XdmfWriter writes all heavy data to a single heavy data file specified by the XdmfHDF5Writer.
 * If a dataset is encountered that resides in a different heavy data file on disk, the dataset is read from disk and written
 * to the new hdf5 file.  If this is undesired, the XdmfWriter can be set to DistributedHeavyData mode in which the writer
 * will automatically reference any hdf5 dataset even if it resides in a different file than the one currently being written to.
 */
class XdmfWriter : public XdmfVisitor,
	public Loki::Visitor<XdmfArray> {

public:

	enum Mode {
		Default, DistributedHeavyData
	};

	/**
	 * Create a new XdmfWriter to write Xdmf data to disk.  This will create its own hdf5 writer based on the xmlFileName.
	 * For example, if supplied "output.xmf" the created hdf5 writer would write to file "output.h5".
	 *
	 * @param xmlFilePath the path to the xml file to write to.
	 * @return the new XdmfWriter.
	 */
	static boost::shared_ptr<XdmfWriter> New(const std::string & xmlFilePath);

	/**
	 * Create a new XdmfWriter to write Xdmf data to disk.  This will utilize the supplied hdf5Writer to write any
	 * heavy data to disk.
	 *
	 * @param xmlFilePath the path to the xml file to write to.
	 * @param hdf5Writer the heavy data writer to use when writing.
	 * @return the new XdmfWriter.
	 */
	static boost::shared_ptr<XdmfWriter> New(const std::string & xmlFilePath, const boost::shared_ptr<XdmfHDF5Writer> hdf5Writer);

	virtual ~XdmfWriter();

	/**
	 * Get the absolute path to the XML file on disk this writer is writing to.
	 *
	 * @return a std::string containing the path to the XML file on disk this writer is writing to.
	 */
	std::string getFilePath() const;

	/**
	 * Get the hdf5 writer that this XdmfWriter uses to write heavy data to disk.
	 *
	 * @return the requested hdf5 writer.
	 */
	boost::shared_ptr<XdmfHDF5Writer> getHDF5Writer();

	/**
	 * Get the hdf5 writer that this XdmfWriter uses to write heavy data to disk (const version).
	 *
	 * @return the requested hdf5 writer.
	 */
	boost::shared_ptr<const XdmfHDF5Writer> getHDF5Writer() const;

	/**
	 * Get the number of values that this writer writes to light data (XML) before switching to a heavy data format.
	 *
	 * @return an unsigned int containing the number of values.
	 */
	unsigned int getLightDataLimit() const;

	/**
	 * Get whether this writer is set to write xpaths.
	 *
	 * @return bool whether this writer is set to write xpaths.
	 */
	bool getWriteXPaths() const;

	/**
	 * Get the Mode of operation for this writer.
	 *
	 * @return the Mode of operation for this writer.
	 */
	Mode getMode() const;

	/**
	 * Set the number of values that this writer writes to light data (XML) before switching to a heavy data format.
	 *
	 * @param numValues an unsigned int containing the number of values.
	 */
	void setLightDataLimit(const unsigned int numValues);

	/**
	 * Set the mode of operation for this writer.
	 *
	 * @param mode the Mode of operation for this writer.
	 */
	void setMode(const Mode mode);

	/**
	 * Set XML document title
	 *
	 * @param title, title to use for this XML document
	 */
	void setDocumentTitle(const std::string title);

	/**
	 * Set version String
	 *
	 * @param version, string to use as version attribute in document title
	 */
	void setVersionString(const std::string version);

	/**
	 * Set whether to write xpaths for this writer.
	 *
	 * @param writeXPaths whether to write xpaths for this writer.
	 */
	void setWriteXPaths(const bool writeXPaths = true);

	/**
	 * Write an XdmfArray to disk
	 *
	 * @param array an XdmfArray to write to disk.
	 * @param visitor a smart pointer to this visitor --- aids in grid traversal.
	 */
	virtual void visit(XdmfArray & array, const boost::shared_ptr<XdmfBaseVisitor> visitor);

	/**
	 * Write an XdmfItem to disk
	 *
	 * @param item an XdmfItem to write to disk.
	 * @param visitor a smart pointer to this visitor --- aids in grid traversal.
	 */
	virtual void visit(XdmfItem & item, const boost::shared_ptr<XdmfBaseVisitor> visitor);


protected:

	XdmfWriter(const std::string & xmlFilePath);
	XdmfWriter(const std::string & xmlFilePath, boost::shared_ptr<XdmfHDF5Writer> hdf5Writer);

private:

	/**
	 * PIMPL
	 */
	class XdmfWriterImpl;

	XdmfWriter(const XdmfWriter & coreWriter);  // Not implemented.
	void operator=(const XdmfWriter & coreWriter);  // Not implemented.

	XdmfWriterImpl * mImpl;
};

#endif /* XDMFWRITER_HPP_ */
