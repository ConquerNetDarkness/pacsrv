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
#include <stdlib.h>
#include <string.h>

#include <DevPlus.h>

#include "client.h"
#include "config.h"
#include "logger.h"


//-----------------------------------------------------------------------------
// CJW: If the client doesnt send us a File Request within this number of seconds, then we will close the connection.  The client should be able to send the simple request within 5 seconds surely.
#define HEARTBEAT_WAIT      5


//-----------------------------------------------------------------------------
// CJW: Constructor.  Initialise the values, and our state.  Since we havent 
//      actually received any data from the client, we have an UNKNOWN state.
Client::Client()
{
    _szQuery  = NULL;
    _nChunk   = 0;
    _nLength  = 0;
    _nVersion = 0;
}


//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Clean up everything we created.  
Client::~Client()
{
    if (_szQuery != NULL) {
        free(_szQuery);
        _szQuery = NULL;
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
bool Client::Process(bool bCheck)
{
    bool bOK = true;
    int nLength;
    char cCommand;
    
    
    // read data from the socket.
    bOK = ReadData();
    
    // process the first telegram in our data queue.
    if (bOK != false) {
		
        nLength = _DataIn.Length();
        if (nLength >= 1) {
            cCommand = GetCommand();
            nLength --;
    
            switch(cCommand) {
                case 'I':       // INIT
                    bOK = ProcessInit(nLength);
                    break;
    
                case 'H':       // Heartbeat
                    _Heartbeat.Clear();
                    break;
    
                case 'F':       // File Request.
                    bOK = ProcessFileRequest(nLength);
                    break;
    
                case 'R':       // Chunk received.
                    // We dont really care about this one, but must remove it from the queue if we get it.  Currently the client is not programmed to send this.
                    bOK = ProcessChunkReceived(nLength);
                    break;

                default:
                    // we got a char we werent expecting, so we want to close the connection.
                    Close();
                    bOK = false;
                    break;
            }
    
        }
    
        // process heartbeat if we are supposed to.
        if (bOK != false) {
            bOK = ProcessHeartbeat();
        }
    
        // Send any data that is in our outbound queue.  Since we got this far, if the Process functions returned false, then we need to close the socket, we wont wait for the client to disconnect.
        bOK = SendData();
        if (bOK != false) {
            Close();
        }
    }
    
    return(bOK);
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
    ASSERT((_szQuery == NULL && _nChunk == 0) || (_szQuery != NULL && _nChunk >= 0));
    
    if (_szQuery != NULL) {
        *query = _szQuery;
        *nChunk = _nChunk;
        bResult = true;
    }
    
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
        _DataOut.Add((char *)pTmp, 5);
    }
    
    // send the chunk to the client.
    pTmp[4] = (unsigned char) (nSize & 0xff);
    pTmp[3] = (unsigned char) ((nSize >> 8) & 0xff);
    pTmp[2] = (unsigned char) (nChunk & 0xff);
    pTmp[1] = (unsigned char) ((nChunk >> 8) & 0xff);
    pTmp[0] = 'C';
    _DataOut.Add((char *)pTmp, 5);
    _DataOut.Add(pData, nSize);

    // increment the chunk counter.
    _nChunk++;
}



//-----------------------------------------------------------------------------
// CJW: The client is initialising.  This should be the first message we get.  
//      Therefore, we check that are internal values are all in their default
//      settings, otherwise we close the connection.
bool Client::ProcessInit(int nLength)
{   
    Logger *pLogger;
    bool bOK = true;
    unsigned char *pData;

    ASSERT(nLength >= 0);
    
    if (_szQuery != NULL || _nChunk != 0 || _nLength != 0 || _nVersion != 0) {
        _DataOut.Add("Q", 1);
        bOK = false;
    }
    else {
    
        // If we dont have enough data from the socket yet, then we need to put this character back in the queue to process again.  Heartbeat wont be incremented, so if we dont ever get enough, then we will eventually close the socket.
        if (nLength < 2) {
            _DataIn.Push("I", 1);
        }
        else {
    
            // we now need to get the two byte protocol version parameters.
            pData = (unsigned char *) _DataIn.Pop(2);
            ASSERT(pData != NULL);
    
            _nVersion = (pData[0] << 8) | pData[1];
            printf("INIT - Version %d\n", _nVersion);
    
                            // right now we only know how to handle protocol version 1.
            if (_nVersion != 1) {
                _DataOut.Add("Q", 1);
                bOK = false;
            }
            else {
            }
        }
    }
    
    return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: The client has requested a particular file.  We need to get that 
//      information and store it so that the Server object can then ask us for
//      it, so that it can be passed on the network of nodes.
bool Client::ProcessFileRequest(int nLength)
{   
    Logger *pLogger;
    bool bOK = true;
    unsigned char *pData;
    unsigned char nFileLength;

    ASSERT(nLength >= 0);
    
    // All the existing data needs to be in a particular state or the
    if (_szQuery != NULL || _nChunk != 0 || _nLength != 0 || _nVersion == 0) {
        _DataOut.Add("Q", 1);
        bOK = false;
    }
    else {
    
        // If we dont have enough data from the socket yet, then we need to put this character back in the queue to process again.  Heartbeat wont be incremented, so if we dont ever get enough, then we will eventually close the socket.
        if (nLength < 1) {
            _DataIn.Push("F", 1);
        }
        else {
    
            // First we need to get the 1 byte length field.
            pData = (unsigned char *) _DataIn.Pop(1);
            ASSERT(pData != NULL);
            nFileLength = (unsigned char) pData[0];
            free(pData);
    
            if (nFileLength == 0) {
                _DataOut.Add("Q", 1);
                bOK = false;
            }
            else {

                // If we dont have enough data yet, then put the data back that we already have, and then try again during the next loop.
                if (nLength < nFileLength+1) {
                    _DataIn.Push((char *) &nFileLength, 1);
                    _DataIn.Push("F", 1);
                }
                else {

                    pData = (unsigned char *) _DataIn.Pop(nFileLength);
                    ASSERT(pData != NULL);
    
					ASSERT(_szQuery == NULL);
                    _szQuery = (char *) malloc(nFileLength + 1);
                    ASSERT(_szQuery);
    
                    strncpy(_szQuery, (const char *) pData, nFileLength);
                    _szQuery[nFileLength] = '\0';
    
                    free(pData);
    
                    // Since we got some data, we need to use it as a heartbeat indicator also.
                    _Heartbeat.Clear();
                }
            }
        }
    }
    
    return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: Since we dont really care that the client received the chunk, we just 
//      assume that it did, we will just remove this message from the data
//      stream and then continue on.
bool Client::ProcessChunkReceived(int nLength)
{
    bool bOK = true;
    char *pData;
    
    ASSERT(nLength >= 0);
    
    if (nLength >= 2) {
        pData = _DataIn.Pop(2);
        ASSERT(pData);
        free(pData);
        _Heartbeat.Clear();
    }
    
    return(bOK);
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
            _DataOut.Push("Q", 1);
            bOK = false;
        }
        else {
            if (_Heartbeat.nDelay >= HEARTBEAT_DELAY) {
                _Heartbeat.nDelay = 0;
                _Heartbeat.nBeats ++;
                if (_Heartbeat.nBeats >= HEARTBEAT_MISS) {
                    _DataOut.Push("Q", 1);
                    bOK = false;
                }
                else {
                    _DataOut.Push("H", 1);
                }
            }
        }
    }

    return(bOK);
}



