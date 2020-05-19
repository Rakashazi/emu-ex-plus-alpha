/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/VDP.h,v $
**
** $Revision: 1.16 $
**
** $Date: 2008-06-25 22:26:17 $
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
#ifndef VDP_H
#define VDP_H

#include "MsxTypes.h"
#include "VideoManager.h"

typedef enum { VDP_V9938, VDP_V9958, VDP_TMS9929A, VDP_TMS99x8A } VdpVersion;
typedef enum { VDP_SYNC_AUTO, VDP_SYNC_50HZ, VDP_SYNC_60HZ } VdpSyncMode; 
typedef enum { VDP_MSX, VDP_SVI, VDP_COLECO, VDP_SG1000 } VdpConnector;

static const char* VdpNames[] = { "V9938", "V9958", "TMS9929A", "TMS99x8A" };

void vdpCreate(VdpConnector connector, VdpVersion version, VdpSyncMode sync, int vramPages);

int  vdpGetRefreshRate();

void vdpSetSpritesEnable(int enable);
int  vdpGetSpritesEnable();
void vdpSetDisplayEnable(int enable);
int  vdpGetDisplayEnable();

void vdpForceSync();

// Video DA Interface

#define VDP_VIDEODA_WIDTH  544
#define VDP_VIDEODA_HEIGHT 240

typedef struct {
    void (*daStart)(void*, int);
    void (*daEnd)(void*);
    UInt8 (*daRead)(void*, int, int, int, Pixel*, int);
} VdpDaCallbacks;

int vdpRegisterDaConverter(VdpDaCallbacks* callbacks, void* ref, VideoMode videoModeMask);
void vdpUnregisterDaConverter(int vdpDaHandle);

/* The following methods needs target dependent implementation */
extern void RefreshScreen(int);

#endif

