//-----------------------------------------------------------------------------
// server.h
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		This class is used to listen on a socket for incoming connections from 
//		the client application.  It will be created by the Network object which 
//		will periodically call the Process function which will return filenames 
//		that it is expecting.
//
//		When a connection comes in, it will create a new client object, and 
//		pass the socket connection to that.  This class is a thread-object and 
//		will continue to idle until connections arrive.  
//
//		The client object will ask for a particular file.  The Network will 
//		then pass that query to all the other connected nodes.  As peices of 
//		the file arrives, they will be sent to the client object which will 
//		send it to the client application.  The client application will then 
//		need to save that file as it arrives, to the proper location.
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

#ifndef __SERVER_H
#define __SERVER_H

#include "baseserver.h"
#include "client.h"
#include "network.h"

class Server : public BaseServer
{
	public:
		Server();
		virtual ~Server();
		
	protected:
		virtual void OnAccept(int);
		virtual void OnIdle(void);
		
	private:
		void CheckConnections(void);
		void ProcessClients(void);
		void AddClient(Client *pClient);
		
		struct {
			Client **pList;
			int nCount;
		} _Client;
		Network *_pNetwork;
};


#endif

