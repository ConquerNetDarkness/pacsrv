//-----------------------------------------------------------------------------
// address.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "address.h" for more information about this class.
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

#include <string.h>
#include <stdio.h>

#include "address.h"

//-----------------------------------------------------------------------------
// CJW: Constructor.    Not much to do, just initialise our variables.
Address::Address()
{
	_szServer[0] = '\0';
	_nPort = 0;
}
		
//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Verify the integrity of our data.
Address::~Address()
{
	ASSERT((_szServer[0] == '\0' && _nPort == 0) || (_szServer[0] != '\0' && _nPort > 0));
}
		
//-----------------------------------------------------------------------------
// CJW: Set the server and port data for this object.
void Address::Set(char *szServer, int nPort)
{
	ASSERT(_szServer[0] == '\0' && _nPort == 0);
	ASSERT(szServer != NULL && nPort > 0);
			
	strncpy(_szServer, szServer, MAX_SERVER_LEN);
	_szServer[MAX_SERVER_LEN] = '\0';
}


//-----------------------------------------------------------------------------
// CJW: Return the Server that we have stored.  There is no reason to do this 
// 		if we dont already have some information.
char * Address::GetServer(void)
{
	ASSERT(_szServer[0] != '\0');
	return(_szServer);
}

//-----------------------------------------------------------------------------
// CJW: Return the port.
int Address::GetPort()
{
	ASSERT(_nPort > 0);
	return(_nPort);
}

//-----------------------------------------------------------------------------
// CJW: The raw binary data from the socket which includes the ip and port 
// 		needs to be processed.  Easy enough since we packed it that way to 
// 		begin with.
void Address::Set(unsigned char *pRaw)
{
	ASSERT(pRaw != NULL);
	ASSERT(_szServer[0] == '\0' && _nPort == 0);
	
	sprintf(_szServer, "%u.%u.%u.%u", (unsigned int) pRaw[0], (unsigned int) pRaw[1], (unsigned int) pRaw[2], (unsigned int) pRaw[3]);
	_nPort = (pRaw[4] << 8);
	_nPort = pRaw[5];
	
	ASSERT(_nPort > 0);
	printf("Address::Get() = %s:%d\n", _szServer, _nPort);
}

//-----------------------------------------------------------------------------
// CJW: We have a textual version of the address stored in this object.  In 
// 		order to send this information to other servers, we need to get it out 
// 		in raw 6 byte format.  This means we need to go thru the string.
void Address::Get(unsigned char *pRaw)
{
	int i,j;
	ASSERT(pRaw != NULL);
	ASSERT(_szServer[0] != '\0' && _nPort > 0);
	
	pRaw[0] = 0;
	for(i=0,j=0; _szServer[i] != '\0'; i++) {
		ASSERT(j < 4);
		ASSERT(_szServer[i] == '.' || (_szServer[i] >= '0' && _szServer[i] <= '9'));
		
		if (_szServer[i] == '.') {
			j++;
			pRaw[j] = 0;
		}
		else {
			pRaw[j] *= 10;
			pRaw[j] += _szServer[i] - '0';
		}
	}
	
	ASSERT(j == 3);
	pRaw[4] = (_nPort / 256);
	pRaw[5] = (_nPort % 256);
}


//-----------------------------------------------------------------------------
// CJW: We have an address passed in by pointer.  We need to get the details 
// 		out of it, and put it in this current object.
void Address::Set(Address *pAddr) 
{
	ASSERT(pAddr != NULL);
	Set(pAddr->GetServer(), pAddr->GetPort());
}

//-----------------------------------------------------------------------------
// CJW: Used to compare the information that we have as an address against a 
// 		raw address packet that we have received.
bool Address::IsSame(unsigned char *pRaw)
{
	unsigned char pData[6];
	bool bSame = true;
	int i;
	
	Get(pData);
	for (i=0; i<6 && bSame == true; i++) {
		if (pData[i] != pRaw[i]) {
			bSame = false;
		}
	}
	
	return(bSame);
}

