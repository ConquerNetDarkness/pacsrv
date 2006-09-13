//-----------------------------------------------------------------------------
// filelist.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "filelist.h" for more information about this class.
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
#include <unistd.h>


#include <DevPlus.h>

#include "filelist.h"


//---------------------------------------------------------------------
// CJW: Constructor.   Start with an empty list.
FileList::FileList()
{
    _pList = NULL;
}
    
//---------------------------------------------------------------------
// CJW: Deconstructor.  Clean up the linked-list.
FileList::~FileList()
{
    FileInfo *pTmp;

    while(_pList != NULL) {
        pTmp = _pList;
        _pList = pTmp->GetNext();
        pTmp->SetNext(NULL);
        delete pTmp;
    }
}
    

//---------------------------------------------------------------------
// CJW: Go thru the list to find the file that has this filename.  If 
// 		we cant find the file, then return a NULL.
FileInfo * FileList::GetFileInfo(char *szFilename)
{
	FileInfo *pInfo;
	bool bFound = false;
	char *str;
	
	ASSERT(szFilename != NULL);
	
	pInfo = _pList;
	while (pInfo != NULL && bFound == false) {
		
		str = pInfo->GetFilename();
		if (strcmp(szFilename, str) == 0) {
			bFound = true;
		}
		else {
			pInfo = pInfo->GetNext();
		}
	}
	
	if (bFound == false) { pInfo = NULL; }
	return(pInfo);
}



//---------------------------------------------------------------------
// CJW: Attempt to load the file locally.  It is assumed that 
// 		GetFileInfo has already been tried, and it returned nothing.  
// 		We dont want to check for a file, only to have it already in 
// 		our list.
//
//	TODO: Need to check the filename for funky chars...
FileInfo * FileList::LoadFile(char *szFilename)
{
	FileInfo *pInfo = NULL;
	char szPath[2048];
	
	ASSERT(szFilename != NULL);
	
	sprintf(szPath, "/var/cache/pacman/pkg/%s", szFilename);
	if (access(szPath, R_OK) == 0) {
		// The file was found, so we add it to our local list of files.
		
		pInfo = new FileInfo;
		pInfo->SetFile(szFilename);
		pInfo->SetLocal();
		AddFile(pInfo);
	}
	else {
		ASSERT(pInfo == NULL);
	}

	return(pInfo);
}


//---------------------------------------------------------------------
// CJW: Add the file info object to the linked list.
void FileList::AddFile(FileInfo *pInfo)
{
	ASSERT(pInfo != NULL);
	
	pInfo->SetNext(_pList);
	_pList = pInfo;
}


//---------------------------------------------------------------------
// CJW: We need to add a file to the list.  Since we are adding, and 
// 		not loading, then this must be a file we expect to find from 
// 		the network.  So we do not set the fileInfo object to find it 
// 		locally.
FileInfo * FileList::AddFile(char *szFilename)
{
	FileInfo *pInfo = NULL;
	
	ASSERT(szFilename != NULL);
	
	pInfo = new FileInfo;
	pInfo->SetFile(szFilename);
	AddFile(pInfo);

	return(pInfo);
}


//---------------------------------------------------------------------
// CJW: Some of the files in our list may have been completed.  We 
// 		should remove them.
void FileList::Process(void)
{
	FileInfo *pInfo;
	FileInfo *pPrev = NULL;
	FileInfo *pTmp;
	int nCount;
	
	pInfo = _pList;
	while (pInfo != NULL) {
		nCount = pInfo->GetUseCount();
		ASSERT(nCount >= 0);
		
		if (nCount == 0) {
			pTmp = pInfo->GetNext();
			pInfo->SetNext(NULL);
			delete pInfo;
			if (pPrev == NULL)	{ _pList = pTmp; }
			else 				{ pPrev->SetNext(pTmp); }
			pInfo = pTmp;
		}
		else {
			pPrev = pInfo;
			pInfo = pInfo->GetNext();
		}
	}
}


//---------------------------------------------------------------------
// CJW: A node was closed, so we need to go thru the file-list, and 
// 		remove any reference to that node so that we can ask the chunks 
// 		again from other nodes.
void FileList::RemoveNode(int nNode)
{
	FileInfo *pInfo;
	
	ASSERT(nNode > 0);
	
	pInfo = _pList;
	while (pInfo != NULL) {
		pInfo->RemoveNode(nNode);
		pInfo = pInfo->GetNext();
	}
}


//---------------------------------------------------------------------
// CJW: Look in our list to find the first remote file that is 
// 		incomplete.  
FileInfo * FileList::GetNextFile(void)
{
	FileInfo *pInfo = NULL;
	FileInfo *pTmp;
	
	pTmp = _pList;
	while (pTmp != NULL && pInfo == NULL) {
		
		if (pTmp->IsLocal() == false) {
			if (pTmp->IsComplete() == false) {
				pInfo = pTmp;
			}
		}
		
		pTmp = pTmp->GetNext();
	}
	
	return(pInfo);
}




