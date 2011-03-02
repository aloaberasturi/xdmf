/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5Writer.cpp                                                  */
/*                                                                           */
/*  Author:                                                                  */
/*     Kenneth Leiter                                                        */
/*     kenneth.leiter@arl.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2011 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

#include <hdf5.h>
#include <sstream>
#include "XdmfArray.hpp"
#include "XdmfArrayType.hpp"
#include "XdmfHDF5Controller.hpp"
#include "XdmfHDF5Writer.hpp"

boost::shared_ptr<XdmfHDF5Writer>
XdmfHDF5Writer::New(const std::string & filePath)
{
  boost::shared_ptr<XdmfHDF5Writer> p(new XdmfHDF5Writer(filePath));
  return p;
}

XdmfHDF5Writer::XdmfHDF5Writer(const std::string & filePath) :
  XdmfHeavyDataWriter(filePath)
{
}

XdmfHDF5Writer::~XdmfHDF5Writer()
{
}

boost::shared_ptr<XdmfHDF5Controller>
XdmfHDF5Writer::createHDF5Controller(const std::string & hdf5FilePath,
                                     const std::string & dataSetPath,
                                     const boost::shared_ptr<const XdmfArrayType> type,
                                     const std::vector<unsigned int> & start,
                                     const std::vector<unsigned int> & stride,
                                     const std::vector<unsigned int> & count)
{
  return XdmfHDF5Controller::New(hdf5FilePath,
                                 dataSetPath,
                                 type,
                                 start,
                                 stride,
                                 count);
}

void
XdmfHDF5Writer::visit(XdmfArray & array,
                      const boost::shared_ptr<XdmfBaseVisitor> visitor)
{
  this->write(array, H5P_DEFAULT);
}

void
XdmfHDF5Writer::write(XdmfArray & array,
                      const int fapl)
{
  hid_t datatype = -1;

  if(array.isInitialized()) {
    if(array.getArrayType() == XdmfArrayType::Int8()) {
      datatype = H5T_NATIVE_CHAR;
    }
    else if(array.getArrayType() == XdmfArrayType::Int16()) {
      datatype = H5T_NATIVE_SHORT;
    }
    else if(array.getArrayType() == XdmfArrayType::Int32()) {
      datatype = H5T_NATIVE_INT;
    }
    else if(array.getArrayType() == XdmfArrayType::Int64()) {
      datatype = H5T_NATIVE_LONG;
    }
    else if(array.getArrayType() == XdmfArrayType::Float32()) {
      datatype = H5T_NATIVE_FLOAT;
    }
    else if(array.getArrayType() == XdmfArrayType::Float64()) {
      datatype = H5T_NATIVE_DOUBLE;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt8()) {
      datatype = H5T_NATIVE_UCHAR;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt16()) {
      datatype = H5T_NATIVE_USHORT;
    }
    else if(array.getArrayType() == XdmfArrayType::UInt32()) {
      datatype = H5T_NATIVE_UINT;
    }
    else {
      assert(false);
    }
  }

  if(datatype != -1) {
    std::string hdf5FilePath = mFilePath;
    std::stringstream dataSetPath;

    if((mMode == Overwrite || mMode == Append)
       && array.getHeavyDataController()) {
      // Write to the previous dataset
      dataSetPath << array.getHeavyDataController()->getDataSetPath();
      hdf5FilePath = array.getHeavyDataController()->getFilePath();
    }
    else {
      dataSetPath << "Data" << mDataSetId;
    }

    // Open a hdf5 dataset and write to it on disk.
    herr_t status;
    hsize_t size = array.getSize();
    hid_t hdf5Handle;

    // Save old error handler and turn off error handling for now
    H5E_auto_t old_func;
    void * old_client_data;
    H5Eget_auto(0, &old_func, &old_client_data);
    H5Eset_auto2(0, NULL, NULL);

    if(H5Fis_hdf5(hdf5FilePath.c_str()) > 0) {
      hdf5Handle = H5Fopen(hdf5FilePath.c_str(), H5F_ACC_RDWR, fapl);
    }
    else {
      hdf5Handle = H5Fcreate(hdf5FilePath.c_str(),
                             H5F_ACC_TRUNC,
                             H5P_DEFAULT,
                             fapl);
    }
    hid_t dataset = H5Dopen(hdf5Handle,
                            dataSetPath.str().c_str(),
                            H5P_DEFAULT);

    hid_t dataspace = H5S_ALL;
    hid_t memspace = H5S_ALL;

    if(dataset < 0) {
      hsize_t unlimited = H5S_UNLIMITED;
      memspace = H5Screate_simple(1, &size, &unlimited);
      hid_t property = H5Pcreate(H5P_DATASET_CREATE);
      hsize_t chunkSize = 1024;
      status = H5Pset_chunk(property, 1, &chunkSize);
      dataset = H5Dcreate(hdf5Handle,
                          dataSetPath.str().c_str(),
                          datatype,
                          memspace,
                          H5P_DEFAULT,
                          property,
                          H5P_DEFAULT);
      status = H5Pclose(property);
    }
    else {
      // Need to resize dataset to fit new data
      if(mMode == Append) {
        // Get size of old dataset
        dataspace = H5Dget_space(dataset);
        hssize_t datasize = H5Sget_simple_extent_npoints(dataspace);
        status = H5Sclose(dataspace);

        // Resize to fit size of old and new data.
        hsize_t newSize = size + datasize;
        status = H5Dset_extent(dataset, &newSize);

        // Select hyperslab to write to.
        memspace = H5Screate_simple(1, &size, NULL);
        dataspace = H5Dget_space(dataset);
        hsize_t start = datasize;
        status = H5Sselect_hyperslab(dataspace,
                                     H5S_SELECT_SET,
                                     &start,
                                     NULL,
                                     &size,
                                     NULL) ;
      }
      else {
        status = H5Dset_extent(dataset, &size);
      }
    }
    status = H5Dwrite(dataset,
                      datatype,
                      memspace,
                      dataspace,
                      H5P_DEFAULT,
                      array.getValuesInternal());
    if(dataspace != H5S_ALL) {
      status = H5Sclose(dataspace);
    }
    if(memspace != H5S_ALL) {
      status = H5Sclose(memspace);
    }
    status = H5Dclose(dataset);
    status = H5Fclose(hdf5Handle);

    // Restore previous error handler
    H5Eset_auto2(0, old_func, old_client_data);

    // Attach a new controller to the array
    unsigned int newSize = array.getSize();
    if(mMode == Append && array.getHeavyDataController()) {
      newSize = newSize + array.getHeavyDataController()->getSize();
    }

    if(mMode == Default || !array.getHeavyDataController()) {
      ++mDataSetId;
    }

    const boost::shared_ptr<XdmfHDF5Controller> newDataController =
      this->createHDF5Controller(hdf5FilePath,
                                 dataSetPath.str(),
                                 array.getArrayType(),
                                 std::vector<unsigned int>(1, 0),
                                 std::vector<unsigned int>(1, 1),
                                 std::vector<unsigned int>(1, newSize));
    array.setHeavyDataController(newDataController);
  }
}
