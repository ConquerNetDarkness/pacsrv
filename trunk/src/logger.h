//-----------------------------------------------------------------------------
// logger.h
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		This class is used to log information to the log file.  
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


#ifndef __LOGGER_H
#define __LOGGER_H

#include <stdio.h>
#include <DpLock.h>

#define TODO(st)		printf("%s (%d): %s\n", __FILE__, __LINE__, st);

#define MAX_LOG_LINE	4096

class Logger
{
	public:
		Logger();
		virtual ~Logger();

		void Test(char *str, ...);
		void System(char *str, ...);
		void Error(char *str, ...);
		
		void Init(char *file, int max=30);
		
	protected:
	private:
		void CycleLogs(void);
		void WriteOut(char sub, char *log);
		
		static FILE *_pFile;
		static int   _nInstances;
		static DpLock _xLock;
		static char *_pFileStr;
		static int   _nMax;
};


#endif

