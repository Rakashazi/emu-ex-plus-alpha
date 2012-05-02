/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchCdrom.h,v $
**
** $Revision: 1.2 $
**
** $Date: 2008-03-30 18:38:38 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
#ifndef ARCH_CDROM_H
#define ARCH_CDROM_H

#include "MsxTypes.h"
#include <stddef.h>

typedef struct ArchCdrom ArchCdrom;
typedef void (*CdromXferCompCb)(void*, int);

ArchCdrom* archCdromCreate(CdromXferCompCb xferCompCb, void* ref);
void archCdromDestroy(ArchCdrom* cdrom);
void archCdromHwReset(ArchCdrom* cdrom);
void archCdromBusReset(ArchCdrom* cdrom);
void archCdromDisconnect(ArchCdrom* cdrom);

int archCdromExecCmd(ArchCdrom* cdrom, const UInt8* cdb, UInt8* buffer, int bufferSize);
int archCdromIsXferComplete(ArchCdrom* cdrom, int* transferLength);
UInt8 archCdromGetStatusCode(ArchCdrom* cdrom);
int archCdromGetSenseKeyCode(ArchCdrom* cdrom);

void archCdromLoadState(ArchCdrom* cdrom);
void archCdromSaveState(ArchCdrom* cdrom);

void* archCdromBufferMalloc(size_t size);
void archCdromBufferFree(void* ptr);

#endif
