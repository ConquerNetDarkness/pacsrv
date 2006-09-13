//-----------------------------------------------------------------------------
// common.h
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      Common structures and values that are shared between the server and
//      client applications.
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


#ifndef __COMMON_H
#define __COMMON_H

#include <time.h>

#define PKG_PATH	"/var/cache/pacman/pkg"

#define MAX_CHUNK_SIZE	32767

#define HEARTBEAT_DELAY     5
#define HEARTBEAT_MISS      3


struct strHeartbeat 
{
    int nBeats;     // number of beats we have missed.
    int nDelay;     // number of seconds before we indicate that we have missed a beat.
    int nWait;      // waiting to find out if the file is on the network or not.
    time_t nLastCheck;
    
    public:
        strHeartbeat() {
            nBeats = 0;
            nDelay = 0;
            nWait  = 0;
            nLastCheck = 0;
        }
    
        void Clear(void) {
            nBeats = 0;
        }
};



#endif

