//-----------------------------------------------------------------------------
// serverlist.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "serverlist.h" for more information about this class.
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

#include <stdlib.h>

#include "serverlist.h"


//---------------------------------------------------------------------
// CJW: Constructor.   Start with an empty list.
ServerList::ServerList()
{
	_pList = NULL;
	_nItems = 0;
}
    
//---------------------------------------------------------------------
// CJW: Deconstructor.  Clean up the list.
ServerList::~ServerList()
{
	while(_nItems > 0) {
		_nItems--;
		if (_pList[_nItems] != NULL) {
			delete _pList[_nItems];
			_pList[_nItems] = NULL;
		}
	}
    
	if (_pList != NULL) {
		free(_pList);
		_pList = NULL;
	}
    
	ASSERT(_nItems == 0);
}
		
//---------------------------------------------------------------------
// CJW: Get the next server in the list that we should try.  Keep in 
// 		mind that any server that has been attempted but failed, must 
// 		wait before trying that one again.  We will return a pointer to 
// 		the ServerInfo object, it should stay there as long as it is 
// 		being used.
//
//		To accomplish this, we will go thru the list of servers one at 
//		a time, and keep track of the one with the oldest attempt-time, 
//		and least number of failures.
ServerInfo * ServerList::GetNextServer(void)
{
	ServerInfo *pInfo = NULL;
	time_t nTime;
	int i;
			
	nTime = time(NULL);
	for(i=0; i<_nItems; i++) {
		if (_pList[i] != NULL) {
			if (_pList[i]->_bConnected == false) {
				if (_pList[i]->_nLastTime == 0) {
					if (pInfo == NULL) { 
						pInfo = _pList[i]; 
						i=_nItems;
					}
				}
				else if ((nTime - _pList[i]->_nLastTime) < NODE_WAIT_TIME) {
					if (pInfo == NULL) { pInfo = _pList[i]; }
					else {
						if (pInfo->_nFailed > _pList[i]->_nFailed) {
							if (pInfo->_nLastTime > _pList[i]->_nLastTime) {
								pInfo = _pList[i];
							}
						}
					}
				}
			}
		}
	}
			
	if (pInfo != NULL) {
		pInfo->_nLastTime = nTime;
	}
			
	return(pInfo);
}



//---------------------------------------------------------------------
// CJW: Convert the server and port information into an address, and 
// 		then add it to our list.
void ServerList::AddServer(char *szServer, int nPort)
{
	Address *pAddress;
	
	ASSERT(szServer != NULL && nPort > 0);
	pAddress = new Address;
	pAddress->Set(szServer, nPort);
	
	AddServer(pAddress);
}
		

//---------------------------------------------------------------------
// CJW: Add a server to the list.
void ServerList::AddServer(Address *pAddress)
{
	ServerInfo *pInfo;
	
	ASSERT(pAddress != NULL);
	ASSERT((_pList == NULL && _nItems == 0) || (_pList != NULL && _nItems >= 0));
			
	pInfo = new ServerInfo;
	pInfo->_pAddress = pAddress;
			
	_pList = (ServerInfo **) realloc(_pList, sizeof(ServerInfo *) * (_nItems+1));
	ASSERT(_pList != NULL);
	_pList[_nItems] = pInfo;
	_nItems++;
}


