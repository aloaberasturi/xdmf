/*****************************************************************************/
/*                                    XDMF                                   */
/*                       eXtensible Data Model and Format                    */
/*                                                                           */
/*  Id : XdmfDSMBuffer.hpp                                                   */
/*                                                                           */
/*  Author:                                                                  */
/*     Andrew Burns                                                          */
/*     andrew.j.burns2@us.army.mil                                           */
/*     US Army Research Laboratory                                           */
/*     Aberdeen Proving Ground, MD                                           */
/*                                                                           */
/*     Copyright @ 2013 US Army Research Laboratory                          */
/*     All Rights Reserved                                                   */
/*     See Copyright.txt for details                                         */
/*                                                                           */
/*     This software is distributed WITHOUT ANY WARRANTY; without            */
/*     even the implied warranty of MERCHANTABILITY or FITNESS               */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice             */
/*     for more information.                                                 */
/*                                                                           */
/*****************************************************************************/

/*=========================================================================
  This code is derived from an earlier work and is distributed
  with permission from, and thanks to ...
=========================================================================*/

/*============================================================================

  Project                 : H5FDdsm
  Module                  : H5FDdsmBufferService.cxx H5FDdsmBuffer.cxx

  Authors:
     John Biddiscombe     Jerome Soumagne
     biddisco@cscs.ch     soumagne@cscs.ch

  Copyright (C) CSCS - Swiss National Supercomputing Centre.
  You may use modify and and distribute this code freely providing
  1) This copyright notice appears on all copies of source code
  2) An acknowledgment appears with any substantial usage of the code
  3) If this code is contributed to any other open source project, it
  must not be reformatted such that the indentation, bracketing or
  overall style is modified significantly.

  This software is distributed WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

  This work has received funding from the European Community's Seventh
  Framework Programme (FP7/2007-2013) under grant agreement 225967 âxtMuSEâOC

============================================================================*/

#include <XdmfDSMBuffer.hpp>
#include <XdmfDSMCommMPI.hpp>
#include <XdmfError.hpp>
#include <mpi.h>
#include <string.h>
#include <stdlib.h>
#include <algorithm>

XdmfDSMBuffer::XdmfDSMBuffer()
{
	this->CommChannel = XDMF_DSM_INTER_COMM;
	this->IsServer = true;
	this->StartAddress = this->EndAddress = 0;
	this->StartServerId = this->EndServerId = -1;
	this->Length = 0;
	this->TotalLength = 0;
	this->BlockLength = 0;
	this->Comm = NULL;
	this->DataPointer = NULL;
	this->IsConnected = false;
}

XdmfDSMBuffer::~XdmfDSMBuffer()
{
	if (this->DataPointer)
	{
		free(this->DataPointer);
	}
	this->DataPointer = NULL;
}

class XdmfDSMBuffer::CommandMsg
{
        public:
                int Opcode;
                int Source;
                int  Target;
                int Address;
                int Length;
};

class XdmfDSMBuffer::InfoMsg
{
	public:
		int type;
		unsigned int length;
		unsigned int total_length;
		unsigned int block_length;
		int start_server_id;
		int end_server_id;
};

void
XdmfDSMBuffer::ConfigureUniform(XdmfDSMCommMPI *aComm, long aLength,
                                int startId, int endId, long aBlockLength,
                                bool random)
{
	if (startId < 0)
	{
		startId = 0;
	}
	if (endId < 0)
	{
		endId = aComm->GetIntraSize() - 1;
	}
	this->SetDsmType(XDMF_DSM_TYPE_UNIFORM_RANGE);
	if ((startId == 0) && (endId == aComm->GetIntraSize() - 1))
	{
		this->SetDsmType(XDMF_DSM_TYPE_UNIFORM);
	}
	if (aBlockLength)
	{
		if (!random)
		{
			this->SetDsmType(XDMF_DSM_TYPE_BLOCK_CYCLIC);
		}
		else
		{
			this->SetDsmType(XDMF_DSM_TYPE_BLOCK_RANDOM);
		}
		this->SetBlockLength(aBlockLength);
	}
	this->StartServerId = startId;
	this->EndServerId = endId;
	this->SetComm(aComm);
	if ((aComm->GetId() >= startId) && (aComm->GetId() <= endId) && this->IsServer)
	{
		try
		{
			if (aBlockLength)
			{
				// For optimization we make the DSM length fit to a multiple of block size
				this->SetLength(((long)(aLength / aBlockLength)) * aBlockLength);
			}
			else
			{
				this->SetLength(aLength);
			}
		}
		catch (XdmfError e)
		{
			throw e;
		}
		this->StartAddress = (aComm->GetId() - startId) * aLength;
		this->EndAddress = this->StartAddress + aLength - 1;
	}
	else
	{
		if (aBlockLength)
		{
			this->Length = ((long)(aLength / aBlockLength)) * aBlockLength;
		}
		else
		{
			this->Length = aLength;
		}
	}
	this->TotalLength = this->GetLength() * (endId - startId + 1);
}

bool
XdmfDSMBuffer::GetIsConnected()
{
	return IsConnected;
}

void
XdmfDSMBuffer::SetIsConnected(bool newStatus)
{
	IsConnected =  newStatus;
}

char *
XdmfDSMBuffer::GetDataPointer()
{
	return this->DataPointer;
}

int
XdmfDSMBuffer::GetDsmType()
{
	return this->DsmType;
}

void
XdmfDSMBuffer::SetDsmType(int newDsmType)
{
	this->DsmType = newDsmType;
}

bool
XdmfDSMBuffer::GetIsServer()
{
	return this->IsServer;
}

void
XdmfDSMBuffer::SetIsServer(bool newIsServer)
{
	this->IsServer = newIsServer;
}

int
XdmfDSMBuffer::GetEndAddress()
{
	return this->EndAddress;
}

int
XdmfDSMBuffer::GetStartAddress()
{
	return this->StartAddress;
}

int
XdmfDSMBuffer::GetStartServerId()
{
	return this->StartServerId;
}

int
XdmfDSMBuffer::GetEndServerId()
{
	return this->EndServerId;
}

long
XdmfDSMBuffer::GetLength()
{
	return this->Length;
}

long
XdmfDSMBuffer::GetTotalLength()
{
	return this->TotalLength;
}

long
XdmfDSMBuffer::GetBlockLength()
{
	return this->BlockLength;
}

void
XdmfDSMBuffer::SetBlockLength(long newBlock)
{
	this->BlockLength = newBlock;
}

XdmfDSMCommMPI *
XdmfDSMBuffer::GetComm()
{
	return this->Comm;
}

void
XdmfDSMBuffer::SetComm(XdmfDSMCommMPI * newComm)
{
	this->Comm = newComm;
}

void
XdmfDSMBuffer::SetLength(long aLength)
{
	this->Length = aLength;
	if (this->DataPointer)
	{
		// try to reallocate
		// this should not be called in most cases
		this->DataPointer = static_cast<char *>(realloc(this->DataPointer, this->Length*sizeof(char)));
	}
	else
	{
#ifdef _WIN32
		this->DataPointer = calloc(this->Length, sizeof(char));
#else
		posix_memalign((void **)(&this->DataPointer), getpagesize(), this->Length);
		memset(this->DataPointer, 0, this->Length);
#endif
	}

	if (this->DataPointer == NULL)
	{
		std::stringstream message;
		message << "Allocation Failed, unable to allocate " << this->Length;
		XdmfError::message(XdmfError::FATAL, message.str());
	}
}

void
XdmfDSMBuffer::SendCommandHeader(int opcode, int dest, int address, int aLength, int comm)
{
	int status;
	CommandMsg cmd;
	memset(&cmd, 0, sizeof(CommandMsg));
	cmd.Opcode = opcode;
	cmd.Source = this->Comm->GetId();
	cmd.Target = dest;
	cmd.Address = address;
	cmd.Length = aLength;

	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Send(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, dest, XDMF_DSM_COMMAND_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		int interSource = 0;
		MPI_Comm_rank(static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &interSource);
		cmd.Source = interSource;
		status = MPI_Send(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, dest, XDMF_DSM_COMMAND_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
	}
	else
	{//in this case the comm should be a pointer to an MPI_Comm object
		status = MPI_Send(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, dest, XDMF_DSM_COMMAND_TAG, comm);
	}
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to send command header");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::ReceiveCommandHeader(int *opcode, int *source, int *address, int *aLength, int comm, int remoteSource)
{
	CommandMsg cmd;
	memset(&cmd, 0, sizeof(CommandMsg));
	int status = MPI_ERR_OTHER;
	MPI_Status signalStatus;

	if (remoteSource < 0)
	{
		remoteSource = MPI_ANY_SOURCE;
	}

	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Recv(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, remoteSource, XDMF_DSM_COMMAND_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(), &signalStatus);
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		status = MPI_Recv(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, remoteSource, XDMF_DSM_COMMAND_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &signalStatus);
	}
	else
	{//in this case the integer is probably a pointer to an MPI_Comm object
		status = MPI_Recv(&cmd, sizeof(CommandMsg), MPI_UNSIGNED_CHAR, remoteSource, XDMF_DSM_COMMAND_TAG, comm, &signalStatus);
	}

	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to receive command header");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
	else
	{
		*opcode  = cmd.Opcode;
                *source  = cmd.Source;
		*address = cmd.Address;
		*aLength = cmd.Length;
	}
}


void
XdmfDSMBuffer::SendData(int dest, char * data, int aLength, int tag, int aAddress, int comm)
{
	int status;
	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Send(data, aLength, MPI_UNSIGNED_CHAR, dest, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		status = MPI_Send(data, aLength, MPI_UNSIGNED_CHAR, dest, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
	}
	else
	{
		status = MPI_Send(data, aLength, MPI_UNSIGNED_CHAR, dest, tag, comm);
	}
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to send data");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::ReceiveData(int source, char * data, int aLength, int tag, int aAddress, int comm)
{
	int status;
	MPI_Status signalStatus;
	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Recv(data, aLength, MPI_UNSIGNED_CHAR, source, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(), &signalStatus);
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		status = MPI_Recv(data, aLength, MPI_UNSIGNED_CHAR, source, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &signalStatus);
	}
	else
	{
		status = MPI_Recv(data, aLength, MPI_UNSIGNED_CHAR, source, tag, comm, &signalStatus);
	}
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::SendAcknowledgment(int dest, int data, int tag, int comm)
{
  	int status;

	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Send(&data, sizeof(int), MPI_UNSIGNED_CHAR, dest, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		status = MPI_Send(&data, sizeof(int), MPI_UNSIGNED_CHAR, dest, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
	}
	else
	{
		status = MPI_Send(&data, sizeof(int), MPI_UNSIGNED_CHAR, dest, tag, comm);
	}
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::ReceiveAcknowledgment(int source, int &data, int tag, int comm)
{
	int status;
	MPI_Status signalStatus;
	if (comm == XDMF_DSM_INTRA_COMM)
	{
		status = MPI_Recv(&data, sizeof(int), MPI_UNSIGNED_CHAR, source, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm(), &signalStatus);
	}
	else if (comm == XDMF_DSM_INTER_COMM)
	{
		status = MPI_Recv(&data, sizeof(int), MPI_UNSIGNED_CHAR, source, tag, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &signalStatus);
	}
	else
	{
		status = MPI_Recv(&data, sizeof(int), MPI_UNSIGNED_CHAR, source, tag, comm, &signalStatus);
	}

	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to receive data");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::SendInfo()
{
	InfoMsg  dsmInfo;
	int status;

	memset(&dsmInfo, 0, sizeof(InfoMsg));
	dsmInfo.type = this->GetDsmType();
	dsmInfo.length = this->GetLength();
	dsmInfo.total_length = this->GetTotalLength();
	dsmInfo.block_length = this->GetBlockLength();
	dsmInfo.start_server_id = this->GetStartServerId();
	dsmInfo.end_server_id = this->GetEndServerId();
	if (this->Comm->GetId() == 0)
	{
		status = MPI_Send(&dsmInfo, sizeof(InfoMsg), MPI_UNSIGNED_CHAR, 0, XDMF_DSM_EXCHANGE_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm());
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Error: Failed to send info");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
	status = MPI_Barrier(this->Comm->GetIntraComm());
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to send info");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

void
XdmfDSMBuffer::ReceiveInfo()
{
	InfoMsg  dsmInfo;
	int status;
	MPI_Status signalStatus;

	memset(&dsmInfo, 0, sizeof(InfoMsg));
	if (this->Comm->GetId() == 0)
	{
		status = MPI_Recv(&dsmInfo, sizeof(InfoMsg), MPI_UNSIGNED_CHAR, XDMF_DSM_ANY_SOURCE, XDMF_DSM_EXCHANGE_TAG, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm(), &signalStatus);
		if (status != MPI_SUCCESS)
		{
			try
			{
				XdmfError::message(XdmfError::FATAL, "Error: Failed to receive info");
			}
			catch (XdmfError e)
			{
				throw e;
			}
		}
	}
	status = MPI_Bcast(&dsmInfo, sizeof(InfoMsg), MPI_UNSIGNED_CHAR, 0, static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm());
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError::message(XdmfError::FATAL, "Error: Failed to broadcast info");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
	this->SetDsmType(dsmInfo.type);
	// We are a client so don't allocate anything but only set a virtual remote length
	this->SetLength(dsmInfo.length);
	this->TotalLength = dsmInfo.total_length;
	this->SetBlockLength(dsmInfo.block_length);
	this->StartServerId = dsmInfo.start_server_id;
	this->EndServerId = dsmInfo.end_server_id;
}

void
XdmfDSMBuffer::BroadcastComm(int *comm, int root)
{
	int status;

	status = MPI_Bcast(comm, sizeof(int), MPI_UNSIGNED_CHAR, root, this->Comm->GetIntraComm());
	if (status != MPI_SUCCESS)
	{
		try
		{
			XdmfError(XdmfError::FATAL, "Broadcast of Comm failed");
		}
		catch (XdmfError e)
		{
			throw e;
		}
	}
}

//redefined from H5FDBufferService

void
XdmfDSMBuffer::BufferServiceLoop(int *returnOpcode)
{
  int op, status = XDMF_DSM_SUCCESS;
  while (status == XDMF_DSM_SUCCESS) {
    try {
      status = this->BufferService(&op);
    }
    catch (XdmfError e) {
      throw e;
    }
    if (returnOpcode) *returnOpcode = op;
    if (op == XDMF_DSM_OPCODE_DONE) {
      break;
    }
  }
}



int
XdmfDSMBuffer::BufferService(int *returnOpcode)
{
  int        opcode, who, status = XDMF_DSM_FAIL;
  int        aLength;
  int          address;
  char        *datap;
  static int syncId      = -1;

  if (this->CommChannel == XDMF_DSM_ANY_COMM) {
    if (this->Comm->GetId() == 0) {
      try {
        this->ProbeCommandHeader(&this->CommChannel);
      }
      catch (XdmfError e) {
        throw e;
      }
    }
    try {
      this->BroadcastComm(&this->CommChannel, 0);
    }
    catch (XdmfError e) {
      throw e;
    }
  }


  try {
    this->ReceiveCommandHeader(&opcode, &who, &address, &aLength, this->CommChannel, syncId);
  }
  catch (XdmfError e) {
    throw e;
  }



  // connection is an ID for client or server,
  // we can use the communicator ID interchangably, but if the architecture is altered - be careful
  int communicatorId = this->CommChannel;

  switch(opcode) {

  // H5FD_DSM_OPCODE_PUT
  case XDMF_DSM_OPCODE_PUT:
    if (((unsigned int) aLength + address) > this->Length) {
      try {
        std::stringstream message;
        message << "Length " << aLength << " too long for Address " << address << "\n" <<
                   "Server Start = " << this->StartAddress << " End = " << this->EndAddress;
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError e) {
        throw e;
      }
    }
    if ((datap = this->DataPointer) == NULL) {
       try {
         XdmfError::message(XdmfError::FATAL, "Null Data Pointer when trying to put data");
       }
       catch (XdmfError e) {
         throw e;
       }
    }
    datap += address;
    try {
      this->ReceiveData(who, datap, aLength, XDMF_DSM_PUT_DATA_TAG, 0, this->CommChannel);
    }
    catch (XdmfError e) {
      throw e;
    }
    break;

  // H5FD_DSM_OPCODE_GET
  case XDMF_DSM_OPCODE_GET:
    if (((unsigned int) aLength + address) > this->Length) {
      try {
        std::stringstream message;
        message << "Length " << aLength << " too long for Address " << address << "\n" <<
                   "Server Start = " << this->StartAddress << " End = " << this->EndAddress;
        XdmfError::message(XdmfError::FATAL, message.str());
      }
      catch (XdmfError e) {
        throw e;
      }
    }
    if ((datap = this->DataPointer) == NULL) {
      try {
         XdmfError::message(XdmfError::FATAL, "Null Data Pointer when trying to put data");
       }
       catch (XdmfError e) {
         throw e;
       }
    }
    datap += address;
    try {
      this->SendData(who, datap, aLength, XDMF_DSM_GET_DATA_TAG, 0, this->CommChannel);
    }
    catch (XdmfError e) {
      throw e;
    }
    break;

  // H5FD_DSM_LOCK_ACQUIRE
  // Comes from client or server depending on communicator
//  case XDMF_DSM_LOCK_ACQUIRE:
	//is->SendAcknowledgment(who, -1, XDMF_DSM_CLIENT_ACK_TAG, this->CommChannel);
/*    // wait for all processes to sync before doing anything
    if (this->Comm->ChannelSynced(who, &syncId, communicatorId)) {
      // only rank 0 actually handles the lock, the other ranks just mimic it later when they get an acknowledgement
      if (this->Comm->GetId() == 0) {
        if (this->BufferServiceInternals->BufferLock.Lock(communicatorId)) {
          // notify all other server nodes - to update their local locks to match rank 0
          H5FDdsmInt32 numberOfRanks = this->Comm->GetIntraSize();
          for (H5FDdsmInt32 who = 1; who < numberOfRanks; ++who) {
            this->SendAcknowledgment(who, communicatorId, H5FD_DSM_SERVER_ACK_TAG, H5FD_DSM_INTRA_COMM);
          }
          // notify the ranks that made the request
          numberOfRanks = (communicatorId==H5FD_DSM_SERVER_ID) ? this->Comm->GetIntraSize() : this->Comm->GetInterSize();
          for (H5FDdsmInt32 who = 0; who < numberOfRanks; ++who) {
            this->SendAcknowledgment(who, communicatorId, H5FD_DSM_CLIENT_ACK_TAG, this->CommChannel);
          }
        }
        //  we were not given the lock, so go back to listening for anyone
        else {
          this->CommChannel = H5FD_DSM_ANY_COMM;
          // notify all other server nodes that lock request failed and to change communicator
          H5FDdsmInt32 numberOfRanks = this->Comm->GetIntraSize();
          for (H5FDdsmInt32 who = 1; who < numberOfRanks; ++who) {
            this->SendAcknowledgment(who, -1, H5FD_DSM_SERVER_ACK_TAG, H5FD_DSM_INTRA_COMM);
          }
        }
      }
      else {
        // all server nodes need to update their local locks to match rank 0
        this->ReceiveAcknowledgment(0, communicatorId, H5FD_DSM_SERVER_ACK_TAG, H5FD_DSM_INTRA_COMM);
        // the lock request failed, so we don't give the lock to the requestor
        if (communicatorId == -1) {
          this->CommChannel = H5FD_DSM_ANY_COMM;
        } else {
          this->BufferServiceInternals->BufferLock.Lock(communicatorId);
        }
      }
    }*/
//    break;

  // H5FD_DSM_LOCK_RELEASE
  // Comes from client or server depending on communicator
//  case XDMF_DSM_LOCK_RELEASE:
/*    // wait for all processes to sync before doing anything
    if (this->Comm->ChannelSynced(who, &syncId, communicatorId)) {
      // only rank 0 actually handles the lock, the other ranks just mimic it later when they get an acknowledgement
      H5FDdsmInt32 newLockOwner = -1;
      if (this->Comm->GetId() == 0) {
        // When we release the lock, it may be passed straight to the next owner,
        // if this happens, we must inform the other server nodes who the owner is
        newLockOwner = this->BufferServiceInternals->BufferLock.Unlock(communicatorId);
        H5FDdsmInt32 numberOfRanks = this->Comm->GetIntraSize();
        for (H5FDdsmInt32 who = 1; who < numberOfRanks; ++who) {
          this->SendAcknowledgment(who, newLockOwner, H5FD_DSM_SERVER_ACK_TAG, H5FD_DSM_INTRA_COMM);
        }
      } else {
        // all server nodes need to update their local locks to match rank 0
        this->ReceiveAcknowledgment(0, newLockOwner, H5FD_DSM_SERVER_ACK_TAG, H5FD_DSM_INTRA_COMM);
        this->BufferServiceInternals->BufferLock.Unlock(communicatorId);
      }

      //
      // the lock has been released : if the client unlocked, wake up waiting server app thread
      // note that a lock count decrease returns the same lock owner, so we don't trigger on that event
      //
      if (newLockOwner != communicatorId && communicatorId == H5FD_DSM_CLIENT_ID) {
        // the address flag holds our unlock status (only treat it when received from client)
        this->SignalUnlock(address, H5FD_DSM_FALSE);
      }
      //
      // if it has been taken by another communicator/connection, do what's needed
      //
      if (newLockOwner == -1) {
        this->CommChannel = H5FD_DSM_ANY_COMM;
         H5FDdsmDebug("Lock released, Switched to " << H5FDdsmCommToString(this->CommChannel));
      }
      else if (newLockOwner != communicatorId) {
        this->CommChannel = newLockOwner;
        H5FDdsmDebug("Lock retaken, Switched to " << H5FDdsmCommToString(this->CommChannel));
        if (this->Comm->GetId() != 0) {
          newLockOwner = this->BufferServiceInternals->BufferLock.Lock(newLockOwner);
        }
        if (this->Comm->GetId() == 0) {
          // notify the ranks that made the original lock request
          H5FDdsmInt32 numberOfRanks = (newLockOwner == H5FD_DSM_SERVER_ID) ? this->Comm->GetIntraSize() : this->Comm->GetInterSize();
          for (H5FDdsmInt32 who = 0; who < numberOfRanks; ++who) {
            this->SendAcknowledgment(who, newLockOwner, H5FD_DSM_CLIENT_ACK_TAG, this->CommChannel);
          }
        }
      }
    }*/
//    break;

  // H5FD_DSM_OPCODE_DONE
  // Always received from server
  case XDMF_DSM_OPCODE_DONE:
    break;

  // DEFAULT
  default :
    try {
      std::stringstream message;
      message << "Error: Unknown Opcode " << opcode;
      XdmfError::message(XdmfError::FATAL, message.str());
    }
    catch (XdmfError e) {
      throw e;
    }
  }

  if (returnOpcode) *returnOpcode = opcode;
  return(XDMF_DSM_SUCCESS);
}


void
XdmfDSMBuffer::SendDone()
{
  try {
    if (static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm() == MPI_COMM_NULL)//this only stops the first core controlled by the server
    {
      for (int i = this->StartServerId; i < this->EndServerId; ++i) {
        if (i != this->Comm->GetId()){
          this->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTRA_COMM);
        }
      }
    }
    else
    {
      for (int i = this->StartServerId; i < this->EndServerId; ++i) {
        if (i != this->Comm->GetId()){
          this->SendCommandHeader(XDMF_DSM_OPCODE_DONE, i, 0, 0, XDMF_DSM_INTER_COMM);
        }
      }
    }
  }
  catch (XdmfError e) {
    throw e;
  }
}

void
XdmfDSMBuffer::ProbeCommandHeader(int *comm)//used for finding a comm that has a waiting command, then sets the comm
{
  int status = XDMF_DSM_FAIL;
  MPI_Status signalStatus;

  int flag;
  MPI_Comm probeComm = static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm();

  // Spin until a message is found on one of the communicators
  while (status != XDMF_DSM_SUCCESS) {
    status = MPI_Iprobe(XDMF_DSM_ANY_SOURCE, XDMF_DSM_ANY_TAG, probeComm, &flag, &signalStatus);
    if (status != MPI_SUCCESS)
    {
       try {
         XdmfError::message(XdmfError::FATAL, "Error: Failed to probe for command header");
       }
       catch (XdmfError e) {
         throw e;
       }
    }
    if (flag) {
      status = XDMF_DSM_SUCCESS;
    }
    else {
      if (static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm() != MPI_COMM_NULL) {
        if (probeComm == static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm()) {
		probeComm = static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm();
	}
	else {
		probeComm = static_cast<XdmfDSMCommMPI *>(this->Comm)->GetIntraComm();
	}
      }
    }
  }
  if (probeComm == static_cast<XdmfDSMCommMPI *>(this->Comm)->GetInterComm()) {
    *comm = XDMF_DSM_INTER_COMM;
  }
  else
  {
    *comm = XDMF_DSM_INTRA_COMM;
  }

  probeComm = MPI_COMM_NULL;
}


int
XdmfDSMBuffer::AddressToId(int Address){
    int   ServerId = XDMF_DSM_FAIL;

    switch(this->DsmType) {
        case XDMF_DSM_TYPE_UNIFORM :
        case XDMF_DSM_TYPE_UNIFORM_RANGE :
            // All Servers have same length
            // This finds out which server the address provided starts on
            ServerId = this->StartServerId + (Address / this->Length);
            if(ServerId > this->EndServerId ){
                try {
                  std::stringstream message;
                  message << "ServerId " << ServerId << " for Address " << Address << " is larger than EndServerId " << this->EndServerId;
                  XdmfError::message(XdmfError::FATAL, message.str());
                }
                catch (XdmfError e) {
                  throw e;
                }
            }
            break;
        default :
            // Not Implemented
            try {
              std::stringstream message;
              message << "DsmType " << this->DsmType << " not yet implemented";
              XdmfError::message(XdmfError::FATAL, message.str());
            }
            catch (XdmfError e) {
              throw e;
            }
            break;
    }
    return(ServerId);
}

void
XdmfDSMBuffer::GetAddressRangeForId(int Id, int *Start, int *End){
    switch(this->DsmType) {
        case XDMF_DSM_TYPE_UNIFORM :
        case XDMF_DSM_TYPE_UNIFORM_RANGE :
            // All Servers have same length
            // Start index is equal to the id inside the servers times the length of the block per server
            // It is the starting index of the server's data block relative to the entire block
            *Start = (Id - this->StartServerId) * this->Length;
            // End index is simply the start index + the length of the server's data block.
            // The range produced is the start of the server's data block to its end.
            *End = *Start + Length - 1;
            break;
        default :
            // Not Implemented
            try {
              std::stringstream message;
              message << "DsmType " << this->DsmType << " not yet implemented";
              XdmfError::message(XdmfError::FATAL, message.str());
            }
            catch (XdmfError e) {
              throw e;
            }
            break;
    }
}

void
XdmfDSMBuffer::Get(long Address, long aLength, void *Data)
{
    int   who, MyId = this->Comm->GetId();
    int   astart, aend, len;
    char   *datap = (char *)Data;

    // While there is length left
    while(aLength)
    {
        // Figure out what server core the address is located on
        who = this->AddressToId(Address);
        if(who == XDMF_DSM_FAIL){
            try {
              XdmfError::message(XdmfError::FATAL, "Address Error");
            }
            catch (XdmfError e) {
              throw e;
            }
        }
        // Get the start and end of the block listed
        this->GetAddressRangeForId(who, &astart, &aend);
	// Determine the amount of data to be written to that core
        // Basically, it's how much data will fit from the starting point of the address to the end
        len = std::min(aLength, aend - Address + 1);
        // If the data is on the core running this code, then the put is simple
        if(who == MyId){
            char *dp;

            dp = this->DataPointer;
            dp += Address - this->StartAddress;
            memcpy(datap, dp, len);

        }else{
            // Otherwise send it to the appropriate core to deal with
            int   status;
            int   dataComm = XDMF_DSM_INTRA_COMM;
            if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
              dataComm = XDMF_DSM_INTER_COMM;
            }
            try {
              this->SendCommandHeader(XDMF_DSM_OPCODE_GET, who, Address - astart, len, dataComm);
            }
            catch (XdmfError e) {
              throw e;
            }
            try {
              this->ReceiveData(who, datap, len, XDMF_DSM_GET_DATA_TAG, Address - astart, dataComm);
            }
            catch (XdmfError e) {
              throw e;
            }
        }
        // Shift all the numbers by the length of the data written
        // Until aLength = 0
        aLength -= len;
        Address += len;
        datap += len;
    }
}

void
XdmfDSMBuffer::Put(long Address, long aLength, const void *Data){
    int   who, MyId = this->Comm->GetId();
    int   astart, aend, len;
    char    *datap = (char *)Data;

    // While there is length left
    while(aLength){
        // Figure out what server core the address is located on
        who = this->AddressToId(Address);
        if(who == XDMF_DSM_FAIL){
            try {
              XdmfError::message(XdmfError::FATAL, "Address Error");
            }
            catch (XdmfError e) {
              throw e;
            }
        }
        // Get the start and end of the block listed
        this->GetAddressRangeForId(who, &astart, &aend);
        // Determine the amount of data to be written to that core
        // Basically, it's how much data will fit from the starting point of the address to the end
        len = std::min(aLength, aend - Address + 1);
        // If the data is on the core running this code, then the put is simple
        if(who == MyId){
            char *dp;

            dp = this->DataPointer;
            dp += Address - this->StartAddress;
            memcpy(dp, datap, len);

        }else{
            // Otherwise send it to the appropriate core to deal with
            int   status;
            int   dataComm = XDMF_DSM_INTRA_COMM;
            if (this->Comm->GetInterComm() != MPI_COMM_NULL) {
              dataComm = XDMF_DSM_INTER_COMM;
            }
            try {
              this->SendCommandHeader(XDMF_DSM_OPCODE_PUT, who, Address - astart, len, dataComm);
            }
            catch (XdmfError e) {
              throw e;
            }
            try {
              this->SendData(who, datap, len, XDMF_DSM_PUT_DATA_TAG, Address - astart, dataComm);
            }
            catch (XdmfError e) {
              throw e;
            }
        }
        // Shift all the numbers by the length of the data written
        // Until aLength = 0
        aLength -= len;
        Address += len;
        datap += len;
    }
}
