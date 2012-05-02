/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Utils/PacketFileSystem.h,v $
**
** $Revision: 1.3 $
**
** $Date: 2008-03-30 18:38:47 $
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
#ifndef PACKET_FILESYSTEM_H
#define PACKET_FILESYSTEM_H

// The Packet File System redefines the file io routines.
// An fopen call first looks for the file to open in the
// current package. If not found in the package, regular
// fopen is called.
// A file opened that was found in the package will cause
// consecutive calls to file methods to access the package.

// The PacketFileSystem.h should be included after all
// standard include files in order to correctly redefine
// the file io methods.



int pkg_load(const char* filename, char* key, int keyLen);
void pkg_unload();

#ifdef USE_PACKET_FS

int pkg_file_exists(const char* fname);

FILE* pkg_fopen(const char* fname, const char* mode);
int pkg_fclose(FILE* file);
size_t pkg_fwrite(const void* buffer, size_t size, size_t count, FILE* file);
size_t pkg_fread(void* buffer, size_t size, size_t count, FILE* file);
int pkg_fseek(FILE* file, long offset, int origin);
long pkg_ftell(FILE* file);
char *pkg_fgets(char* string, int n, FILE* file);

#define fopen   pkg_fopen
#define fclose  pkg_fclose
#define fread   pkg_fread
#define fwrite  pkg_fwrite
#define fseek   pkg_fseek
#define ftell   pkg_ftell
#define fgets   pkg_fgets

#else

#define pkg_file_exists(fname) 0

#endif

#endif
