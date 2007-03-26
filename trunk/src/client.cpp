//-----------------------------------------------------------------------------
// client.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      see "client.h" for more details on this class.
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DevPlus.h>

#include "client.h"
#include "config.h"
#include "logger.h"


//-----------------------------------------------------------------------------
// CJW: If the client doesnt send us a File Request within this number of seconds, then we will close the connection.  The client should be able to send the simple request within 5 seconds surely.
#define HEARTBEAT_WAIT      5



int Client::_nNextClientID = 1;

//-----------------------------------------------------------------------------
// CJW: Constructor.  Initialise the values, and our state.  Since we havent 
//      actually received any data from the client, we have an UNKNOWN state.
Client::Client()
{
	Lock();
    _szQuery  = NULL;
    _nChunk   = 0;
    _nLength  = 0;
    _nVersion = 0;
    _nClientID = _nNextClientID++;
    if (_nNextClientID > 999999) _nNextClientID = 1;
    Unlock();
}


//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Clean up everything we created.  
Client::~Client()
{
	Unlock();
    if (_szQuery != NULL) {
        free(_szQuery);
        _szQuery = NULL;
    }
   	Lock();
}


//-----------------------------------------------------------------------------
// CJW: When the client is idle (which should happen quite a lot), we will 
// 		update our heartbeat information.  If the heartbeat fails, then we will 
// 		close the client connection and it will get deleted by the server 
// 		object.
void Client::OnIdle(void)
{
	if (ProcessHeartbeat() == false) {
		Close();
	}
	else {
		DpSocketEx::OnIdle();
	}
}


//-----------------------------------------------------------------------------
// CJW: Process the data from the socket, and send data that is ready to send.  
//      It will read data from the socket into a dataqueue, and then it will
//      process the data from the data queue.  If bCheck is true, then we will
//      also check that we've received a heartbeat.  Finally we will send as
//      much data in our outgoing dataqueue that we can.  If we cant send it
//      all right now, we can send it the next time this function is called.
//
//      If
//
//      Since the Server object is processing each client in the list, we want
//      this function to finish as quickly as possible, so we will only process
//      one data command at a time.  If there are more than one telegram ready
//      to process it will be ok, because we will quickly get another chance to
//      process.
int Client::OnReceive(char *pData, int nLength)
{
    int nProcessed = 0;
    int nTmp;
    
    ASSERT(pData != NULL && nLength > 0);
    
    Lock();
    
	switch(pData[0]) {
		case 'I':       // INIT
			if (nLength >= 3) {
				ProcessInit(pData, nLength);
				nProcessed = 3;
			}
			break;

		case 'H':       // Heartbeat
			_Heartbeat.Clear();
			nProcessed = 1;
			break;

		case 'F':       // File Request.
			if (nLength >= 3) {
				nTmp = ProcessFileRequest(pData, nLength);
				ASSERT(nTmp >= 0);
				if (nTmp > 0) {
					nProcessed = 2 + nTmp;
				}
				ASSERT((nTmp == 0 && nProcessed == 0) || (nTmp > 0 && nProcessed > 2));
			}
			break;

		case 'R':       // Chunk received.
			// We dont really care about this one, but must remove it from the queue if we get it.  Currently the client is not programmed to send this.
			if (nLength >= 3) {
				ProcessChunkReceived(pData, nLength);
				nProcessed = 3;
			}
			break;

		default:
			// we got a char we weren't expecting, so we want to close the connection.
			Close();
			break;
	}
    
    Unlock();
    
    return(nProcessed);
}



//-----------------------------------------------------------------------------
// CJW: Return a true if this client has a pending query.   We will put the 
//      string in the query parameter.  The calling routine which will be
//      responsible for freeing the memory.
bool Client::QueryData(char **query, int *nChunk)
{
    bool bResult = false;
    
    ASSERT(query != NULL);
    ASSERT(*query == NULL);
    ASSERT(nChunk != NULL);
    
    Lock();
    
    ASSERT((_szQuery == NULL && _nChunk == 0) || (_szQuery != NULL && _nChunk >= 0));
    
    if (_szQuery != NULL) {
        *query = _szQuery;
        *nChunk = _nChunk;
        bResult = true;
    }
    
    Unlock();
    
    return(bResult);
}



//-----------------------------------------------------------------------------
// CJW: We have received some data for the file we are looking for, and we need
//      to send that down to the client.   We dont care if this is the last
//      chunk or not, because the client will disconnect when it has all the
//      chunks it needs.
void Client::QueryResult(int nChunk, char *pData, int nSize, int nLength)
{
    unsigned char pTmp[5];
    
    ASSERT(nChunk >= 0 && pData != NULL && nSize > 0 && nLength > 0);
    
    Lock();
    ASSERT(_nChunk == nChunk);

    // if the stored length is 0, then we store the new length and send a 'L' telegram to the client.
    if (_nLength == 0) {
        _nLength = nLength;

        // ** This would probably be better off being a arithmatic calculation rather than a bitwise calculation... to be portable.
        pTmp[4] = (unsigned char) (nLength & 0xff);
        pTmp[3] = (unsigned char) ((nLength >> 8) & 0xff);
        pTmp[2] = (unsigned char) ((nLength >> 16) & 0xff);
        pTmp[1] = (unsigned char) ((nLength >> 24) & 0xff);
        pTmp[0] = 'L';
        Send((char *)pTmp, 5);
    }
    
    // send the chunk to the client.
    pTmp[4] = (unsigned char) (nSize & 0xff);
    pTmp[3] = (unsigned char) ((nSize >> 8) & 0xff);
    pTmp[2] = (unsigned char) (nChunk & 0xff);
    pTmp[1] = (unsigned char) ((nChunk >> 8) & 0xff);
    pTmp[0] = 'C';
    Send((char *)pTmp, 5);
    Send(pData, nSize);

    // increment the chunk counter.
    _nChunk++;
    
    Unlock();
}



//-----------------------------------------------------------------------------
// CJW: The client is initialising.  This should be the first message we get.  
//      Therefore, we check that are internal values are all in their default
//      settings, otherwise we close the connection.
void Client::ProcessInit(char *pData, int nLength)
{   
    ASSERT(pData != NULL && nLength >= 3);
    ASSERT(pData[0] == 'I');
    
    if (_szQuery != NULL || _nChunk != 0 || _nLength != 0 || _nVersion != 0) {
        Send("Q", 1);
    }
    else {
    
		_nVersion = (pData[1] << 8) | pData[2];
		printf("INIT - Version %d\n", _nVersion);

		// right now we only know how to handle protocol version 1.
		if (_nVersion != 1) {
			Send("Q", 1);
		}
		else {
			// send a reply that indicates that the version is valid.
			Send("V", 1);
		}
	}
}


//-----------------------------------------------------------------------------
// CJW: The client has requested a particular file.  We need to get that 
//      information and store it so that the Server object can then ask us for
//      it, so that it can be passed on the network of nodes.
int Client::ProcessFileRequest(char *pData, int nLength)
{   
    unsigned char nFileLength = 0;

    ASSERT(pData != NULL && nLength >= 0);
    
    // All the existing data needs to be in a particular state or the
    if (_szQuery != NULL || _nChunk != 0 || _nLength != 0 || _nVersion == 0) {
        Send("Q", 1);
    }
    else {
    
		// First we need to get the 1 byte length field.
		ASSERT(pData[0] == 'F');
		nFileLength = (unsigned char) pData[1];

		if (nFileLength == 0) {
			Send("Q", 1);
		}
		else {

			// If we dont have enough data yet, then put the data back that we already have, and then try again during the next loop.
			if (nLength >= nFileLength+2) {
				ASSERT(_szQuery == NULL);
				_szQuery = (char *) malloc(nFileLength + 1);
				ASSERT(_szQuery);

				strncpy(_szQuery, (const char *) &pData[2], nFileLength);
				_szQuery[nFileLength] = '\0';

				// Since we got some data, we need to use it as a heartbeat indicator also.
				_Heartbeat.Clear();
			}
		}
    }
    
    return((int)nFileLength);
}


//-----------------------------------------------------------------------------
// CJW: Since we dont really care that the client received the chunk, we just 
//      assume that it did, we will just remove this message from the data
//      stream and then continue on.
void Client::ProcessChunkReceived(char *pData, int nLength)
{
    ASSERT(pData != NULL && nLength >= 3);
    
    // we're not actually going to do anything with this information at this 
    // stage, but for possible optimisation of resources in the future, we are 
    // keeping it in the protocol.
    _Heartbeat.Clear();
}


//-----------------------------------------------------------------------------
// CJW: This function is used to check the status of our heartbeats, to see if 
//      we need to send one, and also to make sure that we have received one in
//      the allotted time.  It is quite normal to increment the nBeats counter
//      to indicate we've missed one, because the next message will clear it.
//      Therefore that value should kind of fluctuate between 0 and 1.  When
//      it gets to 2, and then moves to 3, then we know we've missed some
//      heartbeats and need to act on it.
bool Client::ProcessHeartbeat(void)
{
    bool bOK = true;
    time_t nTime;
    
    nTime = time(NULL);
    if (nTime > _Heartbeat.nLastCheck) {
        _Heartbeat.nDelay++;
    
        if (_szQuery == NULL) {
            _Heartbeat.nWait++;
        }
    
        if (_Heartbeat.nWait > HEARTBEAT_WAIT) {
            Send("Q", 1);
            bOK = false;
        }
        else {
            if (_Heartbeat.nDelay >= HEARTBEAT_DELAY) {
                _Heartbeat.nDelay = 0;
                _Heartbeat.nBeats ++;
                if (_Heartbeat.nBeats >= HEARTBEAT_MISS) {
                    Send("Q", 1);
                    bOK = false;
                }
                else {
                    Send("H", 1);
                }
            }
        }
    }

    return(bOK);
}



//-----------------------------------------------------------------------------
// CJW: The client has closed the connection.  We need to 
void Client::OnClosed(void)
{
	// todo: not really much to do except log the activity.
}


void Client::OnStalled(char *pData, int nLength)
{
	ASSERT(pData != NULL && nLength > 0);
	
	// todo: not really much to do here because the client has closed the connection.  Just log the event and let the parent clean the object up on its next cycle.
}



