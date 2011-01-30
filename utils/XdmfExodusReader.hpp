#ifndef XDMFEXODUSREADER_HPP_
#define XDMFEXODUSREADER_HPP_

// Forward Declarations
class XdmfHeavyDataWriter;
class XdmfUnstructuredGrid;

// Includes
#include <boost/shared_ptr.hpp>
#include <string>

/**
 * @brief Reads an ExodusII file from disk into an Xdmf structure in
 * memory.
 */
class XdmfExodusReader {

public:

  /**
   * Create a new XdmfExodusReader.
   *
   * @return constructed XdmfExodusReader.
   */
  static boost::shared_ptr<XdmfExodusReader> New();

  virtual ~XdmfExodusReader();

  /**
   * Read the contents of an ExodusII file from disk into an Xdmf
   * structure in memory.
   *
   * @param fileName containing the path of the exodus file to read.
   * @param heavyDataWriter an XdmfHeavyDataWriter to write the mesh to. If no
   * heavyDataWriter is specified, all mesh data will remain in memory.
   *
   * @return an unstructured grid containing the mesh stored in the ExodusII
   * file.
   */
  boost::shared_ptr<XdmfUnstructuredGrid>
  read(const std::string & fileName,
       const boost::shared_ptr<XdmfHeavyDataWriter> heavyDataWriter = boost::shared_ptr<XdmfHeavyDataWriter>()) const;

protected:

  XdmfExodusReader();

private:

  XdmfExodusReader(const XdmfExodusReader &);  // Not implemented.
  void operator=(const XdmfExodusReader &);  // Not implemented.

};

#endif /* XDMFEXODUSREADER_HPP_ */
