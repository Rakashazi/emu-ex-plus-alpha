/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNms8280VideoDa.c,v $
**
** $Revision: 1.4 $
**
** $Date: 2008-03-30 18:38:44 $
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
#include "romMapperNms8280VideoDa.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include "VideoManager.h"
#include "ArchVideoIn.h"
#include "VDP.h"

#include <stdlib.h>

typedef struct {
    int deviceHandle;
    int vdpDaHandle;
    UInt16* videoBuffer;
} RomMapperNms8280VideoDa;

static void daStart(RomMapperNms8280VideoDa* rm, int oddPage)
{
    rm->videoBuffer = 16 + archVideoInBufferGet(VDP_VIDEODA_WIDTH, VDP_VIDEODA_HEIGHT * 2);
    if (oddPage) {
        rm->videoBuffer += VDP_VIDEODA_WIDTH;
    }
}

static UInt8 daRead(RomMapperNms8280VideoDa* rm, int screenMode, int x, int y, Pixel* palette, int paletteCount)
{
    UInt16 color;
    int bestDiff;
    UInt8 match;
    int i;
    
    color = rm->videoBuffer[x + y * VDP_VIDEODA_WIDTH * 2];

    // If palette is NULL we do 8 bit RGB conversion
    if (palette == NULL) {
        return ((color >> 10) & 0x1c) | ((color >> 2) & 0xe0) | ((color >> 3) & 0x03);
    }

    bestDiff = 0x1000000;
    match = 0;
    
    for (i = 0; i < paletteCount; i++) {
        int dR = ((palette[i] >> COLSHIFT_R) & COLMASK_R) - ((color >> COLSHIFT_R) & COLMASK_R);
        int dG = ((palette[i] >> COLSHIFT_G) & COLMASK_G) - ((color >> COLSHIFT_G) & COLMASK_G);
        int dB = ((palette[i] >> COLSHIFT_B) & COLMASK_B) - ((color >> COLSHIFT_B) & COLMASK_B);
        int test = dR * dR + dG * dG + dB * dB;
        if (test < bestDiff) {
            bestDiff = test;
            match = (UInt8)i;
        }
    }
    return match;
}

static void destroy(RomMapperNms8280VideoDa* rm)
{
    deviceManagerUnregister(rm->deviceHandle);
    vdpUnregisterDaConverter(rm->vdpDaHandle);

    free(rm);
}

int romMapperNms8280VideoDaCreate() 
{
    DeviceCallbacks callbacks   = { destroy, NULL, NULL, NULL };
    VdpDaCallbacks  daCallbacks = { daStart, NULL, daRead };
    RomMapperNms8280VideoDa* rm = malloc(sizeof(RomMapperNms8280VideoDa));

    rm->deviceHandle = deviceManagerRegister(ROM_NMS8280DIGI, &callbacks, rm);
    rm->vdpDaHandle    = vdpRegisterDaConverter(&daCallbacks, rm, VIDEO_MASK_ALL);

    return 1;
}
