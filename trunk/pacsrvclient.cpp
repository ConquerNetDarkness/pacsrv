//-----------------------------------------------------------------------------
// pacsrvclient.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		This application is the client that connects to the pacsrv daemon.  It 
//		is supposed to be called from pacman, and it will be given two 
//		parameters.  The first being the file that we are supposed to download 
//		(plus a .part extention).  The second argument is the URL that the file 
//		can be found at.  If we cannot get the file from the pacsrv daemon (and 
//		network), then we will get the file directly from the URL that is 
//		given.
//
//		This client application is supposed to be single threaded.  It is a 
//		linear tool that does one thing and then exits.  That one thing is to 
//		download a particular file (either from the pacsrv network or from the 
//		url that is supplied).  The main download loop when talking with the 
//		pacsrvd will do a 1 second sleep if there is no data to download.  
//		Since the daemon will send as much information as the socket will 
//		allow, that shouldnt affect us.   Since all my usual socket routines 
//		are used in a multi-threaded application, they are not really suitable 
//		for a single threaded application.   Once the concept is working, this 
//		part of the code will need to be modified to be more efficient.
//
//	VANITY DISCLAIMER:
//		The way this application is coded, might not be the best way for an 
//		application that is required to be memory, cpu and portability 
//		efficient.   By using inline functions for all the class functionality 
//		is probably not the most efficient in other environments, but since 
//		almost all the function calls are one-time calls anyway, there 
//		shouldn't actually be a problem, and it might actually be more 
//		efficient.   For those that dont know what I am talking about, it 
//		doesnt matter.  I'm just pointing out that this code is probably not 
//		the best to build more complicated tasks out of.  This instance does 
//		not really do much, so it should be just fine for this application.  
//		The server code on the other hand, is written in a different style 
//		because it will always be running.  This client application will only 
//		run to transfer one file and then exit, starting over again.
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


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <DevPlus.h>

#include "common.h"

#ifndef INI_FILE
#define INI_FILE "/etc/pacsrv.conf"
#endif

#define PACKET_SIZE		1024

#define HEARTBEAT_WAIT		30	// Number of seconds we will wait for a file from the network.

#define WIDTH_FILENAME		31
#define WIDTH_BAR			20


class Client 
{
	public:
	protected:
	private:
		char *m_szFile;			// File parameter that is supplied.
		char *m_szUrl;			// URL parameter that is supplied.
		char *m_szServer;		// address (ip or name) of the daemon we need to talk to.
		int   m_nPort;			// port the daemon is listening on.
		
		struct {
			int nFileSize;
			int nDone;
			time_t nStart;
		} m_Stats;
		
		struct strHeartbeat m_Heartbeat;
		
		struct {
			char szFileDisplay[WIDTH_FILENAME + 1];	// Displayable file name.. may be truncated.
			int nFileSize;
			char cSizeInd;
		} m_Display;
		
		struct {
			bool bStop;			// true if we should stop looping.
			bool bInit;			// true if INIT has been sent.
			bool bRequest;		// true if FILE REQUEST has been sent.
			bool bIdle;			// true if the connection is idle.   will sleep for 1 second.
			bool bFileLength;	// true if we have received file length info.
			bool bDisplay;		// true if we are forcing the display to be updated.
		} m_Status;
		
		struct {
			char *szFilename;	// Filename that we are going to query the network with.
			FILE *fp;			// file pointer that we will write the file with.
			int nSize;			// full size of the file we are getting from the network.
			struct {
				int nCount;
				int nSize;
			} chunk, temp;
		} m_File;

		DpDataQueue m_qIn, m_qOut;

		
		
		
	public:
		//---------------------------------------------------------------------
		// CJW: Constructor.  Initialise our variables here.
		Client() 
		{
			m_szFile = NULL;
			m_szUrl = NULL;
			m_szServer = NULL;
			
			m_Stats.nFileSize = 0;
			m_Stats.nDone = 0;
			m_Stats.nStart = 0;
						
			m_Display.szFileDisplay[0] = '\0';
			m_Display.nFileSize = 0;
			m_Display.cSizeInd = 'x';
			
			m_Status.bInit       = false;
			m_Status.bRequest    = false;
			m_Status.bStop       = false;
			m_Status.bFileLength = false;
			m_Status.bDisplay    = false;
			
			m_File.szFilename = NULL;
			m_File.fp = NULL;
			m_File.nSize = 0;
			m_File.chunk.nCount = 0;
			m_File.chunk.nSize = 0;
		}
		
		//---------------------------------------------------------------------
		// CJW: Deconstructor.  Clean up our crap in here.
		virtual ~Client()
		{
			if (m_szServer != NULL) { 
				free(m_szServer); 
				m_szServer = NULL; 
			}
			
			if (m_File.szFilename != NULL) {
				free(m_File.szFilename);
				m_File.szFilename = NULL;
			}
		}
		
		//---------------------------------------------------------------------
		// CJW: This function will parse the arguments provided.  If the 
		// 		parameters couldnt be parsed because there wasnt enough, or 
		// 		there was too many, then we will return 0 and the application 
		// 		should exit.
		bool ParseArgs(int argc, char **argv)
		{
			bool bValid = false;
	
			ASSERT(m_szFile == NULL && m_szUrl == NULL);
			
			if (argc == 3) {
				m_szFile = argv[1];
				m_szUrl = argv[2];
		
				if (strlen(m_szFile) > WIDTH_FILENAME) {
					strncpy(m_Display.szFileDisplay, m_szFile, (WIDTH_FILENAME - 3));
					m_Display.szFileDisplay[WIDTH_FILENAME-3] = '.';
					m_Display.szFileDisplay[WIDTH_FILENAME-2] = '.';
					m_Display.szFileDisplay[WIDTH_FILENAME-1] = '.';
					m_Display.szFileDisplay[WIDTH_FILENAME] = '\0';
				}
				else {
					strncpy(m_Display.szFileDisplay, m_szFile, WIDTH_FILENAME);
				}
				
				if (m_szFile != NULL && m_szUrl != NULL) {
					bValid = true;
				}
			}
			
			return(bValid);
		}
		
		//---------------------------------------------------------------------
		// CJW: Load the config information.  If we cannot load the file, or 
		// 		the values are not valid, then we will return false.  Otherwise 
		// 		we will return true.
		bool LoadConfig(void) 
		{
			bool bValid = false;
			DpIniFile *pIni;
			
			pIni = new DpIniFile;
			if (pIni != NULL) {
				if (pIni->Load(INI_FILE) == true) {
					if (pIni->SetGroup("client") == true) {
						if (pIni->GetValue("server", &m_szServer) == true) {
							if (pIni->GetValue("port", &m_nPort) == true) {
				
								if (m_szServer != NULL && m_nPort > 0) {
									bValid = true;
								}
							}
						}
					}
				}
				
				delete pIni;
			}
			return(bValid);
		}
		
		//---------------------------------------------------------------------
		// CJW: This function will display a status line to the screen.  We 
		// 		will update this probably every second before we do the sleep.  
		// 		Although, if we keep processing data from the server, we may
		//		need to display this more often also.
		void DisplayStatus(void)
		{
			char szBar[WIDTH_BAR + 1];
			int nPercent;
			int i, j;
			int nRate, nRateLo;
			int nMin, nSec;
			time_t nTime;
			int nSeconds;
			
			ASSERT(m_Display.szFileDisplay != NULL);
			ASSERT(m_Display.szFileDisplay[0] != '\0');
			ASSERT(m_Stats.nFileSize > 0);
			
			if (m_Display.nFileSize == 0) {
			
				m_Display.nFileSize = m_Stats.nFileSize;
				if (m_Display.nFileSize <= 999) {
					m_Display.cSizeInd = 'b';
				}
				else {
					m_Display.nFileSize /= 1024;
					if (m_Display.nFileSize <= 1) { m_Display.nFileSize = 1; }
					if (m_Display.nFileSize <= 999) {
						m_Display.cSizeInd = 'k';
					}
					else {
						m_Display.nFileSize /= 1024;
						if (m_Display.nFileSize <= 1) { m_Display.nFileSize = 1; }
						if (m_Display.nFileSize <= 999) {
							m_Display.cSizeInd = 'm';
						}
						else {
							m_Display.nFileSize /= 1024;
							if (m_Display.nFileSize <= 1) { m_Display.nFileSize = 1; }
							if (m_Display.nFileSize <= 999) {
								m_Display.cSizeInd = 'g';
							}
						}
					}
				}
			}			
			ASSERT(m_Display.cSizeInd != 'x');
			
			
			nPercent = (m_Stats.nDone / (m_Stats.nFileSize / 100));
			j = nPercent / (100/WIDTH_BAR);
			for(i=0; i<WIDTH_BAR; i++) {
				if (i<=j) {
					szBar[i] = '#';
				}
				else {
					szBar[i] = ' ';
				}
			} 
			szBar[WIDTH_BAR] = '\0';
			
			// Need to also determine the download rate, and the estimated time left.
			nTime = time(NULL);
			nSeconds = nTime - m_Stats.nStart;
			ASSERT(nSeconds >= 0);
			if (nSeconds == 0) {
				nRate = m_Stats.nDone;
			}
			else {
				nRate = m_Stats.nDone / nSeconds;
			}
			nRateLo = nRate % 1024;
			nRate = nRate / 1024;
			if (nRateLo < 100) { nRateLo = 1; }
			else { nRate /= 100; }
			
			printf("\r%*s %3d%c |%s| %3d%% %4d.%1dkb/s ", WIDTH_FILENAME, m_Display.szFileDisplay, m_Display.nFileSize, m_Display.cSizeInd, szBar, nPercent, nRate, nRateLo );
			fflush(stdout);
			
			m_Status.bDisplay = false;
		}

		//---------------------------------------------------------------------
		// CJW: Download the file.  If we cant, then we try and get it by 
		// 		downloading the url.  This means that we will try to connect to 
		// 		the daemon.  If the daemon has the file, we will get it from 
		// 		there.  If it doesnt, or we couldnt connect to it, then we will 
		// 		download the file from the URL instead.
		bool Download(void)
		{
			bool bGotIt = false;
			
			if (DownloadNetwork() == false) {
				bGotIt = DownloadUrl();
			}
			else {
				bGotIt = true;
			}
			
			return(bGotIt);
		}
		
		//---------------------------------------------------------------------
		// CJW: We need to prepare the filename.  The one passed to us, 
		// 		probably has ".part" at the end of it.  Also, if the file isnt 
		// 		a .pkg.tar.gz file, then we dont want to request the daemon for 
		// 		it.  
		bool PrepareFilename(void)
		{
			int nLength;
			bool bValid = false;
			
			ASSERT(m_szFile != NULL);
			ASSERT(m_File.szFilename == NULL);
			
			nLength = strlen(m_szFile);
			if (nLength > 16) {
				if (strncmp(&m_szFile[nLength - 16], ".pkg.tar.gz.part", 16) == 0) {
					m_File.szFilename = (char *) malloc((nLength - 16)+1);
					ASSERT(m_File.szFilename);
					strncpy(m_File.szFilename, m_szFile, nLength - 5);
					m_File.szFilename[nLength-5] = '\0';
					bValid = true;
				}
			}
			
			return(bValid);
		}
		
		//---------------------------------------------------------------------
		// CJW: Get any data that is ready in the socket, and put it in our 
		// 		'IN' queue.
		void ReadData(DpSocket *pSocket) 
		{
			char *pData;
			int nLength;
			
			ASSERT(pSocket != NULL);
			
			pData = (char *) malloc(PACKET_SIZE);
			ASSERT(pData != NULL);
			nLength = pSocket->Receive(pData, PACKET_SIZE);
			if (nLength < 0) {
				// socket was closed.  
				m_Status.bStop = true;
			}
			else if (nLength > 0) {
				m_qIn.Add(pData, nLength);
				m_Status.bIdle = false;
			}
			free(pData);
		}
		
		//---------------------------------------------------------------------
		// CJW: Send any data that is waiting in our 'OUT' queue..
		void SendData(DpSocket *pSocket)
		{
			int nLength, nSent;
			char *pData;
			
			ASSERT(pSocket != NULL);
													
			nLength = m_qOut.Length();
			if (nLength > 0) {
				if (nLength > PACKET_SIZE) { nLength = PACKET_SIZE; }
				pData = m_qOut.Pop(nLength);
				ASSERT(pData != NULL);
								
				nSent = pSocket->Send(pData, nLength);
				if (nSent < 0) {
					m_Status.bStop = true;
				}
				else if (nSent < nLength) {
					m_qOut.Push(&pData[nSent], nLength-nSent);
				}
				else {
					m_Status.bIdle = false;
				}
								
				free(pData);
			}
		}
		
		
		//---------------------------------------------------------------------
		// CJW: Queue the INIT message that we need to send to the daemon.  We 
		// 		then change the status to indicate that we've sent it.
		void SendInit(void)
		{
			m_qOut.Add("I\000\000", 3);
			m_Status.bInit = true;
		}
		
		
		//---------------------------------------------------------------------
		// CJW: Queue the File request message that we need to send to the 
		// 		daemon.  We then change the status to indicate that we've sent 
		// 		it.
		void SendFileRequest(void) 
		{
			int nLength;
			unsigned char ch;
			
			ASSERT(m_File.szFilename != NULL);
			nLength = strlen(m_File.szFilename);
			ASSERT(nLength < 256);
			
			ch = (unsigned char) nLength;
			
			m_qOut.Add("F", 1);
			m_qOut.Add((char *) &ch, 1);
			m_qOut.Add(m_File.szFilename, nLength);
			
			m_Status.bRequest = true;
		}
		
		//---------------------------------------------------------------------
		// CJW: We check to see if we have moved to the next second, and if so, 
		// 		we process our heartbeat stuff.  Every 5 seconds, we need to 
		// 		send a heartbeat to the daemon.  If it has been more than 2 
		// 		missed rounds of 5 seconds, then we can assume we have lost 
		// 		communications with the daemon.
		//
		//		We also want to increment the counter that keeps track of how 
		//		long we have waited for a responce to a file request we have 
		//		made.  If it takes longer than 30 seconds to get a responce we 
		//		might as well stop this and get the file from the mirror 
		//		instead.
		void ProcessHeartbeat(void)
		{	
			time_t nTime;
			
			nTime = time(NULL);
			if (nTime > m_Heartbeat.nLastCheck) {
				m_Heartbeat.nDelay ++;
				
				if (m_Status.bFileLength == false) {
					m_Heartbeat.nWait ++;
				}
				
				if (m_Heartbeat.nWait > HEARTBEAT_WAIT) {
					m_Status.bStop = true;
				}
				else {
					if (m_Heartbeat.nDelay >= HEARTBEAT_DELAY) {
						m_Heartbeat.nBeats++;
						m_Heartbeat.nDelay = 0;
						if (m_Heartbeat.nBeats > HEARTBEAT_MISS) {
							m_Status.bStop = true;
						}
						else {
							m_qOut.Add("H", 1);
						}
					}
				}
								
				m_Heartbeat.nLastCheck = nTime;
				m_Status.bDisplay = true;
			}
		}
		
		//---------------------------------------------------------------------
		// CJW: We have received a FILE-LENGTH message from the daemon which 
		// 		indicates that it has found the file we are looking for, on the 
		// 		network, and it knows how big the file is.  We will use that 
		// 		information to create the file ready for the downloaded data, 
		// 		and trigger displaying the download status information.  We 
		// 		will need to check to make sure we havent already received this 
		// 		information.  Because if we've already received it, and we 
		// 		receive it again, its an indication of a corrupted stream and 
		// 		we must stop.
		void ProcessFileLength(void)
		{
			int nLength;
			char *pData;
			
			ASSERT(m_File.nSize == 0);
			ASSERT(m_File.fp == NULL);
			
			if (m_Status.bFileLength == true || m_File.nSize != 0) {
				m_Status.bStop = true;
			}
			else {
				nLength = m_qIn.Length();
				if (nLength >= 4) {
					pData = m_qIn.Pop(4);
					ASSERT(pData != NULL);	
					m_Status.bFileLength = true;
					
					m_File.nSize = 0;
					m_File.nSize += ((unsigned char) pData[0]) << 24;
					m_File.nSize += ((unsigned char) pData[1]) << 16;
					m_File.nSize += ((unsigned char) pData[2]) << 8;
					m_File.nSize +=  (unsigned char) pData[3];
					free(pData);
														
					// we are opening the filename that was passed as an argument to this application. (the one with .part)
					m_File.fp = fopen(m_szFile, "w");		
					if (m_File.fp == NULL) {
						m_Status.bStop = true;
					}
				}
				else {
					// Not enough data there yet, so push this one back.
					m_qIn.Push("L", 1);
				}
			}
		}
		
		
		//---------------------------------------------------------------------
		// CJW: We are now apparently receiving a chunk of a file from the 
		// 		daemon.  We need to make sure that it is the right chunk, and 
		// 		also that all of it has arrived.  Once the entire chunk is in 
		// 		the queue, we will pull it out, write it to the file, and 
		// 		update our display.
		void ProcessChunk(void)
		{
			int nLength;
			char *pData;
			
			nLength = m_qIn.Length();
			if (nLength >= 4) {
				pData = m_qIn.Pop(4);
				ASSERT(pData != NULL);

				m_File.temp.nCount = ((unsigned char) pData[0]) << 8;
				m_File.temp.nCount += (unsigned char) pData[1];
													
				if (m_File.temp.nCount != m_File.chunk.nCount) {
					m_Status.bStop = true;
				}
				else {
					m_File.chunk.nCount = m_File.temp.nCount;

					m_File.temp.nSize = ((unsigned char) pData[2]) << 8;
					m_File.temp.nSize += (unsigned char) pData[3];
														
					if (m_File.temp.nSize <= 0) {
						m_Status.bStop = true;
					}
					else {
						if (nLength >= (m_File.temp.nSize + 4)) {
							free(pData);
							pData = m_qIn.Pop(m_File.temp.nSize);
							ASSERT(pData);
							ASSERT(m_File.fp != NULL);
							
							m_Stats.nDone += m_File.temp.nSize;
							fwrite(pData, m_File.temp.nSize, 1, m_File.fp);
							
							m_Heartbeat.nBeats = 0;
						}
						else {
							m_qIn.Push(pData, 4);
							m_qIn.Push("C", 1);
						}
					}
				}
													
				free(pData);
			}
		}
		
		
		//---------------------------------------------------------------------
		// CJW: Process the data that we have received from the daemon.  Since 
		//		there are a couple of steps we have to go thru before we 
		//		actually start getting any of the file contents, we have to 
		//		check a few status values first and make sure we get the right 
		//		responce from the daemon.
		void ProcessData(void)
		{
			char *pData;
			int nLength;
			char cCommand;
			
			if (m_Status.bInit == false) {
				// We havent sent the init command yet, so send it now.
				ASSERT(m_Status.bRequest == false);
				SendInit();
			}
			else if (m_Status.bRequest == false) {
				// We've sent the init, but havent sent the file request yet.   Do that now.
				SendFileRequest();
			}
			else {
				ProcessHeartbeat();
								
				// Now we need to actually check the contents of our incoming queue. 
				if (m_Status.bStop == false) {
					nLength = m_qIn.Length();
					if (nLength > 0) {
						pData = m_qIn.Pop(1);
						ASSERT(pData);
						cCommand = pData[0];
						free(pData);
										
						switch(cCommand) {
							case 'V':
							case 'H':
								m_Heartbeat.nBeats = 0;
								break;
												
							case 'L':
								ProcessFileLength();
								break;
												
							case 'C':
								ProcessChunk();
								break;
												
							case 'Q':
							case 'X':
							default:
								m_Status.bStop = true;
								break;										
						}
					}
				}
			}
		}
		
		
		
		
		//---------------------------------------------------------------------
		// CJW: Download the file from the network.  We need to connect to the 
		// 		daemon, and request the file.  If we cannot connect to the 
		// 		daemon, or if the file is not on the network, then we will 
		// 		return a false.
		bool DownloadNetwork(void)
		{
			bool bGotIt = false;
			DpSocket *pSocket;
			
			ASSERT(m_szServer != NULL);
			ASSERT(m_nPort > 0);
			ASSERT(m_szFile != NULL);
			
			// Prepare the filename, removing the ".part" bit.
			if (PrepareFilename() == true) {
			
				// connect to the local server daemon.
				pSocket = new DpSocket;
				if (pSocket != NULL) {
				
					if (pSocket->Connect(m_szServer, m_nPort) == false) {
						printf("pacsrvclient: ** Unable to connect to server %s:%d\n\n", m_szServer, m_nPort);
					}
					else {
						
						// Get the current time.
						m_Heartbeat.nLastCheck = time(NULL);
						m_Stats.nStart = m_Heartbeat.nLastCheck;
						
						// Because we are setting the idle setting when any activity occurs, some states in this while loop expect that we will loop agian straight away.  If we sleep for 1 second between these particular loops, then the file transfer will take too long.  For example, we send the init command, and then immediately send the File request, then we should expect to wait for the data to start arriving (checking heartbeats while we do).  the IDLE status is to indicate if the socket activity was idle.  
						while(m_Status.bStop == false) {
							m_Status.bIdle = true;
							
							ReadData(pSocket);
							ProcessData();
							SendData(pSocket);
							
							ASSERT(m_Stats.nDone <= m_File.nSize);
							if (m_Stats.nDone == m_File.nSize) {
								bGotIt = true;
								m_Status.bStop = true;
								m_Status.bIdle = false;
								m_Status.bDisplay = true;
							}
							
							if (m_Status.bIdle == true) {
								m_Status.bDisplay = true;
							}
								
							if (m_Status.bDisplay == true) {
								DisplayStatus();
							}
							
							if (m_Status.bIdle == true) {
								sleep(1);
							}
						}
					}			
				
					delete pSocket;;
				}
			}
			
			if (m_File.fp != NULL) {
				fclose(m_File.fp);
			}
			
			return(bGotIt);
		}
		
		
		//---------------------------------------------------------------------
		// CJW: We werent able to get the file from the daemon, so we must get 
		// 		it ourselves.  For now we will use a system call to wget to do 
		// 		it for us.
		bool DownloadUrl(void)
		{
			ASSERT(m_szUrl != NULL);
			ASSERT(m_szFile != NULL);
			
			// This function will basically call wget and this process will cease to be.  If the function returns, then something went wrong.
			execl("/usr/bin/wget", "-nv", "-c", "-O", m_szFile, m_szUrl, (char *) NULL);
			
			printf("Something went wrong with wget.\n");
			
			return(false);
		}

		
		
};






//-----------------------------------------------------------------------------
// CJW: Main function.  This function is the first one called when the 
// 		application starts.  It provides the arguments that were supplied.  
// 		Inside here we create our client object and then use it to process the 
// 		stuff. 
int main(int argc, char **argv)
{
	Client client;
	int nRet = 0;

	if (client.ParseArgs(argc, argv) == false) {
		fprintf(stderr, "pacsrvclient: Invalid number of parameters.\n");
		nRet = 1;
	}
	else {
		if (client.LoadConfig() == false) {
			fprintf(stderr, "pacsrvclient: Unable to open config file.\n");
			nRet = 2;
		}
		else {
			if (client.Download() == false) {
				fprintf(stderr, "pacsrvclient: Failed to download: %s\n", argv[2]);
				nRet = 3;
			}
		}
	}
	
	if (nRet == 0) { printf("\n"); }
	
	return nRet;
}



