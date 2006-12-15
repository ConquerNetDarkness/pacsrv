//-----------------------------------------------------------------------------
// serverinfo.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "serverinfo.h" for more information about this class.
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


#include "serverinfo.h"


//-----------------------------------------------------------------------------
// CJW: Constructor.  Initialise our member variables.
ServerInfo::ServerInfo() 
{
	_pAddress = NULL;
	_nLastTime = 0;
	_nFailed = 0;
	_bConnected = false;
}
		
//-----------------------------------------------------------------------------
// CJW: Deconstructor... nothing to do here.
ServerInfo::~ServerInfo()
{
	if (_pAddress != NULL) {
		delete _pAddress;
		_pAddress = NULL;
	}
}
		
//-----------------------------------------------------------------------------
// CJW: This function will be called if we try and connect to the server, but 
// 		the connection fails.  We need to increment our failed counters.
void ServerInfo::ServerFailed(void) 
{
	ASSERT(_bConnected == false);
	ASSERT(_nLastTime > 0);
	_nFailed++;
}
		
//-----------------------------------------------------------------------------
// CJW: This function will be called when we establish a connection with the 
// 		server.
void ServerInfo::ServerConnected(void)
{
	ASSERT(_bConnected == false);
	ASSERT(_nLastTime > 0);
	_bConnected = true;
	_nFailed = 0;
}
		

//-----------------------------------------------------------------------------
// CJW: This function will be called when we close a connection that had been 
// 		established.
void ServerInfo::ServerClosed(void)
{
	ASSERT(_bConnected == true);
	ASSERT(_nFailed == 0);
	ASSERT(_nLastTime > 0);
	_bConnected = false;
}


