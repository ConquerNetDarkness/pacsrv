//-----------------------------------------------------------------------------
// server.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		See "server.h" for more details about this object.
//
//-----------------------------------------------------------------------------


/***************************************************************************
 *   Copyright (C) 2003-2005 by Clinton Webb,,,                            *
 *   Copyright (C) 2006-2007 by Hyper-Active Systems,Australia,,           *
 *   pacsrv@hyper-active.com.au                                            *
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

#include "server.h"
#include "config.h"
#include "logger.h"


//-----------------------------------------------------------------------------
// CJW: Constructor.  First we initialise our client list information.  Then we 
// 		need to create the Network object, and if it initialised ok, then we 
// 		need to get the configuration info out of the config class, and start 
// 		listening on the appropriate socket.
Server::Server()
{
	Config config;
	int nPort=0;
	Logger logger;
		
	Lock();
	_Client.pList = NULL;
	_Client.nCount = 0;
	
	_pNetwork = new Network;
	if (_pNetwork != NULL) {
			
		if (_pNetwork->IsListening() == true) {
	
			if (config.Get("server", "port", &nPort) == false) {
				logger.Error("Unable to find server port information in /etc/pacsrv.conf\n");
			}
			else {
				ASSERT(nPort > 0);
				if (Listen(nPort) == true) {
					SetListening();
				} else {
					logger.Error("Cannot listen on port %d for Server.", nPort);
				}
				logger.System("Server listening on port %d", nPort);
			}
		}
	}
	
	Unlock();
}

//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Clean up the resources allocated by this object. Delete 
// 		all the client connections that we have.  We also need to delete the 
// 		network object since it was created by this instance.
Server::~Server()
{
	Logger logger;
	Lock();
	
	logger.System("Shutting down Server");
	
	ASSERT((_Client.pList == NULL && _Client.nCount == 0) || (_Client.pList != NULL && _Client.nCount > 0));
	while(_Client.nCount > 0) {
		_Client.nCount--;
		if (_Client.pList[_Client.nCount] != NULL) {
			delete _Client.pList[_Client.nCount];
			_Client.pList[_Client.nCount] = NULL;
		}
	}
	
	if (_Client.pList != NULL) {
		free(_Client.pList);
		_Client.pList = NULL;
	}
	
	if (_pNetwork != NULL) {
		delete _pNetwork;
		_pNetwork = NULL;
	}
	
	logger.System("Server shutdown complete");
	
	Unlock();
}


//-----------------------------------------------------------------------------
// CJW: An incoming connection was established, and we need to process the 
// 		socket.
void Server::OnAccept(int nSocket)
{
	Client *pTmp;
	Logger log;
	char szName[32];
	
	ASSERT(nSocket >= 0);
	
	pTmp = new Client;
	pTmp->Accept(nSocket);
	
	szName[0] = '\0';
	pTmp->GetPeerName(szName, 32);
	log.System("New Client connection received from %s.", szName);
	
	AddClient(pTmp);
}


//-----------------------------------------------------------------------------
// CJW: This is where most of the actual work is done.  Here we process all the 
// 		clients that are connected to us.  We dont need to tell the network 
// 		object to do anything special.  It is also running in a seperate 
// 		thread, and if there is anything that we need to tell the network, we 
// 		can call functions from it directly.
void Server::OnIdle(void)
{
	Lock();
	ASSERT((_Client.pList == NULL && _Client.nCount == 0) || (_Client.pList != NULL && _Client.nCount > 0));
	Unlock();
	
	ProcessClients();
}


//-----------------------------------------------------------------------------
// CJW: We have a list of clients (hopefully) that are connected to us.  The 
// 		clients dont actually need to talk to each other, but they do need to 
// 		get information from the Network. This server object will need to be a 
// 		go-between for all client/network communications.  In this function, we 
// 		will go thru the list of clients, and delete any closed clients, and 
// 		process any requests that might be waiting at a client.  The network 
// 		will cache the information that it has received, and the client will 
// 		basically indicate which file it is trying to download, and what chunk 
// 		it is now waiting for.  If the network has already retreived it, then 
// 		it will reply with the chunk.   If the chunk has not yet arrived from 
// 		the network, then the it will ask again the next time.   Very simple, 
// 		if not the most efficient.
void Server::ProcessClients(void)
{
	int i;
	int max=0;
	char *szQuery;
	int nChunk, nSize, nLength;
	char *pData;
	bool bIdle, bCheck;
	Logger log;
	
	Lock();
	
	ASSERT(_pNetwork != NULL);
	
	bIdle = false;
	while (bIdle == false) {
		bIdle = true;
		
		bCheck = CheckTime();

		// Check each client to see if there is any query we are waiting on.
		for (i=0; i<_Client.nCount; i++) {
			if (_Client.pList[i] != NULL) {
				if (_Client.pList[i]->IsClosed() == true) {
					// The client is no longer connected, so we should remove it from the list.
					log.System("Deleting client that has been closed");
					delete _Client.pList[i];
					_Client.pList[i] = NULL;
				}
				else {
					
					szQuery = NULL;
					if (_Client.pList[i]->QueryData(&szQuery, &nChunk) == true) {
						// this client had a file query request... pass it on 
						// to the network.   This doesnt mean that it is a new 
						// file.  It just means that the client is waiting for 
						// a part of a file.  The network node will determine 
						// if we have asked the network for the file.  
						
						ASSERT(szQuery != NULL);
						ASSERT(nChunk >= 0);
						
						if (_pNetwork->RunQuery(szQuery, nChunk, &pData, &nSize, &nLength) == true) {
							ASSERT(pData != NULL && nSize > 0 && nLength > 0);
							// The client doesnt really know the size of the 
							// file until we start getting some chunks.  The 
							// network will know a little bit sooner, but why 
							// make it complicated by processing that 
							// information before a chunk arrives.  Since we 
							// got some data from the network, we will pass it 
							// back to the client and then indicate that we 
							// arent idle.  If the network doesnt have the 
							// chunk yet, then we will ask again next time we 
							// loop thru.
							
							_Client.pList[i]->QueryResult(nChunk, pData, nSize, nLength);
							
							// The Idle flag just means we had some data ready 
							// to send to at least once client.  So that we 
							// can send as much information as possible to the 
							// client in each iteration (of all the clients), 
							// we want it to immediately check to see if there 
							// is more data to send to the client.
							bIdle = false;
						}
					}
				}
			}
		}

		// If this is the last one in the list, reduce the client count.
		if (_Client.nCount > 0) {
			if (_Client.pList[_Client.nCount-1] == NULL) {
				_Client.nCount--;
			}
		}

		// So that the process doesnt go wild, we will only do 50 iterations 
		// before we stop and let the rest of the system go.  Since it is a 
		// multi-threaded application we probably dont really need to do this, 
		// but we want to allow the system to accept new sockets from other 
		// clients.
		max++;
		if (max > 50) { bIdle = true; }
	}
		
	Unlock();
}


//-----------------------------------------------------------------------------
// CJW: Add the client to our internal list of clients.  We will try and re-use 
// 		any vacant slots in the existing list, but if there isnt then this new 
// 		client will be added to the end of the list (that is enlarged for it).
void Server::AddClient(Client *pClient)
{
	bool bDone = false;
	int i;
	ASSERT(pClient != NULL);
	
	Lock();
	ASSERT((_Client.pList == NULL && _Client.nCount == 0) || (_Client.pList != NULL && _Client.nCount > 0));
	for(i=0; i<_Client.nCount; i++) {
		if (_Client.pList[i] == NULL) {
			_Client.pList[i] = pClient;
			i = _Client.nCount; 
			bDone = true;
		}
	}
	
	if (bDone == false) {
		_Client.pList = (Client **) realloc(_Client.pList, sizeof(Client *) * (_Client.nCount + 1));
		ASSERT(_Client.pList != NULL);
		_Client.pList[_Client.nCount] = pClient;
		_Client.nCount++;
	}
	
	Unlock();
}

