//-----------------------------------------------------------------------------
// client.h
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      NOTE: This class is not to be used by the Client APP.  It is used by
//            the server to communicate with the client.   All the client-app
//            code is in the single pacsrvclient.cpp file.
//
//      This class is used to process the connection from the client app.  It
//      basically receives very simple file requests, and will reply with
//      chunks of the file in actual file sequence.  Because of the way pacman
//      works, the client connection will only be used to transfer one file.
//      So the states will follow a very simple linear transition.
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


#ifndef __CLIENT_H
#define __CLIENT_H

#include "common.h"
#include "baseclient.h"

class Client : public BaseClient
{
    public:
        Client();
        virtual ~Client();
    
//         bool Process(bool bCheck=false);
        bool QueryData(char **szQuery, int *nChunk);
        void QueryResult(int nChunk, char *pData, int nSize, int nLength);
    
    protected:
    
   		virtual int OnReceive(char *pData, int nLength);
		virtual void OnIdle(void);
		virtual void OnClosed(void);
		virtual void OnStalled(char *pData, int nLength);

    
    private:
        void ProcessInit(char *pData, int nLength);
        bool ProcessHeartbeat(void);
        void ProcessChunkReceived(char *pData, int nLength);
        int ProcessFileRequest(char *pData, int nLength);
    
        char *_szQuery;
        int   _nChunk;
        int   _nLength;
    
        struct strHeartbeat _Heartbeat;
        unsigned _nVersion;        // protocol version.
        int _nClientID;
        static int _nNextClientID;
};


#endif

