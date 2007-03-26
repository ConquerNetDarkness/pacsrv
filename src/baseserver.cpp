//-----------------------------------------------------------------------------
// baseserver.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		See "baseserver.h" for more details about this object.
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


#include "baseserver.h"


//-----------------------------------------------------------------------------
// CJW: Constructor.  
BaseServer::BaseServer()
{
	Lock();
	_nLastCheck = time(NULL);
	_bListening = false;
	Unlock();
}

//-----------------------------------------------------------------------------
// CJW: Deconstructor.  
BaseServer::~BaseServer()
{
	Lock();
	Unlock();
}

//-----------------------------------------------------------------------------
// CJW: Return true if we successfully started listening.  This basically means 
//		that everything is working.
bool BaseServer::IsListening(void)
{
	bool b;
	Lock();
	b = _bListening;
	Unlock();
	return(b);
}


//-----------------------------------------------------------------------------
// CJW: We keep track of the time and every time that we roll over to a new 
// 		second we want to return true so that the nodes can be told to 
// 		increment their counters and stuff.  
bool BaseServer::CheckTime(void)
{
	bool bCheck = false;
	time_t nTime;
	
	Lock();
	nTime = time(NULL);
	if (nTime > _nLastCheck) {
		Unlock();
		bCheck = true;
		Lock();
		_nLastCheck = nTime;
	}
	Unlock();
	return(bCheck);
}


