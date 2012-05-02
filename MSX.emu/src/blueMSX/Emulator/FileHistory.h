/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Emulator/FileHistory.h,v $
**
** $Revision: 1.10 $
**
** $Date: 2008/05/06 12:52:10 $
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
#ifndef FILE_HISTORY_H
#define FILE_HISTORY_H

#include "MediaDb.h"
#include "Properties.h"

const char* stripPath(const char* filename);
const char* stripPathExt(const char* filename);

#ifndef NO_FILE_HISTORY
void verifyFileHistory(char* history, RomType* historyType);
void updateFileHistory(char* history, RomType* historyType, char* filename, RomType romType);
#endif

int tempStateExists();
int fileExist(char* filename, char* zipFile);
char* fileGetNext(char* filename, char* zipFile);
void setExtendedRomName(int drive, const char* name);
void updateExtendedRomName(int drive, char* filename, char* zipFile);
void updateExtendedDiskName(int drive, char* filename, char* zipFile);
void updateExtendedCasName(int drive, char* filename, char* zipFile);

int createSaveFileBaseName(char* fileBase,Properties* properties, int useExtendedName);
char* generateSaveFilename(Properties* properties, char* directory, char* prefix, char* extension, int digits);

#endif


