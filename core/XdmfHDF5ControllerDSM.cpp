/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfHDF5ControllerDSM.cpp                                           */
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

#include <H5FDdsm.h>
#include <H5FDdsmManager.h>
#include <hdf5.h>
#include "XdmfHDF5ControllerDSM.hpp"
#include "XdmfDSMManager.hpp"
#include "XdmfDSMBuffer.hpp"
#include "XdmfDSMCommMPI.hpp"
#include "XdmfDSMDriver.hpp"
#include "XdmfError.hpp"

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           H5FDdsmBuffer * const dsmBuffer)
{
  shared_ptr<XdmfHDF5ControllerDSM> 
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                dsmBuffer));
  return p;
}

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           MPI_Comm comm,
                           unsigned int bufferSize)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                comm,
                                bufferSize));
  return p;
}

//server/ nonthreaded versions
shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           XdmfDSMBuffer * const dsmBuffer,
                           int startCoreIndex,
                           int endCoreIndex)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                dsmBuffer,
                                startCoreIndex,
                                endCoreIndex));
  return p;
}

shared_ptr<XdmfHDF5ControllerDSM>
XdmfHDF5ControllerDSM::New(const std::string & hdf5FilePath,
                           const std::string & dataSetPath,
                           const shared_ptr<const XdmfArrayType> type,
                           const std::vector<unsigned int> & start,
                           const std::vector<unsigned int> & stride,
                           const std::vector<unsigned int> & dimensions,
                           const std::vector<unsigned int> & datspaceDimensions,
                           MPI_Comm comm,
                           unsigned int bufferSize,
                           int startCoreIndex,
                           int endCoreIndex)
{
  shared_ptr<XdmfHDF5ControllerDSM>
    p(new XdmfHDF5ControllerDSM(hdf5FilePath,
                                dataSetPath,
                                type,
                                start,
                                stride,
                                dimensions,
                                datspaceDimensions,
                                comm,
                                bufferSize,
                                startCoreIndex,
                                endCoreIndex));
  return p;
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             H5FDdsmBuffer * const dsmBuffer) :
  XdmfHDF5Controller(hdf5FilePath, 
                     dataSetPath, 
                     type, 
                     start,
                     stride,
                     dimensions, 
                     dataspaceDimensions),
  mDSMManager(NULL),
  mDSMBuffer(dsmBuffer),
  mDSMServerBuffer(NULL),
  mDSMServerManager(NULL),
  mGroupComm(MPI_COMM_NULL),
  mServerComm(MPI_COMM_NULL),
  mWorkerComm(MPI_COMM_NULL),
  mStartCoreIndex(-1),
  mEndCoreIndex(-1),
  mRank(-1),
  mGroupSize(-1),
  mServerMode(false)
{
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             MPI_Comm comm,
                                             unsigned int bufferSize) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mDSMServerBuffer(NULL),
  mDSMServerManager(NULL),
  mGroupComm(MPI_COMM_NULL),
  mServerComm(MPI_COMM_NULL),
  mWorkerComm(MPI_COMM_NULL),
  mStartCoreIndex(-1),
  mEndCoreIndex(-1),
  mRank(-1),
  mGroupSize(-1),
  mServerMode(false)

{
  H5FDdsmManager * newManager = new H5FDdsmManager();
  newManager->SetMpiComm(comm);
  newManager->SetLocalBufferSizeMBytes(bufferSize);
  newManager->SetIsStandAlone(H5FD_DSM_TRUE);
  newManager->Create();

  H5FD_dsm_set_manager(newManager);

  H5FD_dsm_set_options(H5FD_DSM_LOCK_ASYNCHRONOUS);

  H5FDdsmBuffer * newBuffer = newManager->GetDsmBuffer();

  mDSMManager = newManager;
  mDSMBuffer = newBuffer;
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             XdmfDSMBuffer * const dsmBuffer,
                                             int startCoreIndex,
                                             int endCoreIndex) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mDSMManager(NULL),
  mDSMBuffer(NULL),
  mDSMServerBuffer(dsmBuffer),
  mDSMServerManager(NULL),
  mStartCoreIndex(startCoreIndex),
  mEndCoreIndex(endCoreIndex),
  mServerMode(true)
{
  mGroupComm = mDSMServerBuffer->GetComm()->GetInterComm();
  MPI_Comm_rank(mGroupComm, &mRank);
  MPI_Comm_size(mGroupComm, &mGroupSize);
  if (mRank >=mStartCoreIndex && mRank <=mEndCoreIndex) {
    mServerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
    mWorkerComm = MPI_COMM_NULL;
  }
  else {
    mServerComm = MPI_COMM_NULL;
    mWorkerComm = mDSMServerBuffer->GetComm()->GetIntraComm();
  }
}

XdmfHDF5ControllerDSM::XdmfHDF5ControllerDSM(const std::string & hdf5FilePath,
                                             const std::string & dataSetPath,
                                             const shared_ptr<const XdmfArrayType> type,
                                             const std::vector<unsigned int> & start,
                                             const std::vector<unsigned int> & stride,
                                             const std::vector<unsigned int> & dimensions,
                                             const std::vector<unsigned int> & dataspaceDimensions,
                                             MPI_Comm comm,
                                             unsigned int bufferSize,
                                             int startCoreIndex,
                                             int endCoreIndex) :
  XdmfHDF5Controller(hdf5FilePath,
                     dataSetPath,
                     type,
                     start,
                     stride,
                     dimensions,
                     dataspaceDimensions),
  mDSMBuffer(NULL),
  mDSMManager(NULL),
  mServerMode(true)

{
        //negative values will be changed to maximum range
        if (startCoreIndex < 0)
        {
                startCoreIndex = 0;
        }
        if (endCoreIndex < 0)
        {
                endCoreIndex = mGroupSize - 1;
        }

        //ensure start index is less than end index
        if (startCoreIndex > endCoreIndex)
        {
                int tempholder = startCoreIndex;
                startCoreIndex = endCoreIndex;
                endCoreIndex = tempholder;
        }

        mGroupComm = comm;
        mStartCoreIndex = startCoreIndex;
        mEndCoreIndex = endCoreIndex;

        MPI_Comm_size(comm, &mGroupSize);
        MPI_Comm_rank(comm, &mRank);

        MPI_Group workers, dsmgroup, serversplit, servergroup;

        int * ServerIds = (int *)calloc((3), sizeof(int));
        unsigned int index = 0;
        for(int i=mStartCoreIndex ; i <= mEndCoreIndex ; ++i)
        {
                ServerIds[index++] = i;
        }

        MPI_Comm_group(comm, &serversplit);
        MPI_Group_incl(serversplit, index, ServerIds, &servergroup);
        MPI_Comm_create(comm, servergroup, &mServerComm);
        MPI_Comm_group(comm, &dsmgroup);
        MPI_Group_excl(dsmgroup, index, ServerIds, &workers);
        MPI_Comm_create(comm, workers, &mWorkerComm);
        cfree(ServerIds);

        //create the manager

        mDSMServerManager = new XdmfDSMManager();

        mDSMServerManager->SetLocalBufferSizeMBytes(bufferSize);
        mDSMServerManager->SetInterCommType(H5FD_DSM_COMM_MPI);

        if (mRank >=mStartCoreIndex && mRank <=mEndCoreIndex)
        {
                mDSMServerManager->SetMpiComm(mServerComm);
                mDSMServerManager->Create();
        }
        else
        {
                mDSMServerManager->SetMpiComm(mWorkerComm);
                mDSMServerManager->SetIsServer(false);
                mDSMServerManager->Create(mStartCoreIndex, mEndCoreIndex);
        }

        XDMF_dsm_set_manager(mDSMServerManager);

        mDSMServerBuffer = mDSMServerManager->GetDsmBuffer();

        mDSMServerBuffer->GetComm()->DupInterComm(mGroupComm);
        mDSMServerBuffer->SetIsConnected(true);

        MPI_Barrier(comm);

        //loop needs to be started before anything can be done to the file, since the service is what sets up the file

        if (mRank < mStartCoreIndex || mRank > mEndCoreIndex)
        {
                //turn off the server designation
                mDSMServerBuffer->SetIsServer(H5FD_DSM_FALSE);//if this is set to false then the buffer will attempt to connect to the intercomm for DSM stuff
                mDSMServerManager->SetIsServer(H5FD_DSM_FALSE);
        }
        else
        {
                //on cores where memory is set up, start the service loop
                //this should iterate infinitely until a value to end the loop is passed
                H5FDdsmInt32 returnOpCode;
                try
                {
                        mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
                }
                catch (XdmfError e)
                {
                        throw e;
                }
        }
}

XdmfHDF5ControllerDSM::~XdmfHDF5ControllerDSM()
{
}

std::string XdmfHDF5ControllerDSM::getName() const
{
  return "HDFDSM";
}

H5FDdsmManager * XdmfHDF5ControllerDSM::getManager()
{
	return mDSMManager;
}

H5FDdsmBuffer * XdmfHDF5ControllerDSM::getBuffer()
{
        return mDSMBuffer;
}

XdmfDSMManager * XdmfHDF5ControllerDSM::getServerManager()
{
  return mDSMServerManager;
}

XdmfDSMBuffer * XdmfHDF5ControllerDSM::getServerBuffer()
{
  return mDSMServerBuffer;
}

bool XdmfHDF5ControllerDSM::getServerMode()
{
  return mServerMode;
}

MPI_Comm XdmfHDF5ControllerDSM::getServerComm()
{
	MPI_Comm returnComm = MPI_COMM_NULL;
	int status = MPI_Comm_dup(mServerComm, &returnComm);
	return returnComm;
}

MPI_Comm XdmfHDF5ControllerDSM::getWorkerComm()
{
	MPI_Comm returnComm = MPI_COMM_NULL;
	int status = MPI_Comm_dup(mWorkerComm, &returnComm);
	return returnComm;
}

void XdmfHDF5ControllerDSM::setManager(XdmfDSMManager * newManager)
{
  XdmfDSMBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMServerManager = newManager;
  mDSMServerBuffer = newBuffer;
}

void XdmfHDF5ControllerDSM::setBuffer(XdmfDSMBuffer * newBuffer)
{
  mDSMServerBuffer = newBuffer;
}

void XdmfHDF5ControllerDSM::setManager(H5FDdsmManager * newManager)
{
  H5FDdsmBuffer * newBuffer = newManager->GetDsmBuffer();
  mDSMManager = newManager;
  mDSMBuffer = newBuffer;
}

void XdmfHDF5ControllerDSM::setBuffer(H5FDdsmBuffer * newBuffer)
{
	mDSMBuffer = newBuffer;
}

void XdmfHDF5ControllerDSM::setServerMode(bool newMode)
{
  mServerMode = newMode;
}

void XdmfHDF5ControllerDSM::setServerComm(MPI_Comm comm)
{
	int status;
	if (mServerComm != MPI_COMM_NULL)
	{
		status = MPI_Comm_free(&mServerComm);
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
	if (comm != MPI_COMM_NULL)
	{
		status = MPI_Comm_dup(comm, &mServerComm);
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
        mDSMServerBuffer->GetComm()->DupComm(comm);
}

void XdmfHDF5ControllerDSM::setWorkerComm(MPI_Comm comm)
{
	int status;
	if (mWorkerComm != MPI_COMM_NULL)
	{
		status = MPI_Comm_free(&mWorkerComm);
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Failed to disconnect Comm");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
	if (comm != MPI_COMM_NULL)
	{
		status = MPI_Comm_dup(comm, &mWorkerComm);
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Failed to duplicate Comm");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
        mDSMServerBuffer->GetComm()->DupComm(comm);
}

void XdmfHDF5ControllerDSM::stopDSM()
{
        //send manually
        for (int i = mStartCoreIndex; i <= mEndCoreIndex; ++i)
        {
                try
                {
                        mDSMServerBuffer->SendCommandHeader(H5FD_DSM_OPCODE_DONE, i, 0, 0, H5FD_DSM_INTER_COMM);
                }
                catch (XdmfError e)
                {
                        throw e;
                }
                //originally this was set to the intra comm
                //that doesn't work in this instance because it won't reach the server cores
        }
}

void XdmfHDF5ControllerDSM::restartDSM()
{
        if (mRank >= mStartCoreIndex && mRank <= mEndCoreIndex)
        {
                H5FDdsmInt32 returnOpCode;
                try
                {
                        mDSMServerBuffer->BufferServiceLoop(&returnOpCode);
                }
                catch (XdmfError e)
                {
                        throw e;
                }
        }
}

void XdmfHDF5ControllerDSM::deleteManager()
{
	if (mDSMManager != NULL)
	{
		delete mDSMManager;
	}
        if (mDSMServerManager != NULL)
        {
                delete mDSMServerManager;
        }
}

void XdmfHDF5ControllerDSM::read(XdmfArray * const array)
{
  // Set file access property list for DSM
  hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);

  // Use DSM driver
  if (mServerMode) {
    if (mWorkerComm != MPI_COMM_NULL) {
      XDMFH5Pset_fapl_dsm(fapl, mWorkerComm, mDSMServerBuffer, 0);
    }
  }
  else {
    H5Pset_fapl_dsm(fapl, MPI_COMM_WORLD, mDSMBuffer, 0);
  }

  // Read from DSM Buffer
  XdmfHDF5Controller::read(array, fapl);

  // Close file access property list
  herr_t status = H5Pclose(fapl);
}
