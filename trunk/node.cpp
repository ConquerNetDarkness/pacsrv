//-----------------------------------------------------------------------------
// node.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "node.h" for more information about this class.
//
//-----------------------------------------------------------------------------

/***************************************************************************
 *   Copyright (C) 2003-2005 by Clinton Webb,,,                            *
 *   cjw@cjdj.org                                                          *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DevPlus.h>

#include "node.h"
#include "config.h"
#include "logger.h"


#define NODE_HEARTBEAT_DELAY		15
#define NODE_HEARTBEAT_MISS			3


//-----------------------------------------------------------------------------
// CJW: Constructor.  Since the nodes are assembled in a linked list, we need 
//      to especially make sure that the pointer to the next object is
//      initialised.
Node::Node()
{
    _pNext = NULL;
    _nID = 0;
    _nLastActivity = time(NULL);
	
    _Status.bAccepted	= false;
	_Status.bInit		= false;
	_Status.bClosed		= false;
	_Status.bValid		= false;
	
	_Data.szFilename = NULL;
	_Data.pData		 = NULL;
	_Data.nChunk	 = 0;
	_Data.nSize		 = 0;
	
	_Heartbeat.nBeats	  = 0;
	_Heartbeat.nDelay	  = 0;
	_Heartbeat.nLastCheck = time(NULL);

	_pFileRequest = NULL;
	_pFileReply   = NULL;
	_pServerInfo  = NULL;
	_pRemoteNode  = NULL;
}


//----------------------------------------------------------------------------
// CJW: Deconstructor.  The upper management of the list will ensure that all 
//      nodes that are deleted are first safely removed from the list.  So we
//      also make sure that everything looks ok.
Node::~Node()
{
    ASSERT(_pNext == NULL);
	
	if (_Data.szFilename != NULL) {	
		free(_Data.szFilename);
		_Data.szFilename = NULL;
	}
	
	if (_Data.pData != NULL) {
		ASSERT(_Data.nChunk >= 0);
		ASSERT(_Data.nSize > 0);
		free(_Data.pData);
		_Data.pData = NULL;
	}
	
	if (_pServerInfo != NULL)	{ delete _pServerInfo;	_pServerInfo = NULL; }
	if (_pFileRequest != NULL)	{ delete _pFileRequest;	_pFileRequest = NULL; }
	if (_pFileReply != NULL)	{ delete _pFileReply;	_pFileReply = NULL; }
	if (_pRemoteNode != NULL)	{ delete _pRemoteNode;	_pRemoteNode = NULL; }
}


//-----------------------------------------------------------------------------
// CJW: Here we are over-riding the Accept function so that we can set a status 
//      variable.  We then pass control to the parent Accept function so that
//      it can actually do what it needs to do.
void Node::Accept(SOCKET nSocket)
{
    ASSERT(nSocket > 0);
    ASSERT(_Status.bAccepted == false);
    
    _Status.bAccepted = true;
    
    BaseClient::Accept(nSocket);
}


//-----------------------------------------------------------------------------
// CJW: Since all the nodes are part of a linked list, we need to be able to 
//      provide a pointer to the next object in the list.
Node * Node::GetNext(void)
{
    return(_pNext);
}


//-----------------------------------------------------------------------------
// CJW: Since all the nodes are part of a linked list, we need to be able to 
//      link another object to this one.   Since this function should only ever
//      get called if this node is not already linked, then we will check that
//      we arent already linked (programmer error).
void Node::SetNext(Node *ptr)
{
    ASSERT(ptr != NULL);
    ASSERT(_pNext == NULL);
    
    _pNext = ptr;
}


//-----------------------------------------------------------------------------
// CJW: So that we can identify each node connection in our log, each node 
//      object will have an ID.  We need to keep track of this ID.
void Node::SetID(int nID)
{
    ASSERT(nID > 0);
    ASSERT(_nID == 0);
    _nID = nID;
}


//-----------------------------------------------------------------------------
// CJW: Here we will receive any data from the socket that we can.  We will 
//      first put all the data in an incoming data queue.  Then we will try and
//      process all the messages in the data queue.  Sometimes we can only
//      handle one of a type of message at a time, so we leave any others in
//      the incoming data queue.  When processing messages we will store data
//      in our internal variables ready to be picked up by the Network object.
//      Finally we will send all data we can from the out-going queue.
//
//      If we process any data, then we will return a true, otherwise we will
//      return a false.   The return value does not indicate the status of the
//      node.
bool Node::Process(void)
{
    bool bProcessed = false;
    char cCommand;
    int nLength;
    Logger *pLogger;
    
	ReadData();
    
	nLength = _DataIn.Length();
	if (nLength > 0) {
		cCommand = GetCommand();
		nLength--;

		switch (cCommand) {

			case 'I':   bProcessed = ProcessInit(nLength);          break;
			case 'V':   bProcessed = ProcessValid(nLength);         break;
			case 'Q':   bProcessed = ProcessQuit(nLength);          break;
			case 'S':   bProcessed = ProcessServer(nLength);        break;
			case 'P':   bProcessed = ProcessPing(nLength);          break;
			case 'R':   bProcessed = ProcessPingReply(nLength);     break;
			case 'F':   bProcessed = ProcessFileRequest(nLength);   break;
			case 'G':   bProcessed = ProcessFileGot(nLength);       break;
			case 'L':   bProcessed = ProcessLocalFile(nLength);     break;
			case 'A':   bProcessed = ProcessLocalOK(nLength);       break;
			case 'N':   bProcessed = ProcessLocalFail(nLength);     break;
			case 'C':   bProcessed = ProcessChunkRequest(nLength);  break;
			case 'D':   bProcessed = ProcessChunkData(nLength);     break;
			case 'K':   bProcessed = ProcessFileComplete(nLength);  break;

			default:
				pLogger = new Logger;
				pLogger->System("[Node:%d] Unexpected command.  '%c'", _nID, cCommand);
				delete pLogger;
				bProcessed = false;
				break;
		}
		
		if (bProcessed == true) {
			_Heartbeat.nBeats = 0;
		}
	}

	ProcessHeartbeat();

	SendData();
    
    return(bProcessed);
}


//-----------------------------------------------------------------------------
// CJW: We need to keep track of when the last time some meaningful activity 
//      happened on this node.  We will get the current time, and return the
//      number of seconds difference between the 'last activity time' of this
//      node.
int Node::GetIdleSeconds(void)
{
    int nSeconds = 0;
    time_t nNow;
    
    nNow = time(NULL);
    ASSERT(_nLastActivity <= nNow);
    nSeconds = nNow - _nLastActivity;
    ASSERT(nSeconds >= 0);
    
    return(nSeconds);
}


//-----------------------------------------------------------------------------
// CJW: To make it easier to log the information from each node, we will give 
// 		each one an ID.  THe ID will also be used to keep track of which chunks 
// 		have been asked for.
int Node::GetID(void)
{
	ASSERT(_nID >= 0);
	return(_nID);
}


//-----------------------------------------------------------------------------
// CJW: As we process the data coming from the node, any chunks received will 
//		be kept until this function is called from the Network object.  
//		Therefore, if we have any chunks available for the client, then we will 
//		return pointers to it.  We will still retain control of the memory that 
//		this data is in.  When we get the command to request another chunk, or 
//		get the command that the file is complete, then we will clear out the 
//		values.
bool Node::GetChunks(char **szFilename, char **pData, int *nChunk, int *nSize)
{
	bool bGotChunks = false;
	
	ASSERT(szFilename != NULL);
	ASSERT(pData != NULL);
	ASSERT(nChunk != NULL);
	ASSERT(nSize != NULL);
	
	if (_Data.szFilename != NULL) {
		*szFilename = _Data.szFilename;
	}
	
	if (_Data.pData != NULL) {
		ASSERT(_Data.nChunk >= 0);
		ASSERT(_Data.nSize  > 0);
		ASSERT(_Data.szFilename != NULL);
		
		*pData  = _Data.pData;
		*nChunk = _Data.nChunk;
		*nSize  = _Data.nSize;
		
		bGotChunks = true;
	}
	
	return(bGotChunks);
}


//-----------------------------------------------------------------------------
// CJW: Assumming that the node is currently sending a file, we want to know 
//		what it is.  If we are not processing a file, then dont change the 
//		value of the parameter.
void Node::GetCurrentFile(char **szFilename)
{
	ASSERT(szFilename != NULL);
	
	if (_Data.szFilename != NULL) {
		*szFilename = _Data.szFilename;
	}
}


//-----------------------------------------------------------------------------
// CJW: Ask the node for this particular chunk.  
void Node::RequestChunk(int nChunk)
{
	unsigned char tele[3];
	
	ASSERT(nChunk >= 0);
	
	tele[0] = 'C';
	tele[1] = nChunk >> 8;
	tele[2] = nChunk & 0xff;
	
	ASSERT(sizeof(tele) == 3);
	_DataOut.Add((char *)tele, sizeof(tele));
	
	if (_Data.pData != NULL) {
		ASSERT(_Data.nSize > 0);
		ASSERT(_Data.nChunk >= 0);
		ASSERT(nChunk > _Data.nChunk);
		free(_Data.pData);
		_Data.pData = NULL;
		_Data.nSize  = 0;
	}
	ASSERT(_Data.pData == NULL);
	ASSERT(_Data.nSize == 0);
	
	_Data.nChunk = nChunk;
}	


//-----------------------------------------------------------------------------
// CJW: We have finished receiving all the different chunks for the node.  So 
// 		we need to send a message to the node to tell it we have finished, and 
// 		we also need to clear out the information that we have for this file.
void Node::FileComplete(void)
{
	ASSERT(_Data.szFilename != NULL);
	
	_DataOut.Add("K", 1);
	free(_Data.szFilename);
	_Data.szFilename = NULL;
	
	if (_Data.pData != NULL) {
		free(_Data.pData);
		_Data.nChunk = 0;
		_Data.nSize = 0;
	}
}


//-----------------------------------------------------------------------------
// CJW: This function returns a true if this node is ready to ask for a file.  
// 		I am not sure why we would not be ready to ask for a file, because the 
// 		only time we should be asking this question, we should actually know 
// 		that the node is ready.  Oh well, better put it in anyway just to be 
// 		safe.
bool Node::ReadyForFile(void)
{
	bool bReady = false;
	
	if (_Data.szFilename == NULL) {
		bReady = true;
	}
	
	return(bReady);
}


//-----------------------------------------------------------------------------
// CJW: This function will send a request to the node to ask for a particular 
//		file, this request is directly to the node, and shouldnt really be 
//		relayed to the rest, that was already done.
// 	L<flen><file*flen>  
void Node::RequestFile(char *szFilename)
{
	int nLength;
	struct {
		unsigned char nLen;
	} data;
	
	ASSERT(szFilename != NULL);
	ASSERT(_Data.szFilename == NULL);
	ASSERT(_Data.pData == NULL);
	ASSERT(_Data.nChunk == 0);
	ASSERT(_Data.nSize == 0);
	
	nLength = strlen(szFilename);
	ASSERT(nLength < 256);
	data.nLen = (unsigned char) nLength;
	
	_DataOut.Add("L", 1);
	_DataOut.Add((char *)&data.nLen, 1);
	_DataOut.Add(szFilename, nLength);
	
	_Data.szFilename = (char *) malloc(nLength + 1);
	strncpy(_Data.szFilename, szFilename, nLength);
	_Data.szFilename[nLength] = '\0';
}


//-----------------------------------------------------------------------------
// CJW: The node may have gotten some server information, so this function will 
// 		return it.  If we have received an address from the server, we will 
// 		return it, and the calling function will then become responsible for 
// 		it.  If we dont have one, then we will just return NULL.
Address * Node::GetServerInfo(void)
{
	Address *pInfo = NULL;
	
	if (_pServerInfo != NULL) {
		pInfo = _pServerInfo;
		_pServerInfo = NULL;
	}
	
	return(_pServerInfo);
}


//-----------------------------------------------------------------------------
// CJW: We need to send a request to the node.  It doesnt matter if the node is 
// 		already downloading a file, because in this case, we are telling the 
// 		node to pass the request on.  We are not actually asking the node to start 
// 		downloading this file.  We only want to let the nodes know that we are 
// 		looking for it.
//	
// 		Msg... F<hops><ttl><flen><file*flen><host*6>...<host*6>
void Node::RequestFileFromNetwork(char *szFilename)
{
	struct {
		unsigned char nHops, nTtl, nFlen;
		unsigned char *buffer;
	} data;
	int nTmp;
	
	ASSERT(szFilename != NULL);
	
	data.nHops = 0;
	data.nTtl = DEFAULT_TTL;
	nTmp = strlen(szFilename);
	ASSERT(nTmp < 255);
	data.nFlen = nTmp;
	
	data.buffer = (unsigned char *) malloc(4+data.nFlen+6);
	ASSERT(data.buffer != NULL);
	
	nTmp = 0;
	data.buffer[nTmp++] = 'F';
	data.buffer[nTmp++] = data.nHops;
	data.buffer[nTmp++] = data.nTtl;
	data.buffer[nTmp++] = data.nFlen;
	strncpy((char *)&data.buffer[nTmp], szFilename, data.nFlen);
	nTmp += data.nFlen;
	
	_DataOut.Add((char *)data.buffer, nTmp);
}


//-----------------------------------------------------------------------------
// CJW: We've received an INIT command from the remote node.  This should be 
// 		the first message we receive over the socket.  Once we have received an 
// 		INIT once, we should never receive it again.  If we receive a second 
// 		INIT, we should close the socket.  If we successfully process some 
// 		data, then we will return true.  Oherwise we will return a false to 
// 		indicate that all that data was not yet available.
bool Node::ProcessInit(int nLength)
{
	bool bOK = false;
	unsigned char *pData;
	char szBuffer[32];
	int nPort;
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == false);
	ASSERT(_Status.bInit == false);
	ASSERT(_Status.bAccepted == true);
	ASSERT(_pRemoteNode == NULL);
	
	if (nLength >= 3) {
		pData = (unsigned char *) _DataIn.Pop(3);
		
		// Need to actually check the version number, if it is acceptable, 
		// then we send a "V", otherwise we send a "Q"
		if (pData[0] == 0x01)	{ 
			_DataOut.Add("V", 1); 
			_Status.bValid = true;
			
			nPort = (pData[1] << 8) + pData[2];
			GetPeerName(szBuffer, 32);
			
			_pRemoteNode = new Address;
			_pRemoteNode->Set(szBuffer, nPort);
		}
		else { 
			_DataOut.Add("Q", 1); 
		}
		
		free(pData);
		bOK = true;
	}
	
	return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: We got a "V" reply to our "I" request.  In otherwords, our protocol has 
// 		been validated as acceptable.  We will always return true, because 
// 		there is no more data that we need to get for this instruction.
bool Node::ProcessValid(int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == false);
	ASSERT(_Status.bInit == true);
	ASSERT(_Status.bAccepted == false);
	
	_Status.bValid = true;
	
	return(true);
}


//-----------------------------------------------------------------------------
// CJW: The node is telling us to disconnect from them.  We will always return 
// 		true, because there is no more data that we need to get for this 
// 		instruction.
bool Node::ProcessQuit(int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bAccepted == false);
	
	Close();
	_Status.bClosed = true;
	
	return(true);	
}


//-----------------------------------------------------------------------------
// CJW: We've received some server info from the node.  We save it in our 
// 		server buffer.  If there is already one there, we put the data back, 
// 		and process it again another time.
bool Node::ProcessServer(int nLength)
{
	bool bOK = false;
	unsigned char *pData;
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	if (nLength >= 6) {
		if (_pServerInfo == NULL) {
			pData = (unsigned char *) _DataIn.Pop(6);
			_pServerInfo = new Address;
			_pServerInfo->Set(pData);
			free(pData);
			bOK = true;
		}
		else {
			_DataIn.Push("S", 1);
		}
	}
	else {
		_DataIn.Push("S", 1);
	}
	
	return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: Every so many seconds we need to send a ping to the other end.  If we 
// 		dont get a ping reply, or any other communications, then after so many 
// 		seconds we will timeout the connection and close it.
void Node::ProcessHeartbeat(void)
{
	time_t nTime;
    
	nTime = time(NULL);
	if (nTime > _Heartbeat.nLastCheck) {
		_Heartbeat.nDelay++;
    
		if (_Heartbeat.nDelay >= NODE_HEARTBEAT_DELAY) {
			_Heartbeat.nDelay = 0;
			_Heartbeat.nBeats ++;
			if (_Heartbeat.nBeats >= NODE_HEARTBEAT_MISS) {
				Close();
				_Status.bClosed = true;
			}
			else {
				_DataOut.Add("P", 1);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// CJW: We should receive ping messages from the node every so many seconds.  
// 		If we do, we simply reply with a ping reply.
bool Node::ProcessPing(int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	ASSERT(nLength >= 0);
	
	_DataOut.Add("R", 1);
	
	return(true);
}


//-----------------------------------------------------------------------------
// CJW: We should receive ping messages from the node every so many seconds.  
// 		If we do, we simply reply with a ping reply.
bool Node::ProcessPingReply(int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	ASSERT(nLength >= 0);
	return(true);
}


//-----------------------------------------------------------------------------
// CJW: The node is asking, or passing on, a request for a particular file.  
// 		This is one of the more complicated messages we will receive, because 
// 		it contains a lot of dynamic information.   We dont actually reply to 
// 		anything here, we just take down all the information.  The network 
// 		object will check for this information on each cycle, because it needs 
// 		to pass it to the other nodes.  If we already have file request in our 
// 		buffer, we will not do anything at this stage until that one has been 
// 		processed, it will stay in our incoming queue.
//
//		-->  F<hops><ttl><flen><file*flen><host*6>...<host*6>
bool Node::ProcessFileRequest(int nLength)
{
	bool bOK = false;
	unsigned char *pData = NULL;
	int nLen, i;
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	if (nLength > 3 && _pFileRequest == NULL) {
		pData = (unsigned char *) _DataIn.Pop(3);
		_pFileRequest = new strFileRequest;
		
		_pFileRequest->nHops = pData[0];
		_pFileRequest->nTtl  = pData[1];
		_pFileRequest->nFlen = pData[3];
		
		ASSERT(_pFileRequest->nTtl > 0);
		
		nLen = _pFileRequest->nFlen+(_pFileRequest->nHops * 6);
		if (nLength >= 3+nLen) {
			free(pData);
			pData = (unsigned char *) _DataIn.Pop(nLen);
			
			memcpy(_pFileRequest->szFile, pData, _pFileRequest->nFlen);
			_pFileRequest->szFile[_pFileRequest->nFlen] = '\0';
			
			_pFileRequest->pHosts = (Address **) malloc(sizeof(Address*) * (_pFileRequest->nHops + 1));
			
			for (i=0; i < _pFileRequest->nHops; i++) {
				_pFileRequest->pHosts[i] = new Address;
				_pFileRequest->pHosts[i]->Set(&pData[_pFileRequest->nFlen + (i*6)]);
			}
			
			ASSERT(_pServerInfo != NULL);
			_pFileRequest->pHosts[_pFileRequest->nHops] = new Address;
			_pFileRequest->pHosts[_pFileRequest->nHops]->Set(_pServerInfo);
			_pFileRequest->nHops++;
			_pFileRequest->nTtl--;
			
			bOK = true;
		}
		else {
			_DataIn.Push((char *) pData, 3);
		}

		if (pData != NULL) {
			free(pData);
		}
	}
	else {
		_DataIn.Push("F", 1);
	}
	
	return (bOK);
}


//-----------------------------------------------------------------------------
// CJW: We have asked the other node for a particular file, and it has returned 
// 		an answer.  We need to save the message so that the Network object can 
// 		retrieve it, and pass it to the appropriate node.
bool Node::ProcessFileGot(int nLength)
{
	bool bOK = false;
	unsigned char *pData = NULL;
	int nLen, i;
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	if (nLength > 3 && _pFileReply == NULL) {
		pData = (unsigned char *) _DataIn.Pop(3);
		_pFileReply = new strFileRequest;
		
		_pFileReply->nHops = pData[0];
		_pFileReply->nTtl  = pData[1];
		_pFileReply->nFlen = pData[3];
		
		ASSERT(_pFileReply->nTtl > 0);
		
		nLen = _pFileReply->nFlen+(_pFileReply->nHops * 6);
		if (nLength >= 3+nLen) {
			free(pData);
			pData = (unsigned char *) _DataIn.Pop(nLen);
			
			memcpy(_pFileReply->szFile, pData, _pFileReply->nFlen);
			_pFileReply->szFile[_pFileReply->nFlen] = '\0';
			
			_pFileReply->pHosts = (Address **) malloc(sizeof(Address*) * (_pFileReply->nHops));
			
			for (i=0; i < _pFileReply->nHops; i++) {
				_pFileReply->pHosts[i] = new Address;
				_pFileReply->pHosts[i]->Set(&pData[_pFileReply->nFlen + (i*6)]);
			}
			
			bOK = true;
		}
		else {
			_DataIn.Push((char *) pData, 3);
		}

		if (pData != NULL) { free(pData); }
	}
	else {
		_DataIn.Push("G", 1);
	}
	
	return (bOK);
}


//-----------------------------------------------------------------------------
// CJW: If we received a file request from the node, we would have stored it.  
// 		We will return control of this file request to the caller.  
strFileRequest * Node::GetFileRequest(void)
{
	strFileRequest *pReq;
	
	pReq = _pFileRequest;
	_pFileRequest = NULL;
	
	return(pReq);
}


//-----------------------------------------------------------------------------
// CJW: If we received a file reply from the node, we would have stored it.  
// 		We will return control of this file reply to the caller.  
strFileRequest * Node::GetFileReply(void)
{
	strFileRequest *pReq;
	
	pReq = _pFileReply;
	_pFileReply = NULL;
	
	return(pReq);
}




//-----------------------------------------------------------------------------
// CJW: A message has been formed.  We just add it to the outgoing data queue.
void Node::SendMsg(char *ptr, int len)
{
	ASSERT(ptr != NULL);
	ASSERT(len > 0);
	
	_DataOut.Add(ptr, len);
}


//-----------------------------------------------------------------------------
// CJW: We have found the particular file that one of the nodes is looking for.  
// 		So we will reply thru the node that we got this request from.  That 
// 		node would then have to look at its internal list of nodes to try and 
// 		find the one that passed the request on.
void Node::ReplyFileFound(strFileRequest *pReq, FileInfo *pInfo)
{
	int i, j;
	unsigned char buffer[2048];
	
	ASSERT(pReq != NULL);
	ASSERT(pInfo != NULL);
	
	ASSERT(pReq->nFlen > 0); 
	
	// First we build our message because it is going to be the same for each node.
	i=0;
	buffer[i++] = 'G';
	buffer[i++] = pReply->nHops;
	buffer[i++] = pReply->nFlen;
	
	for (j=0; j< pReply->nFlen; j++) {
		buffer[i++] =  pReply->szFile[j];
	}
	
	ASSERT(_pServerInfo != NULL);
	_pServerInfo->Get(&buffer[i]);
	i +=  6;
	
	for (j=0; j<pReply->nHops; j++) {
		pReply->pHosts[j]->Get(&buffer[i]);
		i += 6;
	}
	
	SendMsg((char *)buffer, i);
}


//-----------------------------------------------------------------------------
// CJW: When the daemon has sent a file request and received connection 
// 		information about a node that has the file, the daemon will send the 
// 		(L) telegram.  Actually, every time we connect to another node, we will 
// 		send a request for all the files that we are trying to fulfull.   If 
// 		the node has the file, it will return an (A) telegram.  If it doesnt 
// 		have the file it will send an (N) telegram.  If we connect to a server 
// 		we will send this command immediately after all initialisation is done.   
// 		If the node connected to us, we will wait 2 seconds and then ask them 
// 		for any file that we have a need for.
//
//		-->  L<flen><file*flen>
// 		<--  A<flen><length*4><file*flen>
// 		<--  N<flen><file*flen>
bool Node::ProcessLocalFile(int nLength)
{
	bool bProcessed;
	unsigned char *pData;
	struct {
		unsigned char flen;
		char *fname;
	} data;
	int length;
	
	if (nLength > 3) {
		pData = (unsigned char *) _DataIn.Pop(1);
		data.flen = *pData;
		free(pData);
		
		ASSERT(data.flen > 0);
		length = _DataIn.Length();
		
		if (length <= data.flen) {
			_DataIn.Push(&data.flen, 1);
			_DataIn.Push("L", 1);
		}
		else {
			pData = _DataIn.Pop(data.flen);
			data.fname = (char *) malloc(data.flen + 1);
			strncpy(data.fname, pData, data.flen);
			free(pData);	
		
			
			
			bOK = true;
			
			
		// check to see if we have that file in our cache.
		// if we do, send an (A), make note of all the details.
		// if we dont, send an (N)
		}
	}
	else {
		_DataIn.Push("L", 1);
	}
}


