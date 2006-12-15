//-----------------------------------------------------------------------------
// baseclient.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      see "baseclient.h" for more details on this class.
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

#include <stdlib.h>

#include "baseclient.h"


//-----------------------------------------------------------------------------
// CJW: Since the client app will not likely be sending very much information 
//      to us, this class is not really optimised for receiving a lot of data.
//      Therefore, if not all the data from the client fits in this chunk, it
//      will stay on the socket until the next iteration.  This should be fine
//      for our needs here, but if you add a telegram where the client is
//      sending a large amount of data, you might want to think about how this
//      is handled.
#define MAX_PACKET_SIZE     393216


//-----------------------------------------------------------------------------
// CJW: Constructor.  
BaseClient::BaseClient()
{
}


//-----------------------------------------------------------------------------
// CJW: Deconstructor.  Clean up everything we created.  
BaseClient::~BaseClient()
{
}





//-----------------------------------------------------------------------------
// CJW: Read data from the socket.  If we get any data, put it in our incoming 
//      data queue.  If everything is ok, then we will return true, even if
//      there was no data to read.  If the socket was closed, then we will
//      return false.
bool BaseClient::ReadData(void)
{
    bool bOK = true;
    char *pData;
    int nLength;
    
	ASSERT(MAX_PACKET_SIZE > 0);
	
	if (IsConnected() == true) {
	
		pData = (char *) malloc(MAX_PACKET_SIZE);
		if (pData != NULL) {
			nLength = Receive(pData, MAX_PACKET_SIZE);
			ASSERT(nLength <= MAX_PACKET_SIZE);
			if (nLength > 0) {
				_DataIn.Add(pData, nLength);
			}
			else if (nLength < 0) {
				bOK = false;
			}
			free(pData);
			pData = NULL;
		}
		else {
			bOK = false;
		}
	}

    return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: Send some data if there is any in our out-bound queue.    Since we will
//      very likely be sending out chunks that are larger than the max packet
//      size, we should keep sending until we either run out of data to send,
//      or until the socket says it cant hold any more.  As long as we have
//      data to send, we will send it, but we should keep track of the
//      performance of the system what it is running, but it should be ok.
//      Might need to put a bit of a limit in here to let it loop around,
//      because we still need to make sure we are receiving heartbeats from the
//      client (which I guess we would do if the buffer got full, so it should
//      be ok).
bool BaseClient::SendData(void)
{
    bool bOK = true;
    char *pData;
    int nLength, nSent;
    bool bStop = false;

	if (IsConnected() == false) {
		bStop = true;
	}
	
    while(bStop == false) {
        nLength = _DataOut.Length();
        if (nLength > 0) {
            if (nLength > MAX_PACKET_SIZE) { nLength = MAX_PACKET_SIZE; }
    
            pData = _DataOut.Pop(nLength);
            ASSERT(pData != NULL);
    
            nSent = Send(pData, nLength);
            ASSERT(nSent <= nLength);
            if (nSent < 0) {
                bOK = false;    // couldnt send.. socket is closed.
                bStop = true;
            }
            else if (nSent < nLength) {
                _DataOut.Push(&pData[nSent], nLength - nSent);
                bStop = true;
            }
    
            free(pData);
        }
        else {
            bStop = true;
        }
    }
    
    return(bOK);
}


//-----------------------------------------------------------------------------
// CJW: Get Command char from the incoming data stream.  It is assumed that the
//      next one will actually be a command we need to process.  We also need
//      to assume that there will be a command char there, so the length of the
//      queue should be retrieved and checked first before calling this
//      function.
char BaseClient::GetCommand(void)
{
    char ch = '\0';
    char *pData;
    
    pData = _DataIn.Pop(1);
    ASSERT(pData != NULL);
    ch = pData[0];
    free(pData);
    pData = NULL;

    return(ch);
}




