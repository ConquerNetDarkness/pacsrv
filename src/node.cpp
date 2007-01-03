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
	_Data.pFileInfo  = NULL;
	
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
int Node::OnReceive(char *pData, int nLength)
{
    int nProcessed = 0;
    Logger *pLogger;
    
    ASSERT(pData != NULL && nLength > 0);
    
    Lock();
    
	switch (pData[0]) {

		case 'I':   nProcessed = ProcessInit(pData, nLength);          break;
		case 'V':   nProcessed = ProcessValid(pData, nLength);         break;
		case 'Q':   nProcessed = ProcessQuit(pData, nLength);          break;
		case 'S':   nProcessed = ProcessServer(pData, nLength);        break;
		case 'P':   ProcessPing(); 		nProcessed = 1;			       break;
		case 'R':   ProcessPingReply();	nProcessed = 1;				   break;
		case 'F':   nProcessed = ProcessFileRequest(pData, nLength);   break;
		case 'G':   nProcessed = ProcessFileGot(pData, nLength);       break;
		case 'L':   nProcessed = ProcessLocalFile(pData, nLength);     break;
		case 'A':   nProcessed = ProcessLocalOK(pData, nLength);       break;
		case 'N':   nProcessed = ProcessLocalFail(pData, nLength);     break;
		case 'C':   nProcessed = ProcessChunkRequest(pData, nLength);  break;
		case 'D':   nProcessed = ProcessChunkData(pData, nLength);     break;
		case 'K':   nProcessed = ProcessFileComplete(pData, nLength);  break;

		default:
			pLogger = new Logger;
			pLogger->System("[Node:%d] Unexpected command.  '%c'", _nID, pData[0]);
			delete pLogger;
			nProcessed = 1;
			// TODO: Should we close the connection?
			break;
	}
		
	if (nProcessed > 0) {
		_Heartbeat.nBeats = 0;
	}

	ProcessHeartbeat();

    return(nProcessed);
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
	Send((char *)tele, sizeof(tele));
	
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
	
	Send("K", 1);
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
	
	Send("L", 1);
	Send((char *)&data.nLen, 1);
	Send(szFilename, nLength);
	
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
	
	Send((char *)data.buffer, nTmp);
}


//-----------------------------------------------------------------------------
// CJW: We've received an INIT command from the remote node.  This should be 
// 		the first message we receive over the socket.  Once we have received an 
// 		INIT once, we should never receive it again.  If we receive a second 
// 		INIT, we should close the socket.  If we successfully process some 
// 		data, then we will return true.  Oherwise we will return a false to 
// 		indicate that all that data was not yet available.
int Node::ProcessInit(char *pData, int nLength)
{
	int nProcessed = 0;
	char szBuffer[32];
	int nPort;
	unsigned char *pTmp;
	
	ASSERT(pData != NULL && nLength > 0);
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == false);
	ASSERT(_Status.bInit == false);
	ASSERT(_Status.bAccepted == true);
	ASSERT(_pRemoteNode == NULL);
	
	if (nLength >= 3) {
		
		// Need to actually check the version number, if it is acceptable, 
		// then we send a "V", otherwise we send a "Q"
		if (pData[1] == 0x01)	{ 
			Send("V", 1); 
			_Status.bValid = true;
			
			pTmp = (unsigned char *) pData;
			nPort = (pTmp[2] << 8) + pTmp[3];
			ASSERT(nPort > 0 && nPort < 65536);
			GetPeerName(szBuffer, 32);
			
			_pRemoteNode = new Address;
			_pRemoteNode->Set(szBuffer, nPort);
			
		}
		else { 
			Send("Q", 1); 
		}
		
		nProcessed = 3;
	}
	
	return(nProcessed);
}


//-----------------------------------------------------------------------------
// CJW: We got a "V" reply to our "I" request.  In otherwords, our protocol has 
// 		been validated as acceptable.  We will always return true, because 
// 		there is no more data that we need to get for this instruction.
int Node::ProcessValid(char *pData, int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == false);
	ASSERT(_Status.bInit == true);
	ASSERT(_Status.bAccepted == false);
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'V');
	
	_Status.bValid = true;
	
	return(1);
}


//-----------------------------------------------------------------------------
// CJW: The node is telling us to disconnect from them.  We will always return 
// 		true, because there is no more data that we need to get for this 
// 		instruction.
int Node::ProcessQuit(char *pData, int nLength)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bAccepted == false);
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'Q');
	
	Close();
	_Status.bClosed = true;
	
	return(1);	
}


//-----------------------------------------------------------------------------
// CJW: We've received some server info from the node.  We save it in our 
// 		server buffer.  If there is already one there, we put the data back, 
// 		and process it again another time.
int Node::ProcessServer(char *pData, int nLength)
{
	int nProcessed = 0;
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	ASSERT(pData != NULL && nLength > 0);
	
	if (nLength >= 7) {
		if (_pServerInfo == NULL) {
			_pServerInfo = new Address;
			_pServerInfo->Set((unsigned char *) &pData[1]);
			nProcessed = 7;
		}
	}
	
	return(nProcessed);
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
				Send("P", 1);
			}
		}
	}
}


//-----------------------------------------------------------------------------
// CJW: We should receive ping messages from the node every so many seconds.  
// 		If we do, we simply reply with a ping reply.
void Node::ProcessPing()
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	Send("R", 1);
}


//-----------------------------------------------------------------------------
// CJW: We should receive ping messages from the node every so many seconds.  
// 		If we do, we simply reply with a ping reply.
void Node::ProcessPingReply(void)
{
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	// we could add some logic to determine the latency time for a connection, but we will leave that for a later version because it can be a bit complicated to implement.
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
int Node::ProcessFileRequest(char *pData, int nLength)
{
	int nProcessed = 0;
	unsigned char *pTmp = NULL;
	int i;
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'F');
	
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	if (nLength > 3 && _pFileRequest == NULL) {
		pTmp = (unsigned char *) &pData[1];
		
		_pFileRequest = new strFileRequest;
		_pFileRequest->nHops = pTmp[0];
		_pFileRequest->nTtl  = pTmp[1];
		_pFileRequest->nFlen = pTmp[2];
		ASSERT(_pFileRequest->nTtl > 0);
		ASSERT(_pFileRequest->nFlen > 0);
		
		nProcessed = 4;
		if (nLength >= 4+_pFileRequest->nFlen+(_pFileRequest->nHops * 6)) {
			
			memcpy(_pFileRequest->szFile, &pTmp[3], _pFileRequest->nFlen);
			_pFileRequest->szFile[_pFileRequest->nFlen] = '\0';
			nProcessed += _pFileRequest->nFlen;
			
			_pFileRequest->pHosts = (Address **) malloc(sizeof(Address*) * (_pFileRequest->nHops + 1));
			
			for (i=0; i < _pFileRequest->nHops; i++) {
				_pFileRequest->pHosts[i] = new Address;
				_pFileRequest->pHosts[i]->Set(&pTmp[_pFileRequest->nFlen + (i*6)]);
				nProcessed += 6;
			}
			
			ASSERT(_pServerInfo != NULL);
			_pFileRequest->pHosts[_pFileRequest->nHops] = new Address;
			_pFileRequest->pHosts[_pFileRequest->nHops]->Set(_pServerInfo);
			_pFileRequest->nHops++;
			_pFileRequest->nTtl--;
		}
		else {
			nProcessed = 0;
		}
	}
	
	ASSERT(nProcessed == 0 || nProcessed > 5);
	return (nProcessed);
}


//-----------------------------------------------------------------------------
// CJW: We have asked the other node for a particular file, and it has returned 
// 		an answer.  We need to save the message so that the Network object can 
// 		retrieve it, and pass it to the appropriate node.
int Node::ProcessFileGot(char *pData, int nLength)
{
	int nProcessed = 0;
	unsigned char *pTmp = NULL;
	int i;
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'G');
	ASSERT(_Status.bClosed == false);
	ASSERT(_Status.bValid == true);
	
	if (nLength > 3 && _pFileReply == NULL) {
		pTmp = (unsigned char *) &pData[1];
		_pFileReply = new strFileReply;
		
		_pFileReply->nHops = *(pTmp++);
		_pFileReply->nFlen = *(pTmp++);
		nProcessed = 3;
		
		if (nLength >= 3+_pFileReply->nFlen+(_pFileReply->nHops * 6)) {
			
			memcpy(_pFileReply->szFile, &pTmp[2], _pFileReply->nFlen);
			_pFileReply->szFile[_pFileReply->nFlen] = '\0';
			nProcessed += _pFileReply->nFlen;
			pTmp += _pFileReply->nFlen;
			
			_pFileReply->pTarget = new Address;
			_pFileReply->pTarget->Set(pTmp);
			nProcessed += 6;
			pTmp += 6;
			
			_pFileReply->pHosts = (Address **) malloc(sizeof(Address*) * (_pFileReply->nHops));
			for (i=0; i < _pFileReply->nHops; i++) {
				_pFileReply->pHosts[i] = new Address;
				_pFileReply->pHosts[i]->Set(pTmp);
				nProcessed += 6;
				pTmp += 6;
			}
		}
		else {
			nProcessed = 0;
		}
	}
	else {
		ASSERT(nProcessed == 0);
	}
	
	ASSERT(nProcessed == 0 || nProcessed > 4);
	return (nProcessed);
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
strFileReply * Node::GetFileReply(void)
{
	strFileReply *pReply;
	
	pReply = _pFileReply;
	_pFileReply = NULL;
	
	return(pReply);
}




//-----------------------------------------------------------------------------
// CJW: A message has been formed.  We just add it to the outgoing data queue.
void Node::SendMsg(char *ptr, int len)
{
	ASSERT(ptr != NULL);
	ASSERT(len > 0);
	
	Send(ptr, len);
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
	buffer[i++] = pReq->nHops;
	buffer[i++] = pReq->nFlen;
	
	for (j=0; j< pReq->nFlen; j++) {
		buffer[i++] =  pReq->szFile[j];
	}
	
	ASSERT(_pServerInfo != NULL);
	_pServerInfo->Get(&buffer[i]);
	i +=  6;
	
	for (j=0; j<pReq->nHops; j++) {
		pReq->pHosts[j]->Get(&buffer[i]);
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
int Node::ProcessLocalFile(char *pData, int nLength)
{
	int nProcessed=0;
	unsigned char *pTmp;
	unsigned char flen;
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'L');
	
	if (nLength > 3 && _szLocalFile == NULL) {
		pTmp = (unsigned char *) &pData[1];
		flen = *pTmp;
		
		nProcessed = 2;
		
		ASSERT(flen > 0);
		
		if (nLength >= 2 + flen) {
			ASSERT(_Data.szFilename == NULL)
			_Data.szFilename = (char *) malloc(flen + 1);
			strncpy(_Data.szFilename, (char *) &pData[2], flen);
			_Data.szFilename[flen] = '\0';
		
			nProcessed += flen;
			
			// we dont return anything yet.  We wait for the Network object to 
			// notice that we have a LocalFile request to process.  When it 
			// finds it, it will tell us how to proceed.
		}
	}
	else {
		ASSERT(nProcessed == 0);
	}
	
	ASSERT(nProcessed == 0 || nProcessed >= 3);
	return (nProcessed);
}


//-----------------------------------------------------------------------------
// CJW: Return the local file lookup, if there is one.  We return the string 
// 		if we have one ready.  
char *Node::GetLocalFile(void) 
{
	char *ptr = NULL;
	
	if (_Data.pFileInfo == NULL && _Data.szFilename != NULL) {
		ptr = _Data.szFilename;
		_Data.szFilename = NULL;
	}
	
	return(ptr);
}


//-----------------------------------------------------------------------------
// CJW: The node asked if we have a localfile.  The network object looked up 
// 		our file list and determined that we have this file (or maybe part of 
// 		it).  We need to reply to the node that we have the file (and its 
// 		length).  Then we need to mark the file as in use, and then wait for 
// 		the chunck requests to come in.
//
// 		<--  A<flen><length*4><file*flen>
void Node::SendFile(char *szLocalFile, FileInfo *pInfo)
{
	int nLength;
	
	pInfo->FileStart();
	nLength = pInfo->GetLength();
	TODO("Incomplete");
}


//-----------------------------------------------------------------------------
// CJW: Reply to the node that we dont have the file they are looking for.
// 		<--  N<flen><file*flen>
void Node::LocalFileFail(char *szLocalFile)
{
	unsigned char flen;
	int length;

	ASSERT(szLocalFile != NULL);
	length = strlen(szLocalFile);
	ASSERT(length < 255);
	flen = (unsigned char) length;
	 
	Send("N", 1);
	Send((char *) &flen, 1);
	Send(szLocalFile, length);
}


//-----------------------------------------------------------------------------
// CJW: When the peer-daemon has sent a file request and received connection 
// 		information about a node that has the file, the daemon will send the 
// 		(L) telegram.  Actually, every time we connect to another node, we will 
// 		send a request for all the files that we are trying to fulfull.   If 
// 		the node has the file, it will return an (A) telegram.  If it doesnt 
// 		have the file it will send an (N) telegram.  If we connect to a server 
// 		we will send this command immediately after all initialisation is done.   
// 		If the node connected to us, we will wait 2 seconds and then ask them 
// 		for any file that we have a need for.
//
//		In this case, we have received a confirmation from the peer that it 
//		has the file, and we could begin asking it for chunks.
//     
//     <--  A<flen><length*4><file*flen>
int Node::ProcessLocalOK(char *pData, int nLength)
{
	int nProcessed = 0;
	char szFilename[256];
	int len;
	unsigned char *pTmp;
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'A');
	
	if (nLength > 6) {
		pTmp = (unsigned char *) &pData[1];
		len = pTmp[0];
	
		if (nLength >= 6+len) {
			nProcessed = 6+len;
	
			ASSERT(len > 0 && len < 256);
			strncpy(szFilename, &pData[6], len);
			szFilename[len] = '\0';
	
			len = 0;
			len += ((unsigned char) pData[2]) << 24;
			len += ((unsigned char) pData[3]) << 16;
			len += ((unsigned char) pData[4]) << 8;
			len +=  (unsigned char) pData[5];

			ASSERT(_Data.pFileInfo == NULL);
			_Data.pFileInfo = new FileInfo;
			ASSERT(_Data.pFileInfo != NULL);
			
			_Data.pFileInfo->SetFile(szFilename);
			_Data.pFileInfo->SetLength(len);
		}
	}
		
	ASSERT(nProcessed == 0 || nProcessed > 6);
	return(nProcessed);
}


//-----------------------------------------------------------------------------
// CJW:	The remote node has replied that it doesnt have the file that we are 
// 		attempting to download.  We need to make a note of it so that the 
// 		controlling object can then ask for the next file in our list.
//     -->  L<flen><file*flen>
//     <--  A<flen><length*4><file*flen>
//     <--  N<flen><file*flen>
int Node::ProcessLocalFail(char *pData, int nLength)
{
	int nProcessed = 0;
	char szFilename[256];
	int len;
	unsigned char *pTmp;
	
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'N');
	ASSERT(_Data.szFilename != NULL);
	
	if (nLength > 3) {
		pTmp = (unsigned char *) &pData[1];
		len = pTmp[0];
		ASSERT(len > 0);
	
		if (nLength >= 2+len) {
			nProcessed = 2+len;
	
			ASSERT(len > 0 && len < 256);
			strncpy(szFilename, &pData[2], len);
			szFilename[len] = '\0';
			
			// free the filename so that we know that we are not currently processing the file.
			ASSERT(strcmp(szFilename, _Data.szFilename) == 0);
			free(_Data.szFilename);
			_Data.szFilename = NULL;
		}
	}
	
	ASSERT(nProcessed == 0 || nProcessed > 2);
	return(nProcessed);
}




//-----------------------------------------------------------------------------
// CJW:	The node is requesting a particular chunk.  For this to be valid, the 
// 		'L' telegram must already have been received and accepted.
//     -->  C<chunk*2>
//     <--  D<chunk*2><len*2><data*len>
int Node::ProcessChunkRequest(char *pData, int nLength)
{
	int nProcessed = 0;
	int nChunk;

	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'C');
	
	if (nLength >= 3 && _Data.nChunk == 0) {
		nChunk = 0;
		nChunk += ((unsigned char) pData[1]) << 8;
		nChunk +=  (unsigned char) pData[2];
		
		_Data.nChunk = nChunk;
		ASSERT(_Data.nSize == 0);
		ASSERT(_Data.szFilename != NULL);
		
		nProcessed = 3;
	}
	
	ASSERT(nProcessed == 0 || nProcessed == 3);
	return(nProcessed);
}




//-----------------------------------------------------------------------------
// CJW:	The node is returning a chunk of data that we requested.  
//     -->  C<chunk*2>
//     <--  D<chunk*2><len*2><data*len>
int Node::ProcessChunkData(char *pData, int nLength)
{
	int nProcessed = 0;
	int nChunk, nLen;

	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'D');
	
	if (nLength >= 6 && _Data.nChunk > 0) {
		nChunk = 0;
		nChunk += ((unsigned char) pData[1]) << 8;
		nChunk +=  (unsigned char) pData[2];
		ASSERT(nChunk > 0);
		
		nLen = 0;
		nLen += ((unsigned char) pData[3]) << 8;
		nLen +=  (unsigned char) pData[4];
		ASSERT(nLen > 0);
		
		ASSERT(_Data.szFilename != NULL);
		ASSERT(_Data.pData == NULL);
		ASSERT(nChunk == _Data.nChunk);
		_Data.pData = (char *) malloc(nLen);
		_Data.nSize = nLen;
		
		nProcessed = 5 + nLen;
	}
	
	ASSERT(nProcessed == 0 || nProcessed > 5);
	return(nProcessed);
}


//-----------------------------------------------------------------------------
// CJW:	The node is indicating that the file is complete.
//     -->  K
int Node::ProcessFileComplete(char *pData, int nLength)
{
	ASSERT(pData != NULL && nLength > 0);
	ASSERT(pData[0] == 'K');
	
	ASSERT(_Data.szFilename != NULL);
	free(_Data.szFilename);
	_Data.szFilename = NULL;
	
	if (_Data.pData != NULL) {
		free(_Data.pData);
		_Data.pData = NULL;
		_Data.nChunk = 0;
		_Data.nSize = 0;
	}
	
	return(1);
}


