/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/DirAsDisk.h,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:40 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, Tomas Karlsson
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
#ifndef DIR_AS_DISK_H
#define DIR_AS_DISK_H

#include "MsxTypes.h"

typedef enum {
    DDT_MSX                = 0,
    DDT_SVI_328_CPM_SSDD   = 1,
    DDT_SVI_328_CPM_DSDD   = 2,
    DDT_SVI_738_CPM_SSDD   = 3,
    DDT_SVI_328_Basic_SSDD = 4,
    DDT_SVI_328_Basic_DSDD = 5,
    DDT_MSX_CPM_SSDD       = 6,
    DDT_MSX_CPM_DSDD       = 7,
} DirDiskType;

void* dirLoadFile(DirDiskType diskType, const char* fileName, int* size);

#endif
