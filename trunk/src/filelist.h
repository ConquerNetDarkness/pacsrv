//-----------------------------------------------------------------------------
// filelist.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
//      This object manages a linked list of FileInfo objects.
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

#ifndef __FILELIST_H
#define __FILELIST_H

#include "fileinfo.h"

class FileList 
{
    private:
        FileInfo *_pList;
    
    public:
        FileList();
        virtual ~FileList();

        FileInfo * AddFile(char *szFilename);
        FileInfo * LoadFile(char *szFilename);
        FileInfo * GetFileInfo(char *szFilename);
		FileInfo * GetNextFile(void);
		void Process(void);
		
		void RemoveNode(int nNode);
		
    
    protected:
		void AddFile(FileInfo *pInfo);
};


#endif

