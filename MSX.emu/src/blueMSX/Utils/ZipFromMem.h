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
#ifndef ZIPFROMMEM_H
#define ZIPFROMMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "zip.h"
#include "unzip.h"

extern void fill_fopen_memfunc(zlib_filefunc_def *pzlib_filefunc_def, unsigned int size);
extern void free_fopen_memfunc(zlib_filefunc_def *pzlib_filefunc_def);

#ifdef __cplusplus
}
#endif

#endif
