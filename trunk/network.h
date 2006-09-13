//-----------------------------------------------------------------------------
// network.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
//      This class is used to listen on a socket for incoming connections from
//      other pacsrv systems in the network.
//
//      When a connection comes in, it will create a new node object, and
//      pass the socket connection to that.  This class is a thread-object and
//      will continue to idle until connections arrive.  The Idle functionality
//      will be used to try and maintain a connection of up to 3 other servers.
//      To do this, it will check the existing connections, and will also query
//      the CGI script for more IP addresses.  It will periodically send a PING
//      message to each server and will make a note of the time it took to get
//      a reply.  The server with the slowest ping times will be dropped if a
//      faster connection can be found, this makes it for a self-adjusting
//      network with the slowest connections continuously being dropped so that
//      the fastest can stay closer together.  Please note that the slow
//      connection will not be dropped until a new connection has been
//      established that is faster.
//
//      While idling, the network will attempt to keep at least the minimum
//      connections.  If the minimum is obtained, it will not connect to any
//      others.  However, as requests are made and filled, new connections will
//      be obtained, either connecting directly, or by receiving connections
//      from other nodes.   Every 60 seconds, if we have more than the minimum
//      connections, we will look at all the nodes in our list.  If any of them
//      have been idle for more than 5 minutes, it will drop the one with the
//      lowest transfer rate or PING latency.
//
//      DESIGN QUESTION
//      Should we use an array to keep track of the nodes, or should we use a
//      linked list?   Since the node connections will probably fluctuate a
//      lot, it might be the best, but it could also be a bit more maintenance.
//      Should we control the list externally?
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

#ifndef __NETWORK_H
#define __NETWORK_H

#include <string.h>
#include <stdlib.h>
#include <DevPlus.h>

#include "baseserver.h"
#include "node.h"
#include "serverlist.h"
#include "filelist.h"


//-----------------------------------------------------------------------------
// Length of time that a node must be idle before it is considered for closing.
#define MAX_IDLE_TIME   30

//-----------------------------------------------------------------------------
// Each node will be given an ID so that it can be tracked in the logs easier.  
// The node will also be used to indicate which node is being asked for what 
// chunk.  THat way if the connection is lost, we know to ask that chunk again.
#define MAX_NODE_ID     1000000000

//-----------------------------------------------------------------------------
// 
#define FILE_LIST_CHECK     5


class Network : public BaseServer
{
    public:
        Network();
        virtual ~Network();
    
        bool RunQuery(char *szQuery, int nChunk, char **pData, int *nSize, int *nLength);
    
    protected:
        virtual void OnAccept(int);
        virtual void OnIdle(void);
    
    private:
        void CheckConnections(void);
        void ProcessFileList(void);
        bool ConnectStarter(void);
        bool ConnectNode(ServerInfo *pInfo);
        void CloseSlowConnection(void);
        void ProcessNodes(void);
        void ProcessFinal(Node *pNode);
        void RemoveClosedNodes(void);
        int AddNode(Node *pNode);
        int GetNodeCount(void);
        int GetConnectionCount(void);
    
        void SaveChunk(char *szFilename, char *pData, int nChunk, int nSize);
        bool GetNextChunk(char *szFilename, int *nChunk);
        bool GetNextFile(char **szFilename);
		
		void RelayFileRequest(strFileRequest *pReq, Node *pNode);
    
        struct {
            bool bWebQuery;
            int  nMin, nMax;
        } _Connections;
        
        ServerList *_pServerList;
        FileList *_pFileList;
        Node *_pNodes;
        int _nNextNodeID;
        int _nPort;
        time_t _tLastFileListCheck;
};


#endif

