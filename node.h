//-----------------------------------------------------------------------------
// node.h
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      This class is used to connect to other servers.   The list of nodes is
//      maintained by a linked list.  So each node will have a little bit of
//      control of the node that is connected to it.  This node will
//      communicate with another daemon, periodically asking it questions, such
//      as a list of other nodes, etc.   A PING will be sent to the other node
//      every 5 seconds, and we will expect a reply.  We will time how long it
//      takes for the reply to get back (will need a high resolution timer
//      because I'm sure even the slowest connection will be able reply in less
//      than a second.  Also, since the nodes are processed serially, the
//      reply will not be very good actually.
//
//      For starters, the ping/reply is not very important.  It can be added
//      later to segment the network into more effient cells, rather than just
//      pick randome nodes to talk to.
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

#ifndef __NODE_H
#define __NODE_H


#include "baseclient.h"
#include "address.h"
#include "fileinfo.h"


struct strFileRequest {
	unsigned char nHops;
	unsigned char nTtl;
	unsigned char nFlen;
	char szFile[256];
	Address **pHosts;
	
	strFileRequest() {
		nHops = 0;
		nTtl = 0;
		nFlen = 0;
		szFile[0] = '\0';
		pHosts = NULL;
	}
	
	virtual ~strFileRequest() {
		if (pHosts != NULL) {
			while(nHops > 0) {
				nHops--;
				if (pHosts[nHops] != NULL) {
					delete pHosts[nHops];
				}
			}
		}
		ASSERT(nHops == 0);
	}
};


struct strFileReply {
	unsigned char nHops;
	unsigned char nFlen;
	char szFile[256];
	Address *pTarget;
	Address **pHosts;
	
	strFileReply() {
		nHops = 0;
		nFlen = 0;
		szFile[0] = '\0';
		pTarget = NULL;
		pHosts = NULL;
	}
	
	virtual ~strFileReply() {
		if (pTarget != NULL) {
			delete pTarget; pTarget = NULL;
		}
		
		if (pHosts != NULL) {
			while(nHops > 0) {
				nHops--;
				if (pHosts[nHops] != NULL) {
					delete pHosts[nHops];
				}
			}
		}
		
		ASSERT(nHops == 0);
	}
};





class Node : public BaseClient
{
    public:
        Node();
        virtual ~Node();
    
        virtual void Accept(SOCKET nSocket);
    
        bool Process(void);
    
        Node * GetNext(void);
        void SetNext(Node *ptr);
        int GetIdleSeconds(void);
        int GetID(void);
        void SetID(int nID);
    
        bool GetChunks(char **szFilename, char **pData, int *nChunk, int *nSize);
        void GetCurrentFile(char **szFilename);
        void FileComplete(void);
    
        void RequestChunk(int nChunk);
        bool ReadyForFile(void);
        void RequestFile(char *szFilename);
		void RequestFileFromNetwork(char *szFilename);
		
		void ReplyFileFound(strFileRequest *pReq, FileInfo *pInfo);
    
        Address * GetServerInfo(void);
		strFileRequest * GetFileRequest(void);
		strFileReply * GetFileReply(void);
		
		void SendMsg(char *ptr, int len);
		
		char *GetLocalFile(void);
		void SendFile(char *szLocalFile, FileInfo *pInfo);
		void LocalFileFail(char *szLocalFile);
    
    protected:
    
    private:
        bool ProcessInit(int nLength);
        bool ProcessValid(int nLength);
        bool ProcessQuit(int nLength);
        bool ProcessServer(int nLength);
        bool ProcessPing(int nLength);
        bool ProcessPingReply(int nLength);
        bool ProcessFileRequest(int nLength);
        bool ProcessFileGot(int nLength);
        bool ProcessLocalFile(int nLength);
        bool ProcessLocalOK(int nLength);
        bool ProcessLocalFail(int nLength);
        bool ProcessChunkRequest(int nLength);
        bool ProcessChunkData(int nLength);
        bool ProcessFileComplete(int nLength);

        void ProcessHeartbeat(void);

        Node *_pNext;
        int _nID;
        time_t _nLastActivity;
        struct {
            bool bAccepted;
			bool bInit;
			bool bClosed;
			bool bValid;
        } _Status;
		
		// TODO: for now we will only save one chunk at a time.  But in the future we may have multiple chunks as we try to optimise our requests from network nodes that have parts of the file.
		struct {
			char *szFilename;
			char *pData;
			int nChunk;
			int nSize;
			FileInfo *pFileInfo;
		} _Data;
		
		struct {
			int nBeats;     // number of beats we have missed.
			int nDelay;     // number of seconds before we indicate that we have missed a beat.
			time_t nLastCheck;
		} _Heartbeat;
		
		struct strFileRequest *_pFileRequest;
		struct strFileReply *_pFileReply;
		
		Address *_pServerInfo;
		Address *_pRemoteNode;
		
		char *_szLocalFile;
};


#endif

