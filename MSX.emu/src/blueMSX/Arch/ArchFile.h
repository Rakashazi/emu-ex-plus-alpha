/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchFile.h,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-03-30 18:38:39 $
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
#ifndef ARCH_FILE_H
#define ARCH_FILE_H

#include "Properties.h"
#include "MediaDb.h"

int archCreateDirectory(const char* pathname);
const char* archGetCurrentDirectory();
void archSetCurrentDirectory(const char* pathname);
int archFileExists(const char* fileName);

// File dialogs:
char* archFilenameGetOpenRom(Properties* properties, int cartSlot, RomType* romType);

char* archFilenameGetOpenDisk(Properties* properties, int drive, int allowCreate);

char* archFilenameGetOpenCas(Properties* properties);

char* archFilenameGetOpenHarddisk(Properties* properties, int drive, int allowCreate);

char* archFilenameGetSaveCas(Properties* properties, int* type);
char* archFileSave(char* title, char* extensionList, char* defaultDir, char* extensions, int* selectedExtension, char* defExt);

char* archFilenameGetOpenState(Properties* properties);
char* archFilenameGetOpenCapture(Properties* properties);

char* archFilenameGetSaveState(Properties* properties);

char* archDirnameGetOpenDisk(Properties* properties, int drive);

char* archFilenameGetOpenRomZip(Properties* properties, int cartSlot, const char* fname, const char* fileList, int count, int* autostart, int* romType);

char* archFilenameGetOpenDiskZip(Properties* properties, int drive, const char* fname, const char* fileList, int count, int* autostart);

char* archFilenameGetOpenCasZip(Properties* properties, const char* fname, const char* fileList, int count, int* autostart);

char* archFilenameGetOpenAnyZip(Properties* properties, const char* fname, const char* fileList, int count, int* autostart, int* romType);


#endif
