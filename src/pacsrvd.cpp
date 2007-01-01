//-----------------------------------------------------------------------------
// pacsrvd.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
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


#include <unistd.h>
#include <stdio.h>

#include <DpMain.h>

#include "server.h"
#include "config.h"
#include "logger.h"

#ifndef INI_FILE
#define INI_FILE "../etc/pacsrv.conf"
#endif


//-----------------------------------------------------------------------------
// Check that the DevPlus version is at least what we started development with.  
// It might be possible to compile on an older version, but cant think why you 
// would want to.
#if (DEVPLUS_VERSION < 225) 
	#error DevPlus version must be 2.25 or higher.
#endif

class myApp : public DpMain 
{
	private:
		Server *_pServer;
		Config *_pConfig;
		Logger *_pLogger;
	
	public:
		
		//---------------------------------------------------------------------
		// CJW: Constructor.  Do some basic initialisation if there is anything 
		// 		to be initialised.
		myApp() 
		{
			_pServer = NULL;
			_pConfig = NULL;
			_pLogger = NULL;
		}
		
		//---------------------------------------------------------------------
		// CJW: Deconstructor.  Clean up the things we initialised.
		virtual ~myApp()
		{
			ASSERT(_pServer == NULL);
			ASSERT(_pConfig == NULL);
			ASSERT(_pLogger == NULL);
		}
	
	protected:

		//---------------------------------------------------------------------
		// CJW: We want to do normal interrupt processing here. When a control 
		// 		break is pressed we will print a message only.  The application 
		// 		will shutdown normally when it gets it.
		virtual void OnCtrlBreak(void)
		{
			printf(" ** Ctrl Break.\n");
			DpMain::OnCtrlBreak();
		}

		
		//---------------------------------------------------------------------
		// CJW: Before the main part of the application starts, we can do more 
		// 		functionality here.  When we exit this function, if we have 
		// 		threads compiled in, the application will continue to run until 
		// 		the application is told to stop by some external interface.  If 
		// 		you dont want the application to continue running, you must 
		// 		call the Shutdown() function.
		virtual void OnStartup(int argc, char **argv)
		{
			printf("pacsrvd started.\n");
			
			ASSERT(_pServer == NULL);
			ASSERT(_pConfig == NULL);
			ASSERT(_pLogger == NULL);

			_pLogger = new Logger;
			ASSERT(_pLogger != NULL);
			if (_pLogger == NULL) {
				fprintf(stderr, "Out of memory.\n");
				Shutdown(1);
			}
			else {
				_pLogger->Init("pacsrvd-log", 30);
				_pLogger->System("System Starting");
	
				// load the config file.
				_pConfig = new Config;
				ASSERT(_pConfig != NULL);
				if (_pConfig == NULL) {
					fprintf(stderr, "Out of memory.\n");
					Shutdown(1);
				}
				else {
			
		 			ASSERT(INI_FILE != NULL);
					_pConfig->Load(INI_FILE);
		
		
					// start the local listener.
					_pServer = new Server;
					ASSERT(_pServer != NULL);
					if (_pServer == NULL) {
						fprintf(stderr, "Out of memory.\n");
						Shutdown(1);
					}
					else {
						if (_pServer->IsListening() == false) {
							Shutdown(2);
						}					
					}
				}
			}	
		}
		
		
		//---------------------------------------------------------------------
		// CJW: We are either shutting down because a Ctrl-Break was pressed, 
		// 		or somewhere in the execution the Shutdown function was called.
		virtual void OnShutdown(void)
		{
			if (_pLogger != NULL) {
				_pLogger->System("Shutting down network");
			}
			
			if (_pServer != NULL) {
				delete _pServer;
				_pServer = NULL;
			}
			
			if(_pConfig != NULL) {
				_pConfig->Release();
				delete _pConfig;
				_pConfig = NULL;
			}
		
			if (_pLogger != NULL) {
				_pLogger->System("System Stopped");
				delete _pLogger;
				_pLogger = NULL;
			}
			
			printf("pacsrvd stopped.\n");
		}
		
} thisApp;



