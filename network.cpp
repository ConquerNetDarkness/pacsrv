//-----------------------------------------------------------------------------
// network.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "network.h" for more information about this class.
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

#include "network.h"
#include "config.h"
#include "logger.h"
#include "address.h"


//-----------------------------------------------------------------------------
// CJW: Constructor.  We need to get the configuration info out of the config 
//      class, and then set our listener, listening on the correct port.
Network::Network()
{
    Logger logger;
    Config config;
    char *str;
    int nPort;
    bool bDirect;
    char *szServer;

    Lock();
    _nNextNodeID = 1;
    _pNodes = NULL;
    
    _pServerList = new ServerList;
    ASSERT(_pServerList != NULL);
    
    _pFileList = new FileList;
    ASSERT(_pFileList != NULL);
    
    if (config.Get("network", "min-connections", &_Connections.nMin) == false) {
        _Connections.nMin = 3;
    }
    
    if (config.Get("network", "max-connections", &_Connections.nMax) == false) {
        _Connections.nMax = 30;
    }
    
    // We cant really function with a value less than 1.  We will choose the
    // default if that is so.
    if (_Connections.nMin < 1) {
        _Connections.nMin = 3;
    }
    if (_Connections.nMax <= _Connections.nMin) {
        if (_Connections.nMin >= 30) {
            _Connections.nMax = _Connections.nMin + 5;
        }
        else {
            _Connections.nMax = 30;
        }
    }
    
    // Check our config to see if we are allowed to query the webserver for an
    // ip address of a node we can connect to.
    _Connections.bWebQuery = true;
    if (config.Get("network", "web-query", &str) == true) {
        ASSERT(str != NULL);
        if (strcmp(str, "no") == 0) {
            _Connections.bWebQuery = false;
        }
        free(str);  str = NULL;
    }
    
    // Check our config to see if we have a direct server connecton we're
    // supposed to establish.  Note, direct doesnt mean exclusive.
    bDirect = false;
    if (config.Get("network", "direct", &str) == true) {
        ASSERT(str != NULL);
        if (strcmp(str, "yes") == 0) {
            bDirect = true;
        }
        free(str);  str = NULL;
    }
    
    if (bDirect == true) {
        szServer = NULL;
        config.Get("direct", "server", &szServer);
        ASSERT(szServer != NULL);
        config.Get("direct", "port", &str);
        ASSERT(str != NULL);
        nPort = atoi(str);
        ASSERT(nPort > 0);
    
        _pServerList->AddServer(szServer, nPort);
    
        free(szServer); szServer = NULL;
        free(str);      str = NULL;
    
    }

    _nPort = 0;
    if (config.Get("network", "port", &nPort) == false) {
        logger.Error("Unable to find network port information in /etc/pacsrv.conf\n");
    }
    else {
        ASSERT(nPort > 0);
        if (Listen(nPort) == true) {
            _nPort = nPort;
            SetListening();
        } else {
            logger.Error("Cannot listen on port %d for Network.", nPort);
        }
        logger.System("Network listening on port %d", nPort);
    }

    // every 5 seconds, we need to go through our filelist.  This variable will be used to trigger it.
    _tLastFileListCheck = time(NULL);
    
    Unlock();
}

//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Clean up the resources allocated by this object.
//      Delete all the client connections that we have.
Network::~Network()
{
    Node *pTmp;
    
    Lock();
    
    ASSERT(_pServerList != NULL);
    delete _pServerList;
    _pServerList = NULL;
    
    ASSERT(_pFileList != NULL);
    delete _pFileList;
    _pFileList = NULL;
    
    while(_pNodes != NULL) {
        pTmp = _pNodes->GetNext();
        delete _pNodes;
        _pNodes = pTmp;
    }
    
    Unlock();
}

//-----------------------------------------------------------------------------
// CJW: Go through the list of nodes and count how many we have.  This is
//      returning the total number regardless of the state of the node.  is
//      that really what we want?   The closed nodes will be removed at some
//      point anyway, so it should be ok.
int Network::GetNodeCount(void)
{
    Node *pTmp;
    int nCount = 0;

    pTmp = _pNodes;
    while(pTmp != NULL) {
        pTmp = pTmp->GetNext();
        nCount++;
    }

    return(nCount);
}


//-----------------------------------------------------------------------------
// CJW: Go through the list of nodes and count how many we have that are 
//      actually connected.
int Network::GetConnectionCount(void)
{
    Node *pTmp;
    int nCount = 0;

    pTmp = _pNodes;
    while(pTmp != NULL) {
        if (pTmp->IsConnected() == true) {
            nCount++;
        }
        pTmp = pTmp->GetNext();
    }

    return(nCount);
}



//-----------------------------------------------------------------------------
// CJW: An incoming connection was established, we need to pass it to an object
//      that can handle it.
void Network::OnAccept(int nSocket)
{
    Node *pTmp;
    Logger log;
    char szName[32];
    int nID;
    
    ASSERT(nSocket >= 0);
    
    pTmp = new Node;
    pTmp->Accept(nSocket);

    nID = AddNode(pTmp);
    
    szName[0] = '\0';
    pTmp->GetPeerName(szName, 32);
    log.System("[Node:%d] New Node connection received from %s.", nID, szName);

}

//-----------------------------------------------------------------------------
// CJW: This is where most of the actual work is done.  We need to connect to
//      at least one other server, we need to go thru our list of clients to
//      find any that are marked for deletion.   If we make a new connection to
//      another server, we need to send it a copy of all the outstanding
//      queries that we are currently processing locally.
void Network::OnIdle(void)
{
    CheckConnections();
    ProcessNodes();
    ProcessFileList();
}


//-----------------------------------------------------------------------------
// CJW: Check every second to make sure that we have the required number of
//      connections to other servers.  If not, then we will try to establish a
//      connection to another node.  If we have MORE than the minimum number
//      of connections, we will make a note of the ones that are idle, and the
//      one that has the lowest bandwidth recorded.  That connection will be
//      dropped.  First we must ensure that the connection is at least 30
//      seconds old though.  We want to get connections at least that amount
//      of time.
void Network::CheckConnections(void)
{
    time_t nCurrent;
    int count;
    
    Lock();
    
    ASSERT(_Connections.nMin > 0);
    ASSERT(_Connections.nMax > _Connections.nMin);
    
    nCurrent = time(NULL);
    if (nCurrent > _nLastCheck) {
    
        // go thru the list and count the number of current connections.
        count = GetConnectionCount();
    
        // if we dont have ANY clients, then we need to connect to the starter connection in our config.
        if (count < _Connections.nMin) {
            ConnectStarter();
        }
        else {
            // Now we need to check the number of clients that we have.  If we
            // have too many, then we need to choose the lowest performers
            // that are not actually transfering a file, and close that
            // connection.
    
            if (count > _Connections.nMax) {
                CloseSlowConnection();
            }
        }
    
        _nLastCheck = time(NULL);
    }
    
    Unlock();
}


//-----------------------------------------------------------------------------
// CJW: This function will be called when we dont have our minimum number of 
// 		connections.  We will need to go thru the list of servers that we have 
// 		an try to connect to any that we can.  If there isnt any in the list 
// 		we can use, then we will have to wait till we get told about some new 
// 		connections.
bool Network::ConnectStarter(void)
{
	// TODO:
	return false;
}


//-----------------------------------------------------------------------------
// CJW: If we have more than the minimum number of connections, we need to 
//      start closing the ones that are idle.  Determining which connection
//      should be closed can be a bit tricky and complicated, so is not
//      currently being done.   For now, we will just go thru the list and
//      pick the oldest idle connection and remove it.  If we cant easily
//      remove the connection, then there is no real harm done if we dont.
//
//      It is assumed that it is correct to close the connection, no checking
//      will be made to ensure that the required number of connections are
//      maintained.
void Network::CloseSlowConnection(void)
{
    Node *pTmp, *pPrev;
    Logger *pLogger;
    
    pPrev = NULL;
    pTmp = _pNodes;
    while (pTmp != NULL) {
        pPrev = pTmp;
        pTmp = pTmp->GetNext();
    }
    
    if (pPrev != NULL) {
        pTmp = pPrev->GetNext();
        ASSERT(pTmp->GetNext() == NULL);
        if (pTmp->GetIdleSeconds() > MAX_IDLE_TIME) {
            pLogger = new Logger;
            pLogger->System("[Network] Deleting idle node %d.", pTmp->GetID());
            delete pLogger;
    
            pPrev->SetNext(NULL);
    
            ProcessFinal(pTmp);
            delete pTmp;
        }
    }
}




//-----------------------------------------------------------------------------
// CJW: We have a list of nodes (hopefully) that are connected to other servers
//      in the network.  Since the nodes themselves dont actually talk to each
//      other, this network object will need to be a go-between for all
//      intra-node communications.
//
//      First, we will go thru the list of nodes, telling each to process its
//      current socket queues.
//
//      Then we will ask the node if it has received any chunks.  Then asks the
//      node what file it is currently receiving (if there weren't any chunks).
//      If the file has some chunks needed, we will ask the node to request a
//      chunk.   If we have no more chunks needed, then we will tell the node
//      that the file is complete.
//
//      If the node is not processing any file, we will ask the node to 
//      request the next file in the list to be downloaded.   We will not 
//      actually ask the node to
//
//      We will also ask the node if it has received any serverlist
//      information.  If so, it adds it to our main server-list.
//
//		Finally we will need to ask the node if it has received any GotFile 
//		notifications.  If it has, then we need to pull out the next node it 
//		needs to be passed onto, and if we still have a connection to it 
//		(which we most likely do), then we will pass that information onto 
//		that node.
void Network::ProcessNodes(void)
{
    int max=0;
    bool bIdle = false;
    Node *pTmp;
    char *szFilename;
    int nChunk;
    int nSize;
    char *pData;
    bool bClosed = false;
	FileInfo *pInfo;
	Address *pServerInfo;
	strFileRequest *pReq;
	strFileReply *pReply;
	char *szLocalFile;
    
    Lock();
    
    while (bIdle == false) {
        bIdle = true;

        pTmp = _pNodes;
        while (pTmp != NULL) {
            if (pTmp->IsConnected() == true) {
    
                // Process the node.
                if (pTmp->Process() == true) {
                    bIdle = false;
                }
    
                // Has node received any chunks?  Save them all if so.
                szFilename = NULL;
                if (pTmp->GetChunks(&szFilename, &pData, &nChunk, &nSize)) {
					ASSERT(szFilename != NULL);
					ASSERT(pData != NULL);
                    SaveChunk(szFilename, pData, nChunk, nSize);
                    bIdle = false;
                }
    
                // Ask node what file it is receiving.
                if (szFilename == NULL) {
                    pTmp->GetCurrentFile(&szFilename);
                }
    
                if (szFilename != NULL) {
					ASSERT(_pFileList != NULL);
					pInfo = _pFileList->GetFileInfo(szFilename);
					ASSERT(pInfo != NULL);

                    if (GetNextChunk(szFilename, &nChunk) == true) {
                        // If file has chunks needed, ask node for the next chunk.
                        pTmp->RequestChunk(nChunk);
						pInfo->ChunkRequested(nChunk, pTmp->GetID());
                        bIdle = false;
                    }
                    else {
                        // If file does not have chunks needed, tell node that 
                        // file is complete.
						pInfo->FileComplete();
                        pTmp->FileComplete();
                    }
                }
                else {
                    // If node is not processing any file, ask the node to 
                    // request the next file in the list to be downloaded.
                    if (pTmp->ReadyForFile() == true) {
                        if (GetNextFile(&szFilename) == true) {
                            pTmp->RequestFile(szFilename);
                            bIdle = false;
                        }
                    }
                }
    
                // Ask node if it has received any serverlist information.
				pServerInfo = pTmp->GetServerInfo();
                if (pServerInfo != NULL) {
                    _pServerList->AddServer(pServerInfo);
                }
				
				// Ask the node if it has received a remote file request that 
				// needs to be passed on to the other nodes
				pReq = pTmp->GetFileRequest();
				if (pReq != NULL) {
					RelayFileRequest(pReq);
					
					ASSERT(_pFileList != NULL);
					pInfo = _pFileList->GetFileInfo(pReq->szFile);
					if (pInfo == NULL) {
						pInfo = _pFileList->LoadFile(pReq->szFile);
					}
					
					if (pInfo != NULL) {
						pTmp->ReplyFileFound(pReq, pInfo);
					}

					delete pReq;
				}
				
				// Ask the node if it has received a reply for a file request.  
				// If it did, then we got some information back, that we need 
				// to pass back to the node that we originally got it from.
				pReply = pTmp->GetFileReply();
				if (pReply != NULL) {
					RelayFileReply(pReply);
					delete pReply;
				}
				
				
				szLocalFile = pTmp->GetLocalFile();
				if (szLocalFile != NULL) {
					ASSERT(_pFileList != NULL);
					pInfo = _pFileList->GetFileInfo(szLocalFile);
					if (pInfo == NULL) {
						pInfo = _pFileList->LoadFile(pReq->szFile);
					}
					
					if (pInfo != NULL) {
						pTmp->SendFile(szLocalFile, pInfo);
					}
					else {
						pTmp->LocalFileFail(szLocalFile);
					}
					
					free(szLocalFile);
				}
            }
            else {
                bClosed = true;
            }
    
            pTmp = pTmp->GetNext();
        }
    
        max++;
        if (max > 50) {
            bIdle = true;
        }
    }
    
    // Now we will delete any closed nodes if we noticed any while we were processing.
    if (bClosed == true) {
        RemoveClosedNodes();
    }
    
    Unlock();
}


//-----------------------------------------------------------------------------
// CJW: Add the node to our linked list of nodes.  We will actually be adding 
//      the node to the top of the list, since there is no real advantage in
//      finding the bottom of the list to put it there.  All nodes will get a
//      chance at communicating, so all should be ok.
int Network::AddNode(Node *pNode)
{
    Logger *pLog;
    int nID;
    
    ASSERT(pNode != NULL);
    
    Lock();
    
    nID = _nNextNodeID;
    _nNextNodeID ++;
    if (_nNextNodeID > MAX_NODE_ID) {
        _nNextNodeID = 1;
        pLog = new Logger;
        pLog->System("NodeID counter reached maximum.  Restarting counter at 1.");
        delete pLog;
    }
    
    pNode->SetID(nID);
    
    if (_pNodes != NULL) {
        pNode->SetNext(_pNodes);
    }
    _pNodes = pNode;
    
    Unlock();
    
    ASSERT(nID > 0 && nID <= MAX_NODE_ID);
    ASSERT(_nNextNodeID > 0 && _nNextNodeID <= MAX_NODE_ID);
    return(nID);
}


//-----------------------------------------------------------------------------
// CJW: Here we need to go thru the list of nodes and remove any that have been 
//      closed.  We are going to assume that the object is locked while we are
//      doing this.
void Network::RemoveClosedNodes(void)
{
    Node *pTmp, *pPrev;
    
    pPrev = NULL;
    pTmp = _pNodes;
    while (pTmp != NULL) {
        if (pTmp->IsConnected() == false) {
            if (pPrev == NULL) {
                _pNodes = pTmp->GetNext();
    
                ProcessFinal(pTmp);
                delete pTmp;
                pTmp = _pNodes;
            }
        }
        else {
			pPrev = pTmp;
            pTmp = pTmp->GetNext();
        }
    }
}


//-----------------------------------------------------------------------------
// CJW: This function is called by the Server object to ask for a particular 
//      file chunk.  We will look in our files list, if it isnt in the list
//      then we will add it, and ask the network for the file.  If we have the
//      file in the list, but we dont have that particular chunk we will
//      return a false.  If we do have it, we will return with that information.
//      Since this function is called from outside the scope of this thread,
//      we have to make sure that we are thread-safe.
bool Network::RunQuery(char *szQuery, int nChunk, char **pData, int *nSize, int *nLength)
{
    bool bGotChunk = false;
    FileInfo *pInfo;
    Node *pNode;
    
    ASSERT(szQuery != NULL && nChunk >= 0 &&  pData != NULL);
    ASSERT(nSize != NULL && nLength != NULL);
    
    Lock();
	
	ASSERT(_pFileList != NULL);
    
    // Check the file list to see if the file is in it.
    pInfo = _pFileList->GetFileInfo(szQuery);
    if (pInfo == NULL) {
        // If it isn't, check to see if we have it locally.
        ASSERT(nChunk == 0);
        pInfo = _pFileList->LoadFile(szQuery);
    }
    
    if (pInfo != NULL) {
        // If we have the file in our list, check to see if we have this chunk.
        if (pInfo->GetChunk(nChunk, pData, nSize, nLength) == true) {
            // if we do, create the memory chunk to be used.  return true.
            bGotChunk = true;
            ASSERT(*pData != NULL);
            ASSERT(*nSize > 0);
            ASSERT(*nLength > 0);
        }
    }
    else {
    	// If we still havent found it, query all the nodes for it.
		ASSERT(nChunk == 0);
    
        _pFileList->AddFile(szQuery);
    
        pNode = _pNodes;
        while(pNode != NULL) {
            if (pNode->IsConnected() == true) {
                pNode->RequestFileFromNetwork(szQuery);
            }
            pNode = pNode->GetNext();
        }
    
        // We will return false because it is highly unlikely that we have 
        // already received parts of this file.
		ASSERT(bGotChunk == false);
    }
    
    Unlock();
    
    return(bGotChunk);
}


//-----------------------------------------------------------------------------
// CJW: We have some details of a server we can try and connect to.  If we are 
//      able to connect (and create a node, then we will add it to the list
//      and then return true.  If we were unable to connect, then we will
//      return false.
bool Network::ConnectNode(ServerInfo *pInfo)
{
    bool bConnected = false;
    Node *pNode;
    char *szServer;
    int nPort;
    Logger log;
    
    ASSERT(pInfo != NULL);
    
    szServer = pInfo->_pAddress->GetServer();
    nPort = pInfo->_pAddress->GetPort();
    
    ASSERT(szServer != NULL && szServer[0] != '\0' && nPort > 0);
    
    pNode = new Node;
    ASSERT(pNode != NULL);
    
    log.System("[Network] Attempting to connect to %s:%d", szServer, nPort);
    if (pNode->Connect(szServer, nPort) == false) {
        log.System("[Network] Unable to connect to %s:%d", szServer, nPort);
        delete pNode;
        pInfo->ServerFailed();
    }
    else {
        log.System("[Network] Connected to %s:%d", szServer, nPort);
        pNode->SetNext(_pNodes);
        _pNodes = pNode;
        pInfo->ServerConnected();
        bConnected = true;
    }
    
    return(bConnected);
}


//-----------------------------------------------------------------------------
// CJW: We have a list of files that are being received over the network (or
//      locally being supplied to a client).  Every 5 seconds, we should go
//      through the file list and remove any that have been completed and dont
//      have any pending connections receiving the information.
void Network::ProcessFileList(void)
{
    time_t tNow;
    
    Lock();

    ASSERT(_pFileList != NULL);
    tNow = time(NULL);
    if ((tNow - _tLastFileListCheck) >= FILE_LIST_CHECK) {
        _pFileList->Process();
        _tLastFileListCheck = tNow;
    }

    Unlock();
}


//-----------------------------------------------------------------------------
// CJW: If we are about to close a slow connection, or if it has already been 
// 		closed by the other side, we need to process the information it has, 
// 		before we destroy the object.  There may be chunks of data, or 
// 		Serverlist information that was sent before the connection was closed.  
// 		We dont really care about any messages that require a response.  
// 		Additionally we need to let the file-list know that this node is being 
// 		removed, so that it can remove any outstanding chunks that were 
// 		allocated to this node, so that it can be allocated again.
void Network::ProcessFinal(Node *pNode)
{
	char *szFilename;
	char *pData;
	int nChunk;
	int nSize;
	bool bDone;
	Address *pServerInfo;
	int nID;
	
	ASSERT(pNode != NULL);
	
	bDone = false;
	while (bDone == false) {
		bDone = true;
	
		pNode->Process();
		
		szFilename = NULL;
		if (pNode->GetChunks(&szFilename, &pData, &nChunk, &nSize) == true) {
			SaveChunk(szFilename, pData, nChunk, nSize);
			bDone = false;
		}
		
		pServerInfo = pNode->GetServerInfo();
		if (pServerInfo != NULL) {
			_pServerList->AddServer(pServerInfo);
			bDone = false;
		}
	}
	
	nID = pNode->GetID();
	_pFileList->RemoveNode(nID);
}


//-----------------------------------------------------------------------------
// CJW: We have retrieved a chunk from a node, so we need to save it to our 
// 		file list.  If the file doesnt exist in our internal file list, then we 
// 		will add it.  The data should not be free'd, because the FileInfo 
// 		object will now control that block of memory (rather than creating a 
// 		new block and copying the data over, etc).
void Network::SaveChunk(char *szFilename, char *pData, int nChunk, int nSize)
{
	FileInfo *pInfo;
	
	ASSERT(szFilename != NULL);
	ASSERT(pData != NULL);
	ASSERT(nChunk >= 0);
	ASSERT(nSize > 0);
	
	ASSERT(_pFileList != NULL);
	
	pInfo = _pFileList->GetFileInfo(szFilename);
	if (pInfo == NULL) {
		pInfo = _pFileList->AddFile(szFilename);
	}
	ASSERT(pInfo != NULL);
	
	pInfo->SaveChunk(pData, nChunk, nSize);
}


//-----------------------------------------------------------------------------
// CJW: This function is being asked for the next chunk that needs to be asked 
// 		for.  We will return true if there is another chunk for this file.  
// 		Return false if there are no more chunks needed for the file.
bool Network::GetNextChunk(char *szFilename, int *nChunk)
{
	bool bMore = false;
	FileInfo *pInfo;
	
	ASSERT(szFilename != NULL);
	ASSERT(nChunk != NULL);
	ASSERT(_pFileList != NULL);
	
	pInfo = _pFileList->GetFileInfo(szFilename);
	if (pInfo != NULL) {
		if (pInfo->GetNextChunk(nChunk) == true) {
			ASSERT(*nChunk >= 0);
			bMore = true;
		}
	}
	
	return(bMore);
}


//-----------------------------------------------------------------------------
// CJW: Return the filename of the next file that we need to download.   To do 
// 		this we look at our File list for the next file in the list that is 
// 		incomplete and not local.  Return true if we found a file to request, 
// 		return false if we didnt.
bool Network::GetNextFile(char **szFilename)
{
	bool bFound = false;
	FileInfo *pInfo;
	
	ASSERT(szFilename != NULL);
	ASSERT(_pFileList != NULL);
	
	pInfo = _pFileList->GetNextFile();
	if (pInfo != NULL) {
		*szFilename = pInfo->GetFilename();
		ASSERT(*szFilename != NULL);
		ASSERT(pInfo->IsLocal() == false);
		bFound = true;
	}
	
	return(bFound);
}


//-----------------------------------------------------------------------------
// CJW: We've received a file request from a node.  We need to relay this info 
// 		on to our other nodes (if our ttl is greater than zero).  However, we 
// 		dont want to send it back to the node that we just got it from, so we 
// 		send it to everything except that one.  We also dont want to send it to 
// 		any server that is already on the list of hosts that is inside this 
// 		message packet.
//
//		F<hops><ttl><flen><file*flen><host*6>...<host*6>
void Network::RelayFileRequest(strFileRequest *pReq)
{
	Node *pNode;
	int i, j;
	unsigned char buffer[2048];
	Address *pAddr;
	bool bSend;
	unsigned char tmp[6];

	ASSERT(pReq != NULL);
	ASSERT(_pNodes != NULL);	// must have at least one, because we got this message from somewhere.
	
	// First we build our message because it is going to be the same for each node.
	i=0;
	buffer[i++] = 'F';
	buffer[i++] = pReq->nHops;
	buffer[i++] = pReq->nTtl;
	buffer[i++] = pReq->nFlen;
	for (j=0; j<pReq->nFlen; j++) {
		buffer[i++] =  pReq->szFile[j];
	}
	for (j=0; j<pReq->nHops; j++) {
		pReq->pHosts[j]->Get(&buffer[i]);
		i += 6;
	}
	
	// Then we go thru the nodes and tell each one to send the message.
	pNode = _pNodes;
	while(pNode != NULL) {
		if (pNode->IsConnected() == true) {
			
			// check that it is not from a node that has already got this message
			pAddr = pNode->GetServerInfo();
			if (pAddr != NULL) {
				
				bSend = true;
				for (j=0; j<pReq->nHops && bSend == true; j++) {
					pReq->pHosts[j]->Get(tmp);			
					if (pAddr->IsSame(tmp) == false) {
						bSend = false;
					}
				}
				
				if (bSend == true) {
					pNode->SendMsg((char *)buffer, i);
				}
			}
		}
		pNode = pNode->GetNext();
	}
}



//-----------------------------------------------------------------------------
// CJW: We've received a file reply from a node.  We need to relay this info 
// 		on to the last host in the list.  our other nodes (if our ttl is greater than zero).  However, we 
// 		dont want to send it back to the node that we just got it from, so we 
// 		send it to everything except that one.
//
//		G<hops><flen><file*flen><target*6><host*6>...<host*6>
void Network::RelayFileReply(strFileReply *pReply)
{
	Node *pNode;
	unsigned char pNextAddress[6];
	Address *pAddr;
	int i, j;
	unsigned char buffer[2048];
	
	ASSERT(pReply != NULL);
	ASSERT(_pNodes != NULL);
	
	ASSERT(pReply->nHops > 0);
	pReply->pHosts[pReply->nHops-1]->Get(pNextAddress);
	
	
	// First we build our message because it is going to be the same for each node.
	i=0;
	buffer[i++] = 'G';
	buffer[i++] = pReply->nHops-1;
	buffer[i++] = pReply->nFlen;
	for (j=0; j<pReply->nFlen; j++) {
		buffer[i++] =  pReply->szFile[j];
	}
	for (j=0; j<pReply->nHops-1; j++) {
		pReply->pHosts[j]->Get(&buffer[i]);
		i += 6;
	}
	
	ASSERT(i < 2048);
	
	// Go thru the nodes and find the one we need to send the message to.
	pNode = _pNodes;
	while(pNode != NULL) {
		if (pNode->IsConnected() == true) {
			pAddr = pNode->GetServerInfo();
			if (pAddr != NULL) {
				if (pAddr->IsSame(pNextAddress) == true) {
					pNode->SendMsg((char *)buffer, i);
				}
			}
		}
		pNode = pNode->GetNext();
	}
}




