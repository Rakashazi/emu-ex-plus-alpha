/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Utils/ZipFromMem.c,v $
**
** $Revision: 0.0 $
**
** $Date: 2008/03/30 21:38:43 $
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
#include "ZipFromMem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/******************************************************************************
*** Description
***     Functions to support reading zip files from memory resources.
***
******************************************************************************/

typedef struct {
    unsigned long index;
    unsigned long size;
} MEMFILE;

void *fopen_mem_func(void *opaque, const char *filename, int mode)
{
    MEMFILE *memfile = (MEMFILE *)opaque;
    memfile->index = 0;
    return (void *)filename;
}

unsigned long fread_mem_func(void *opaque, void *stream, void *buf, unsigned long size)
{
    MEMFILE *memfile = (MEMFILE *)opaque;
    if( memfile->index + size > memfile->size ) {
        size = memfile->size - memfile->index;
    }
    memcpy(buf, (char*)stream + memfile->index, size);
    memfile->index += size;
    return size;
}

unsigned long fwrite_mem_func(void *opaque, void *stream, const void *buf, unsigned long size)
{
    return -1;
}

long ftell_mem_func(void *opaque, void *stream)
{
    MEMFILE *memfile = (MEMFILE *)opaque;
    return memfile->index;
}

long fseek_mem_func(void *opaque, void *stream, unsigned long offset, int origin)
{
    MEMFILE *memfile = (MEMFILE *)opaque;
    switch (origin)
    {
    case ZLIB_FILEFUNC_SEEK_CUR :
        memfile->index += offset;
        break;
    case ZLIB_FILEFUNC_SEEK_END :
        memfile->index = memfile->size - offset;
        break;
    case ZLIB_FILEFUNC_SEEK_SET :
        memfile->index = offset;
        break;
    default: return -1;
    }
    if( memfile->index > memfile->size ) memfile->index = memfile->size;
    if( memfile->index < 0 ) memfile->index = 0;
    return 0;
}

int fclose_mem_func(void *opaque, void *stream)
{
    return 0;
}

int ferror_mem_func(void *opaque, void *stream)
{
    return 0;
}

void fill_fopen_memfunc(zlib_filefunc_def *pzlib_filefunc_def, unsigned int size)
{
    MEMFILE *memfile = (MEMFILE *)malloc(sizeof(MEMFILE));
    memfile->size = size;
    memfile->index = 0;
    pzlib_filefunc_def->opaque = memfile;
    pzlib_filefunc_def->zopen_file = fopen_mem_func;
    pzlib_filefunc_def->zread_file = fread_mem_func;
    pzlib_filefunc_def->zwrite_file = fwrite_mem_func;
    pzlib_filefunc_def->ztell_file = ftell_mem_func;
    pzlib_filefunc_def->zseek_file = fseek_mem_func;
    pzlib_filefunc_def->zclose_file = fclose_mem_func;
    pzlib_filefunc_def->zerror_file = ferror_mem_func;
}

void free_fopen_memfunc(zlib_filefunc_def *pzlib_filefunc_def)
{
    free(pzlib_filefunc_def->opaque);
}

