//-----------------------------------------------------------------------------
// serverlist.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
// 		The ServerList object is pretty self-explainatory.  It maintains a list 
// 		of servers.  The amount of information we keep is rather small, but we 
// 		will keep a list of pointers to ServerInfo objects.  The pointer list 
// 		will mostly grow and rarely shrink because it takes a few failed 
// 		connection attempts in order to assume the connection has well and 
// 		truly been lost.   When the list gets really big, it could take a while 
// 		to come to this conclusion.   And it will grow because every time we 
// 		get a connection attempt, we will ensure that the server is in this 
// 		list, as well as every time that the other nodes let us know of servers 
// 		that they know about.
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

#ifndef __SERVERLIST_H
#define __SERVERLIST_H

#include "serverinfo.h"


//-----------------------------------------------------------------------------
// If we attempt to connect to a node but cannot, this is the minimum amount 
// of time we must wait before attempting that particular server again.
#define NODE_WAIT_TIME	300




class ServerList 
{
	private:
		ServerInfo **_pList;
		int _nItems;
		
		
	public:
		ServerList();
		virtual ~ServerList();
		
		void AddServer(char *szServer, int nPort);
		void AddServer(Address *pServerInfo);
		ServerInfo * GetNextServer(void);
		
    
    protected:
};


#endif

