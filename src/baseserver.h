//-----------------------------------------------------------------------------
// baseserver.h
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		This class is an abstract class that must be derived.  It contains the 
//		basic server connectivity that is common for our Server and Network 
//		classes.
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

#ifndef __BASESERVER_H
#define __BASESERVER_H

#include <DpServerInterface.h>

class BaseServer : public DpServerInterface
{
	public:
		BaseServer();
		virtual ~BaseServer();
		
		bool IsListening(void);
		
	protected:
		virtual void OnAccept(int)=0;
		virtual void OnIdle(void)=0;
		
		void SetListening(void) {
			Lock();
			ASSERT(_bListening == false);
			_bListening = true;
			Unlock();
		}
		
		bool CheckTime(void);
		
		
	public:
		
	protected:
		time_t _nLastCheck;
	
	private:
		bool _bListening;
};


#endif

