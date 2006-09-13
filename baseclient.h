//-----------------------------------------------------------------------------
// baseclient.h
//
//  Project: pacsrv
//  Author: Clint Webb
// 
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


#ifndef __BASECLIENT_H
#define __BASECLIENT_H

#include <DevPlus.h>


class BaseClient : public DpSocket
{
    public:
        BaseClient();
        virtual ~BaseClient();
    
    protected:
		DpDataQueue _DataIn, _DataOut;
    
        bool ReadData(void);
        char GetCommand(void);
        bool SendData(void);

	private:
		
};


#endif

