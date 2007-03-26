//-----------------------------------------------------------------------------
// serverinfo.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
// 		The ServerInfo object keeps all the information about one particular 
//		server.   It will store the address information as well as information 
//		about the last time we tried to connect to it, and the number of failed 
//		connection attempts, etc.
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

#ifndef __SERVERINFO_H
#define __SERVERINFO_H

#include "address.h"


struct ServerInfo
{
	public:
		ServerInfo();
		virtual ~ServerInfo();
		
		void ServerFailed(void);
		void ServerConnected(void);
		void ServerClosed(void);
		
	public:
    	Address *_pAddress;
		time_t  _nLastTime;
		int     _nFailed;
		bool 	_bConnected;
};


#endif

