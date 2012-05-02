/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Utils/ziphelper.h,v $
**
** $Revision: 1.4 $
**
** $Date: 2008/03/30 18:38:47 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#ifndef ZIPHELPER_H
#define ZIPHELPER_H

#include "unzip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ZIP_EXTRACT_CB)(int, int);

void memZipFileSystemCreate(int maxFiles);
void memZipFileSystemDestroy();

void zipCacheReadOnlyZip(const char* zipName);
void* zipLoadFile(const char* zipName, const char* fileName, int* size);
int zipSaveFile(const char* zipName, const char* fileName, int append, void* buffer, int size);
int zipFileExists(const char* zipName, const char* fileName);
char* zipGetFileList(const char* zipName, const char* ext, int* count);
int zipHasFileType(char* zipName, char* ext);
int zipExtractCurrentfile(unzFile uf, int overwrite, const char* password);
int zipExtract(unzFile uf, int overwrite, const char* password, ZIP_EXTRACT_CB progress_callback);
void* zipCompress(void* buffer, int size, unsigned long* retSize);
// Note: retSize in zipUncompress is input/output parameter and need to be set to unzipped buffer size
void* zipUncompress(void* buffer, int size, unsigned long* retSize); 

#ifdef __cplusplus
}
#endif

#endif
