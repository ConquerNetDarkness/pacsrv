//-----------------------------------------------------------------------------
// logger.cpp
//
//	Project: pacsrv
//	Author: Clint Webb
// 
//		See "logger.h" for more information about this class.  
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
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include "logger.h"

//---------------------------------------------------------------------------
// CJW: 

FILE *Logger::_pFile = NULL;
int Logger::_nInstances = 0;
DpLock Logger::_xLock;
char *Logger::_pFileStr = NULL;
int Logger::_nMax = 0;

Logger::Logger()
{
    _xLock.Lock();
    ASSERT(_nInstances >= 0);

    if (_nInstances == 0) {
        ASSERT(_pFileStr == NULL);
        ASSERT(_pFile == NULL);
    }

    _nInstances ++;
    ASSERT(_nInstances > 0);

    _xLock.Unlock();
}


Logger::~Logger()
{
    _xLock.Lock();
    ASSERT(_nInstances > 0);

    _nInstances --;
    ASSERT(_nInstances >= 0);

    if (_nInstances == 0) {
        if(_pFile != NULL) {
            fclose(_pFile);
        }
    }

    _xLock.Unlock();
}


void Logger::Init(char *file, int max)
{
    ASSERT(file != NULL);
    ASSERT(max > 0);
    
    _xLock.Lock();
    ASSERT(_pFile == NULL);
    ASSERT(_pFileStr == NULL);
    ASSERT(_nMax == 0);
    
    _pFileStr = (char *) malloc(strlen(file) + 1);
    strcpy(_pFileStr, file);
    
    _nMax = max;
    
    CycleLogs();
    _pFile = fopen(file, "a");
    ASSERT(_pFile);
    
    _xLock.Unlock();
}


void Logger::CycleLogs(void) 
{
    char str1[256], str2[256];
    int i;
    
    ASSERT(_pFile == NULL);
    ASSERT(_pFileStr != NULL);
    ASSERT(_nMax > 0);

    sprintf(str1, "%s-%02d", _pFileStr, _nMax);
    unlink(str1);

    i = _nMax;
    while (i > 0) {
        i--;
        
        sprintf(str1, "%s-%02d", _pFileStr, i);
        sprintf(str2, "%s-%02d", _pFileStr, i+1);
        rename(str1, str2);
    }
    
    sprintf(str1, "%s", _pFileStr);
    sprintf(str2, "%s-00", _pFileStr);
    rename(str1, str2);
}

void Logger::WriteOut(char sub, char *log)
{
    char ldate[32]; 
    time_t currtime;
    struct tm vtime;
    
    time(&currtime);
    localtime_r(&currtime, &vtime);
    sprintf(ldate, "%04d%02d%02d-%02d%02d%02d", vtime.tm_year+1900, vtime.tm_mon+1, vtime.tm_mday, vtime.tm_hour, vtime.tm_min, vtime.tm_sec);
    
    _xLock.Lock();
    ASSERT(_pFile != NULL);
    fprintf(_pFile, "%s,%c,%s\n", ldate, sub, log);
    fflush(_pFile);
    _xLock.Unlock();
}


void Logger::Test(char *str, ...)
{
    va_list ap;
    char *buffer;

    ASSERT(str != NULL);

    buffer = (char *) malloc(MAX_LOG_LINE);
    if (buffer) {
        va_start(ap, str);
        vsprintf(buffer, str, ap);
        va_end(ap);
        WriteOut('T', buffer);
        free(buffer);
    }
}

void Logger::System(char *str, ...)
{
    va_list ap;
    char *buffer;

    ASSERT(str != NULL);

    buffer = (char *) malloc(MAX_LOG_LINE);
    if (buffer) {
        va_start(ap, str);
        vsprintf(buffer, str, ap);
        va_end(ap);
        WriteOut('S', buffer);
        free(buffer);
    }
}

void Logger::Error(char *str, ...)
{
    va_list ap;
    char *buffer;

    ASSERT(str != NULL);

    buffer = (char *) malloc(MAX_LOG_LINE);
    if (buffer) {
        va_start(ap, str);
        vsprintf(buffer, str, ap);
        va_end(ap);
        WriteOut('E', buffer);
        free(buffer);
    }
}

