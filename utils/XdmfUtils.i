/*
XdmfUtilsPython.cpp:
swig -v -c++ -python -o XdmfUtilsPython.cpp XdmfUtils.i
*/

#ifdef XDMF_BUILD_DSM

%module XdmfUtils
%{
    // MPI includes
    #include <mpi.h>

    // XdmfCore Includes
    #include <XdmfArray.hpp>
    #include <XdmfArrayType.hpp>
    #include <XdmfCoreItemFactory.hpp>
    #include <XdmfCoreReader.hpp>
    #include <XdmfError.hpp>
    #include <XdmfHDF5Controller.hpp>
    #include <XdmfHDF5Writer.hpp>
    #include <XdmfHDF5ControllerDSM.hpp>
    #include <XdmfHDF5WriterDSM.hpp>
    #include <XdmfHeavyDataController.hpp>
    #include <XdmfHeavyDataWriter.hpp>
    #include <XdmfInformation.hpp>
    #include <XdmfItem.hpp>
    #include <XdmfItemProperty.hpp>
    #include <XdmfSharedPtr.hpp>
    #include <XdmfSparseMatrix.hpp>
    #include <XdmfSystemUtils.hpp>
    #include <XdmfVisitor.hpp>
    #include <XdmfWriter.hpp>

    // Xdmf Includes
    #include <XdmfAttribute.hpp>
    #include <XdmfAttributeCenter.hpp>
    #include <XdmfAttributeType.hpp>
    #include <XdmfCurvilinearGrid.hpp>
    #include <XdmfDomain.hpp>
    #include <XdmfGeometry.hpp>
    #include <XdmfGeometryType.hpp>
    #include <XdmfGraph.hpp>
    #include <XdmfGrid.hpp>
    #include <XdmfGridCollection.hpp>
    #include <XdmfGridCollectionType.hpp>
    #include <XdmfItemFactory.hpp>
    #include <XdmfMap.hpp>
    #include <XdmfReader.hpp>
    #include <XdmfRectilinearGrid.hpp>
    #include <XdmfRegularGrid.hpp>
    #include <XdmfSet.hpp>
    #include <XdmfSetType.hpp>
    #include <XdmfTime.hpp>
    #include <XdmfTopology.hpp>
    #include <XdmfTopologyType.hpp>
    #include <XdmfUnstructuredGrid.hpp>

    // XdmfUtils Includes
    #include <XdmfUtils.hpp>
    #include <XdmfDiff.hpp>
    #include <XdmfExodusReader.hpp>
    #include <XdmfExodusWriter.hpp>
    #include <XdmfPartitioner.hpp>
    #include <XdmfTopologyConverter.hpp>
%}

#else

%module XdmfUtils
%{
    // XdmfCore Includes
    #include <XdmfArray.hpp>
    #include <XdmfArrayType.hpp>
    #include <XdmfCoreItemFactory.hpp>
    #include <XdmfCoreReader.hpp>
    #include <XdmfError.hpp>
    #include <XdmfHDF5Controller.hpp>
    #include <XdmfHDF5Writer.hpp>
    #include <XdmfHeavyDataController.hpp>
    #include <XdmfHeavyDataWriter.hpp>
    #include <XdmfInformation.hpp>
    #include <XdmfItem.hpp>
    #include <XdmfItemProperty.hpp>
    #include <XdmfSharedPtr.hpp>
    #include <XdmfSystemUtils.hpp>
    #include <XdmfVisitor.hpp>
    #include <XdmfWriter.hpp>

    // Xdmf Includes
    #include <XdmfAttribute.hpp>
    #include <XdmfAttributeCenter.hpp>
    #include <XdmfAttributeType.hpp>
    #include <XdmfCurvilinearGrid.hpp>
    #include <XdmfDomain.hpp>
    #include <XdmfGeometry.hpp>
    #include <XdmfGeometryType.hpp>
    #include <XdmfGrid.hpp>
    #include <XdmfGridCollection.hpp>
    #include <XdmfGridCollectionType.hpp>
    #include <XdmfItemFactory.hpp>
    #include <XdmfMap.hpp>
    #include <XdmfReader.hpp>
    #include <XdmfRectilinearGrid.hpp>
    #include <XdmfRegularGrid.hpp>
    #include <XdmfSet.hpp>
    #include <XdmfSetType.hpp>
    #include <XdmfTime.hpp>
    #include <XdmfTopology.hpp>
    #include <XdmfTopologyType.hpp>
    #include <XdmfUnstructuredGrid.hpp>

    // XdmfUtils Includes
    #include <XdmfUtils.hpp>
    #include <XdmfDiff.hpp>
    #include <XdmfExodusReader.hpp>
    #include <XdmfExodusWriter.hpp>
    #include <XdmfPartitioner.hpp>
    #include <XdmfTopologyConverter.hpp>
%}

#endif

%import Xdmf.i

#ifdef SWIGPYTHON

#ifdef XDMF_BUILD_DSM

%include mpi4py/mpi4py.i

%mpi4py_typemap(Comm, MPI_Comm);

#endif

%pythoncode {
    from Xdmf import *
}

#endif /* SWIGPYTHON */

// Shared Pointer Templates
%shared_ptr(XdmfDiff)
#ifdef XDMF_BUILD_EXODUS_IO
    %shared_ptr(XdmfExodusReader)
    %shared_ptr(XdmfExodusWriter)
#endif
#ifdef XDMF_BUILD_PARTITIONER
    %shared_ptr(XdmfPartitioner)
#endif
%shared_ptr(XdmfTopologyConverter)

%include XdmfUtils.hpp
%include XdmfDiff.hpp
#ifdef XDMF_BUILD_EXODUS_IO
    %include XdmfExodusReader.hpp
    %include XdmfExodusWriter.hpp
#endif
#ifdef XDMF_BUILD_PARTITIONER
    %include XdmfPartitioner.hpp
#endif
%include XdmfTopologyConverter.hpp
