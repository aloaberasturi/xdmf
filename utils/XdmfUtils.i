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
	//#include <XdmfPartitioner.hpp>
	#include <XdmfTopologyConverter.hpp>
%}

%import Xdmf.i

//%pythoncode {
//	from Xdmf import *
//}

// Shared Pointer Templates
%shared_ptr(XdmfExodusReader)
//%shared_ptr(XdmfPartitioner)
%shared_ptr(XdmfTopologyConverter)

%include XdmfExodusReader.hpp
//%include XdmfPartitioner.hpp
%include XdmfTopologyConverter.hpp
