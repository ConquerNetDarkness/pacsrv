//-----------------------------------------------------------------------------
// config.h
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		This class is used to load the config data from the config file, and 
//		provide that information to all other instances and threads.  It uses 
//		static member variables, so all you have to do is create an instance of 
//		this object and you can access the config information.
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


#ifndef __CONFIG_H
#define __CONFIG_H

#include <DevPlus.h>

#define MAX_SERVER_LEN			15

#define SERVER_TYPE_DIRECT		0
#define SERVER_TYPE_WEB			1
#define SERVER_TYPE_RELAY		2

#define SERVER_STATUS_UNKNOWN   0
#define SERVER_STATUS_FULL		1
#define SERVER_STATUS_GONE		2
#define SERVER_STATUS_DEAD		3


//-----------------------------------------------------------------------------
// When we send a file request over the network... this is the number of hops 
// this message will take before it is discarded.  It should be set as a 
// configurable value at some point.
#define DEFAULT_TTL			15

#define NODE_PROTOCOL_VER	1



struct ServerEntry 
{
	char szServer[MAX_SERVER_LEN+1];
	int nPort;
	time_t tTime;
	int nType;
	int nStatus;
};

class Config 
{
    public:
        Config();
        virtual ~Config();
        
        bool Load(char *file);
        void Release(void);
        bool Get(char *grp, char *name, char **value);
        bool Get(char *grp, char *name, int   *value);

		void AddServer(char *ip, int port, int type);
		
    protected:
        
    private:
		
		void Lock(void)     { _lock.Lock(); }
		void Unlock(void)	{ _lock.Unlock(); }
		
		static DpLock     _lock;
        static DpIniFile *_pIniFile;
		static ServerEntry *_pServers;
		static int _nServers;
};


#endif

