//-----------------------------------------------------------------------------
// address.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
//      
// 		This address object is used to manage an IP address and port 
//		combination that represents a remote server.
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

#ifndef __ADDRESS_H
#define __ADDRESS_H

#include "config.h"

#ifndef MAX_SERVER_LEN
#error MAX_SERVER_LEN must be defined.  Should be in config.h
#endif

//-----------------------------------------------------------------------------
struct Address 
{
	public:
		Address();
		virtual ~Address();
		
		void Set(char *szServer, int nPort);
		void Set(unsigned char *pRaw);
		void Set(Address *pAddr);
		char *GetServer();
		int GetPort();
		
		void Get(unsigned char *pRaw);
		
		bool IsSame(unsigned char *pRaw);
	
	protected:
		
	private:
		char  _szServer[MAX_SERVER_LEN + 1];
    	int   _nPort;
};


#endif

