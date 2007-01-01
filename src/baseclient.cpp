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










