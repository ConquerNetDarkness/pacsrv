//-----------------------------------------------------------------------------
// fileinfo.cpp
//
//  Project: pacsrv
//  Author: Clint Webb
// 
//      See "fileinfo.h" for more information about this class.
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

#include <DevPlus.h>

#include "fileinfo.h"
#include "common.h"


//-----------------------------------------------------------------------------
// CJW: Constructor.  Initialise our member variables.
FileInfo::FileInfo()
{
    _pNext = NULL;
	_szFilename = NULL;
	_nFileLength = 0;
	_nUseCount = 0;
	_bLocal = false;
		
	_LocalFile.pFilePtr = NULL;
	_LocalFile.nLocation = 0;
		
	_RemoteFile.pChunkList = NULL;
	_RemoteFile.nChunks = 0;
}
    
//-----------------------------------------------------------------------------
// CJW: Deconstructor... nothing to do here.
FileInfo::~FileInfo()
{
	int i;
	
    ASSERT(_pNext != NULL);
	ASSERT(_nUseCount == 0);
	if (_szFilename != NULL) {
		free(_szFilename);
		_szFilename = NULL;
	}
	if (_LocalFile.pFilePtr != NULL) {
		ASSERT(_bLocal == true);
		fclose(_LocalFile.pFilePtr);
		_LocalFile.pFilePtr = NULL;
	}
	
	if (_RemoteFile.pChunkList != NULL) {
		ASSERT(_RemoteFile.nChunks > 0);
		for(i=0; i<_RemoteFile.nChunks; i++) {
			if (_RemoteFile.pChunkList[i] != NULL) {
				delete _RemoteFile.pChunkList[i];
				_RemoteFile.pChunkList[i] = NULL;
			}
		}
		
		free(_RemoteFile.pChunkList);
		_RemoteFile.pChunkList = NULL;
		_RemoteFile.nChunks = 0;
	}
}
    
    
//-----------------------------------------------------------------------------
// CJW: Since this is a part of a linked list, we need to provide some 
//		functionality to traverse the list.
FileInfo * FileInfo::GetNext(void)
{
	return(_pNext);
}

//-----------------------------------------------------------------------------
// CJW: Add the object to the list.  This can only be done if there is no 
// 		object already in the next prosition.
void FileInfo::SetNext(FileInfo *pInfo)
{
	ASSERT(_pNext == NULL);
	ASSERT(pInfo != NULL);
	
	_pNext = pInfo;
}



//-----------------------------------------------------------------------------
// CJW: Since we are caching the file information, we need to keep track of how 
// 		many nodes or clients are currently using this file.  This function 
// 		indicates that one of the nodes or clients is done with this file.  If 
// 		the usage count gets down to 0, then FileList may remove it from the 
// 		list.
void FileInfo::FileComplete(void)
{
	ASSERT(_nUseCount > 0);
	_nUseCount --;
	ASSERT(_nUseCount >= 0);
}


//-----------------------------------------------------------------------------
// CJW: Return the name of the file that this object represents.  We should 
// 		know what the name is at this point.
char * FileInfo::GetFilename(void)
{
	ASSERT(_szFilename != NULL);
	return(_szFilename);
}


//-----------------------------------------------------------------------------
// CJW: Return a chunk of the file if we know it.  If the file is local, then 
// 		we will open the file (if its not already open), and get the chunk and 
// 		return it, without storing it.   If the file is not local (is remote), 
// 		then we will look to see if we have that chunk.  If we do, we will 
// 		return it and a true, otherwise we will return a false without 
// 		disturbing the parameters supplied.
bool FileInfo::GetChunk(int nChunk, char **pData, int *nSize, int *nLength)
{
	bool bGotIt = false;
	char szPath[2048];
	long nLoc;
	size_t nLen;
	int nReadSize;
	
	ASSERT(nChunk > 0);
	ASSERT(pData != NULL);
	ASSERT(nSize != NULL);
	ASSERT(nLength != NULL);
	
	ASSERT(_szFilename != NULL);
	
	if (_bLocal == true) {
		if (_LocalFile.pFilePtr == NULL) {
			sprintf(szPath, "%s/%s", PKG_PATH, _szFilename);
			_LocalFile.pFilePtr = fopen(szPath, "rb");
			if (_LocalFile.pFilePtr == NULL) {
				_bLocal = false;
			}
			else {
				fseek(_LocalFile.pFilePtr, 0, SEEK_END);
				_nFileLength = ftell(_LocalFile.pFilePtr);
				fseek(_LocalFile.pFilePtr, 0, SEEK_SET);
			}
			
			ASSERT(_LocalFile.nLocation == 0);
		}
		
		if (_LocalFile.chunk.nChunk == nChunk) {
			ASSERT(_LocalFile.chunk.pData != NULL);
			ASSERT(_LocalFile.chunk.nLength > 0);
			ASSERT(_nFileLength > 0);
			
			*pData = _LocalFile.chunk.pData;
			*nSize = _LocalFile.chunk.nLength;
			*nLength = _nFileLength;
			
			bGotIt = true;
		}
		else {
			
			if (_LocalFile.pFilePtr != NULL) {
				nLoc = nChunk * MAX_CHUNK_SIZE;
				if (nLoc != _LocalFile.nLocation) {
					if (fseek(_LocalFile.pFilePtr, nLoc, SEEK_SET) != 0) {
						fclose(_LocalFile.pFilePtr);
						_LocalFile.pFilePtr = NULL;
						_LocalFile.nLocation = nLoc;
						_bLocal = false;
					}
				}
			}
			
			if (_LocalFile.pFilePtr != NULL) {
				
				// determine the size of the chunk we are getting.
				nReadSize = _nFileLength - _LocalFile.nLocation;
				ASSERT(nReadSize >= 0);
				if (nReadSize > MAX_CHUNK_SIZE) { nReadSize = MAX_CHUNK_SIZE; }
				if (nReadSize > 0) {
				
					if (_LocalFile.chunk.pData == NULL) {
						_LocalFile.chunk.pData = (char *) malloc(MAX_CHUNK_SIZE);
					}
					ASSERT(_LocalFile.chunk.pData != NULL);
					
					nLen = fread(_LocalFile.chunk.pData, nReadSize, 1, _LocalFile.pFilePtr);
					if (nLen == 0) {
						// we shouldnt get to this state, because we are checking for file lengths.  If we get here, then something happened to the file (which is possible).
						_bLocal = false;
						fclose(_LocalFile.pFilePtr);
						_LocalFile.pFilePtr = NULL;
						_LocalFile.nLocation = 0;
					}
					else {
						ASSERT(nLen == nReadSize);
						_LocalFile.chunk.nLength = nLen;
						_LocalFile.nLocation += nLen;
						ASSERT(_LocalFile.nLocation <= _nFileLength);
						
						*pData = _LocalFile.chunk.pData;
						*nSize = _LocalFile.chunk.nLength;
						*nLength = _nFileLength;
			
						bGotIt = true;
					}
				}
			}
		}
	}
	else {
		// Its not a local file, so see if we have this chunk in our array.
		
		ASSERT(nChunk <= _RemoteFile.nChunks);
		ASSERT(_RemoteFile.pChunkList != NULL);
		
		if (_RemoteFile.pChunkList[nChunk-1] != NULL) {
			ASSERT(_RemoteFile.pChunkList[nChunk-1]->pData != NULL);
			ASSERT(_RemoteFile.pChunkList[nChunk-1]->nChunk == nChunk);
			ASSERT(_RemoteFile.pChunkList[nChunk-1]->nLength > 0);
			ASSERT(_RemoteFile.pChunkList[nChunk-1]->nLength <= MAX_CHUNK_SIZE);
			
			*pData = _RemoteFile.pChunkList[nChunk-1]->pData;
			*nSize = _RemoteFile.pChunkList[nChunk-1]->nLength;
			*nLength = _nFileLength;
			
			bGotIt = true;
		}
	}
	
	return(bGotIt);
}


//-----------------------------------------------------------------------------
// CJW: When a chunk is requested by a node, we need to make a note of it... so 
// 		if that node gets closed, we know we need to ask for this chunk again 
// 		from a different node.
void FileInfo::ChunkRequested(int nChunk, int nNode)
{
	ASSERT(nChunk > 0 && nNode > 0);
	
	ASSERT(nChunk <= _RemoteFile.nChunks);
	ASSERT(_RemoteFile.pChunkList != NULL);
	ASSERT(_RemoteFile.pChunkList[nChunk-1] == NULL);
	
	_RemoteFile.pChunkList[nChunk-1] = new Chunk;
	
	_RemoteFile.pChunkList[nChunk-1]->nChunk = nChunk;
	_RemoteFile.pChunkList[nChunk-1]->nNode  = nNode;
	
	ASSERT(_RemoteFile.pChunkList[nChunk-1]->pData == NULL);
}


//-----------------------------------------------------------------------------
// CJW: We've received a chunk of a file, from a network node.   So we save it.  
// 		We should only receive chunks that we dont already have, so we also 
// 		check for that.   We also need to make sure that the chunk-size matches 
// 		what we expect it should be (all should be MAX_CHUNK_SIZE except for 
// 		the last chunk of the file).
void FileInfo::SaveChunk(char *pData, int nChunk, int nSize)
{
	ASSERT(pData != NULL);
	ASSERT(nChunk > 0);
	ASSERT(nSize > 0);
	
	ASSERT(nChunk <= _RemoteFile.nChunks);
	ASSERT(_RemoteFile.pChunkList != NULL);
	ASSERT(_RemoteFile.pChunkList[nChunk-1] != NULL);
	
	ASSERT(_RemoteFile.pChunkList[nChunk-1]->nNode > 0);
	ASSERT(_RemoteFile.pChunkList[nChunk-1]->pData == NULL);
	
	_RemoteFile.pChunkList[nChunk-1]->pData = pData;
	_RemoteFile.pChunkList[nChunk-1]->nChunk = nChunk;
	_RemoteFile.pChunkList[nChunk-1]->nLength = nSize;
}


//-----------------------------------------------------------------------------
// CJW: If we lose contact we a node, we need to go thru our chunk list (if 
// 		this file is not local), and remove any outstanding chunk requests to 
// 		that node.  Those chunks should automatically be requested again to 
// 		another nodes.
void FileInfo::RemoveNode(int nNode)
{
	int nCount;
	
	ASSERT(nNode > 0);
	if (_bLocal == false) {
		ASSERT(_RemoteFile.nChunks > 0);
		ASSERT(_RemoteFile.pChunkList != NULL);
		
		for (nCount=0; nCount < _RemoteFile.nChunks; nCount++) {
			if (_RemoteFile.pChunkList[nCount] != NULL) {
				if (_RemoteFile.pChunkList[nCount]->nNode == nNode) {
					_RemoteFile.pChunkList[nCount]->nNode = 0;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// CJW: If there are any chunks we need for this file, return it.  We will 
// 		return false if there are no more chunks needed for this file.  We will 
// 		return true if all the chunks have been asked.
bool FileInfo::GetNextChunk(int *nChunk)
{
	bool bDone = true;
	int nCount;
	
	ASSERT(nChunk != NULL);
	ASSERT(_bLocal == false);
	
	ASSERT(_RemoteFile.nChunks > 0);
	ASSERT(_RemoteFile.pChunkList != NULL);
	
	for (nCount=0; nCount < _RemoteFile.nChunks && bDone == true; nCount++) {
		if (_RemoteFile.pChunkList[nCount] != NULL) {
			if (_RemoteFile.pChunkList[nCount]->nNode == 0) {
				_RemoteFile.pChunkList[nCount]->nNode = 0;
				*nChunk = nCount + 1;
				bDone = false;
			}
		}
		else {
			*nChunk = nCount+1;
			bDone = false;
		}
	}
	
	return(bDone);
}


//-----------------------------------------------------------------------------
// CJW: Go thru the chunk list to see if we have all the chunks requested for.  
// 		If we do, then return true.  If we havent asked for all the chunks, 
// 		then return false.  This is not checking to see if we have received all 
// 		the chunks, this is checking to see if we have asked for all the 
// 		chunks.
//
//	**	We are going to start searching the list backwards because that would 
//		be the most efficient because most likely the last chunk will be the 
//		last one to fill, and we dont have to keep looking from the begining 
//		where chunks will have been requested.  
bool FileInfo::IsComplete(void)
{
	bool bComplete = true;
	int nCount;
	
	ASSERT(_bLocal == false);
	
	ASSERT(_RemoteFile.nChunks > 0);
	ASSERT(_RemoteFile.pChunkList != NULL);
	
	for (nCount=_RemoteFile.nChunks-1; nCount >= 0 && bComplete == true; nCount--) {
		if (_RemoteFile.pChunkList[nCount] != NULL) {
			if (_RemoteFile.pChunkList[nCount]->nNode == 0) {
				bComplete = false;
			}
		}
		else {
			bComplete = false;
		}
	}
	
	return(bComplete);

}


//-----------------------------------------------------------------------------
// CJW: Return true if this file is being filled from a local file, rather than 
// 		from the network of nodes.
bool FileInfo::IsLocal(void)
{
	return(_bLocal);
}


//-----------------------------------------------------------------------------
// CJW: Indicate that this file is going to be in use by a node (or client).   
// 		We do this because we dont want this object to be destroyed before we 
// 		have finished using it.
void FileInfo::FileStart(void)
{
}



void FileInfo::SetFile(char *szFilename)
{
	ASSERT(szFilename != NULL);
	ASSERT(_szFilename == NULL);
	
}



void FileInfo::SetLength(int nLength)
{
	ASSERT(nLength > 0);
	ASSERT(_szFilename != NULL);
	ASSERT(_nFileLength == 0);
	_nFileLength = nLength;
}





