/*
XdmfUtilsPython.cpp:
swig -v -c++ -python -o XdmfUtilsPython.cpp XdmfUtils.i
*/

%module XdmfUtils
%{
	// XdmfCore Includes
	#include <XdmfArray.hpp>
	#include <XdmfArrayType.hpp>
	#include <XdmfCoreItemFactory.hpp>
	#include <XdmfCoreReader.hpp>
	#include <XdmfHDF5Controller.hpp>
	#include <XdmfHDF5Writer.hpp>
#ifdef XDMF_BUILD_DSM
	#include <XdmfHDF5ControllerDSM.hpp>
	#include <XdmfHDF5WriterDSM.hpp>
#endif
	#include <XdmfHeavyDataController.hpp>
	#include <XdmfHeavyDataWriter.hpp>
	#include <XdmfInformation.hpp>
	#include <XdmfItem.hpp>
	#include <XdmfItemProperty.hpp>
	#include <XdmfSystemUtils.hpp>
	#include <XdmfVisitor.hpp>
	#include <XdmfWriter.hpp>

	// Xdmf Includes
	#include <XdmfAttribute.hpp>
	#include <XdmfAttributeCenter.hpp>
	#include <XdmfAttributeType.hpp>
	#include <XdmfDomain.hpp>
	#include <XdmfGeometry.hpp>
	#include <XdmfGeometryType.hpp>
	#include <XdmfGrid.hpp>
	#include <XdmfGridCollection.hpp>
	#include <XdmfGridCollectionType.hpp>
	#include <XdmfGridCurvilinear.hpp>
	#include <XdmfGridRectilinear.hpp>
	#include <XdmfGridRegular.hpp>
	#include <XdmfItemFactory.hpp>
	#include <XdmfMap.hpp>
	#include <XdmfReader.hpp>
	#include <XdmfSet.hpp>
	#include <XdmfSetType.hpp>
	#include <XdmfTime.hpp>
	#include <XdmfTopology.hpp>
	#include <XdmfTopologyType.hpp>

	// XdmfUtils Includes
	#include <XdmfExodusReader.hpp>
	#include <XdmfExodusWriter.hpp>
	#include <XdmfPartitioner.hpp>
	#include <XdmfTopologyConverter.hpp>
%}

%import Xdmf.i

#ifdef SWIGPYTHON

%pythoncode {
	from Xdmf import *
}

#endif /* SWIGPYTHON */

// Shared Pointer Templates
#ifdef XDMF_BUILD_EXODUS_IO
	%shared_ptr(XdmfExodusReader)
	%shared_ptr(XdmfExodusWriter)
#endif
#ifdef XDMF_BUILD_PARTITIONER
	%shared_ptr(XdmfPartitioner)
#endif
%shared_ptr(XdmfTopologyConverter)

#ifdef XDMF_BUILD_EXODUS_IO
	%include XdmfExodusReader.hpp
	%include XdmfExodusWriter.hpp
#endif
#ifdef XDMF_BUILD_PARTITIONER
	%include XdmfPartitioner.hpp
#endif
%include XdmfTopologyConverter.hpp
