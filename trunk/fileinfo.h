//-----------------------------------------------------------------------------
// fileinfo.h
//
//  Project: pacsrv
//  Author: Clint Webb
//
//		Stored information about a file, such as all the chunks, etc.   As we 
//		recieve chunks of a file from the nodes in the network, we store it in 
//		this linked list.  Since we will eventually know the size of the file, 
//		and the number of chunks, we can easily create an array of pointers.   
//		This will be the easiest to manage.
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

#ifndef __FILEINFO_H
#define __FILEINFO_H

#include <stdio.h>

struct Chunk {

	public:
		Chunk() {
			pData = NULL;
			nChunk = 0;
			nLength = 0;
			nNode = 0;
		}
		
		virtual ~Chunk() {
			if (pData != NULL) {
				free(pData);
				pData = NULL;
				ASSERT(nChunk > 0);
				ASSERT(nLength > 0);
			}
		}

	char *pData;
	int nChunk;
	int nLength;
	int nNode;
};


class FileInfo
{
    public:
        FileInfo();
        virtual ~FileInfo();

        bool GetChunk(int nChunk, char **pData, int *nSize, int *nLength);
		void SaveChunk(char *pData, int nChunk, int nSize);
		bool GetNextChunk(int *nChunk);

		FileInfo * GetNext(void);
		void SetNext(FileInfo *pInfo);
		
		void FileStart(void);
		void FileComplete(void);
		
		void ChunkRequested(int nChunk, int nNode);
		
		void SetFile(char *szFilename);
		void SetLocal(void);
		
		char *GetFilename(void);
		bool IsLocal(void);
		bool IsComplete(void);
		
		int GetUseCount(void);
		void RemoveNode(int nNode);
		
    protected:

    private:
        FileInfo *_pNext;
		char *_szFilename;
		int _nFileLength;
		int _nUseCount;
		bool _bLocal;
		
		struct {
			FILE *pFilePtr;
			int nLocation;
			Chunk chunk;
		} _LocalFile;
		
		struct {
			Chunk **pChunkList;
			int nChunks;
		} _RemoteFile;
};


#endif

