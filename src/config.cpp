//-----------------------------------------------------------------------------
// config.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		See "config.h" for more details on this class.
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


#include <stdlib.h>
#include <string.h>

#include "config.h"

DpLock Config::_lock;
DpIniFile *Config::_pIniFile = NULL;
ServerEntry *Config::_pServers = NULL; 
int Config::_nServers = 0;


//---------------------------------------------------------------------------
// CJW: 
Config::Config()
{
	Lock();
	ASSERT((_nServers == 0 && _pServers == NULL) || (_nServers > 0 && _pServers != NULL));
	Unlock();
}


//---------------------------------------------------------------------------
Config::~Config()
{
	Lock();
	ASSERT((_nServers == 0 && _pServers == NULL) || (_nServers > 0 && _pServers != NULL));
	Unlock();
}

//---------------------------------------------------------------------------
bool Config::Load(char *szFile)
{
	bool result;
	
    ASSERT(szFile != NULL);
    
    Lock();
    ASSERT(_pIniFile == NULL);
    _pIniFile = new DpIniFile;
    ASSERT(_pIniFile != NULL);
    result = _pIniFile->Load(szFile);
    Unlock();
	
	return(result);
}

//---------------------------------------------------------------------------
void Config::Release(void)
{
    Lock();
    ASSERT(_pIniFile != NULL);
    delete _pIniFile;
    _pIniFile = NULL;
	
	
	ASSERT((_nServers == 0 && _pServers == NULL) || (_nServers > 0 && _pServers != NULL));
	if (_pServers != NULL) {
		free(_pServers);
		_pServers = NULL;
		_nServers = 0;
	}
	
    Unlock();
}


//---------------------------------------------------------------------------
bool Config::Get(char *grp, char *name, char **value)
{
    bool result = false;
    
    ASSERT(grp != NULL);
    ASSERT(name != NULL);
    ASSERT(value != NULL);

    Lock();
    ASSERT(_pIniFile != NULL);
    if (_pIniFile->SetGroup(grp) != false) {
        result = _pIniFile->GetValue(name, value);
    }
    Unlock();
    
    return(result);
}

//---------------------------------------------------------------------------
bool Config::Get(char *grp, char *name, int *value)
{
    bool result = false;
    
    ASSERT(grp != NULL);
    ASSERT(name != NULL);
    ASSERT(value != NULL);

    Lock();
    ASSERT(_pIniFile != NULL);
    if (_pIniFile->SetGroup(grp) != false) {
        result = _pIniFile->GetValue(name, value);
    }
    Unlock();
    
    return(result);
}


//---------------------------------------------------------------------------
// CJW: We have some server information we want to add to the list of 
//		servers.  At some point we can add functionality to clean up the list 
//		and remove any entries that are dead (not responding for some time).
void Config::AddServer(char *ip, int port, int type)
{
	ASSERT(ip != NULL);
	ASSERT(port > 0);
	ASSERT(type >= 0);
	
	Lock();
	
	ASSERT((_nServers == 0 && _pServers == NULL) || (_nServers > 0 && _pServers != NULL));
	
	_pServers = (ServerEntry *) realloc(_pServers, sizeof(ServerEntry) * (_nServers + 1)); 
	ASSERT(_pServers != NULL);
	
	strncpy(_pServers[_nServers].szServer, ip, MAX_SERVER_LEN);
	_pServers[_nServers].szServer[MAX_SERVER_LEN] = '\0';
	_pServers[_nServers].nPort = port;
	_pServers[_nServers].tTime = 0;
	_pServers[_nServers].nType = type;
	_pServers[_nServers].nStatus = SERVER_STATUS_UNKNOWN;
	
	_nServers++;
	
	Unlock();
}



