/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperSonyHBIV1.c,v $
**
** $Revision: 1.13 $
**
** $Date: 2008-03-31 19:42:22 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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

#include "romMapperSonyHBIV1.h"
#include "MediaDb.h"
#include "Board.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "ArchVideoIn.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
	
    int command;
    int startBlockX;
    int startBlockY;
    int blockSizeX;
    int blockSizeY;
    int mode;
    UInt8 vramOffset;
    UInt8 vramLine;
    UInt8 status0;
    UInt8 delay;
    BoardTimer* timerDigitize;
    BoardTimer* timerBusy;
    UInt8 vram[256][256];
} RomMapperSonyHbiV1;

static void saveState(RomMapperSonyHbiV1* rm)
{
    SaveState* state = saveStateOpenForWrite("Vmx80");
    saveStateClose(state);
}

static void loadState(RomMapperSonyHbiV1* rm)
{
    SaveState* state = saveStateOpenForRead("Vmx80");
    saveStateClose(state);
}

static void destroy(RomMapperSonyHbiV1* rm)
{
    boardTimerDestroy(rm->timerDigitize);
    boardTimerDestroy(rm->timerBusy);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static int calcY(UInt16 c)
{
    int r = (c>>10)&0x1f;
    int g = (c>> 5)&0x1f;
    int b = (c>> 0)&0x1f;

    int y = b/2 + r / 4 + g / 8;

    if (y > 31) y = 31;
    return y;
}

static void calcJK(UInt16* c, int* J, int* K)
{
    int r = (((c[0]>>10)&0x1f) + ((c[1]>>10)&0x1f) + ((c[2]>>10)&0x1f) + ((c[3]>>10)&0x1f)) / 4;
    int g = (((c[0]>> 5)&0x1f) + ((c[1]>> 5)&0x1f) + ((c[2]>> 5)&0x1f) + ((c[3]>> 5)&0x1f)) / 4;
    int b = (((c[0]>> 0)&0x1f) + ((c[1]>> 0)&0x1f) + ((c[2]>> 0)&0x1f) + ((c[3]>> 0)&0x1f)) / 4;

    int y = b/2 + r / 4 + g / 8;
    int j = (r - y);
    int k = (g - y);

    if (j >  31) j = 31;
    if (j < -32) j = -32;
    if (j <   0) j = j + 64;
    if (k >  31) k = 31;
    if (k < -32) k = -32;
    if (k <   0) k = k + 64;

    *J = j;
    *K = k;
}

static void digitizeOne(RomMapperSonyHbiV1* rm, UInt16* img, 
                        int vramX, int vramY, int vramWidth, int vramHeight,
                        int scaleDestX, int scaleDestY, 
                        int scaleSrcX, int scaleSrcY)
{
    int x;
    int y;
    int mask = 0xfe;

    switch (rm->mode) {
    case 0:
        mask = 0xff;
    case 1:
        for (y = 0; y < vramHeight; y++) {
            int j = 0;
            int k = 0;
            for (x = 0; x < vramWidth; x++) {
                int imgOffset = 256 * (y * scaleDestY / scaleSrcY) +
                                      (x * scaleDestX / scaleSrcX);
                UInt16 color = img[imgOffset];
                UInt8  val = 0;
                switch (x & 3) {
                case 0:
                    calcJK(&img[imgOffset], &j, &k);
                    val = (k & 7) | ((calcY(color) & mask) << 3);
                    break;
                case 1:
                    val = (k >> 3) | ((calcY(color) & mask) << 3);
                    break;
                case 2:
                    val = (j & 7) | ((calcY(color) & mask) << 3);
                    break;
                case 3:
                    val = (j >> 3) | ((calcY(color) & mask) << 3);
                    break;
                }
                rm->vram[vramY + y][vramX + x] = val;
            }
        }
        break;
    case 2:
        for (y = 0; y < vramHeight; y++) {
            for (x = 0; x < vramWidth; x++) {
                int imgOffset = 256 * (y * scaleDestY / scaleSrcY) +
                                      (x * scaleDestX / scaleSrcX);
                UInt16 color = img[imgOffset];
                rm->vram[vramY + y][vramX + x] = 
                     (UInt8)(((color >> 10) & 0x1c) | 
                             ((color >> 2)  & 0xe0) | 
                             ((color >> 3)  & 0x03));
            }
        }
        break;
    case 3:
        for (y = 0; y < vramHeight; y++) {
            for (x = 0; x < vramWidth; x ++) {
                rm->vram[vramY + y][vramX + x] = 0;
            }
        }
        break;
    }
}


static int ScaleDest[8] = { 1, 2, 3, 4, 4, 2, 3, 4 };
static int ScaleSrc[8]  = { 1, 1, 1, 1, 3, 2, 2, 2 };

static int VramStartX[4][5] = {
    { 0, 256, 256, 256, 256 },
    { 0, 128, 256, 256, 256 },
    { 0,  88, 172, 256, 256 },
    { 0,  64, 128, 192, 256 }
};

static int VramStartY[4][5] = {
    { 0, 212, 212, 212, 212 },
    { 0, 106, 212, 212, 212 },
    { 0,  71, 142, 212, 212 },
    { 0,  53, 106, 159, 212 }
};

static void digitize(RomMapperSonyHbiV1* rm)
{
    // Get scaling factors for input and output image
    int scaleDestX = ScaleDest[rm->blockSizeX];
    int scaleDestY = ScaleDest[rm->blockSizeY];
    int scaleSrcX  = ScaleSrc[rm->blockSizeX];
    int scaleSrcY  = ScaleSrc[rm->blockSizeY];

    // Get destination start image
    int startX = rm->startBlockX < scaleDestX ? rm->startBlockX : scaleDestX - 1;
    int startY = rm->startBlockY < scaleDestY ? rm->startBlockY : scaleDestY - 1;

    // Get video in buffer
    UInt16* img = archVideoInBufferGet(256, 212);
    if (img == NULL) {
        return;
    }

    for (;;)
    {
        int vramX      = VramStartX[scaleDestX - 1][startX];
        int vramY      = VramStartY[scaleDestY - 1][startY];
        int vramWidth  = VramStartX[scaleDestX - 1][startX + 1] - vramX;
        int vramHeight = VramStartY[scaleDestY - 1][startY + 1] - vramY;

        digitizeOne(rm, img, vramX, vramY, vramWidth, vramHeight,
                    scaleDestX, scaleDestY, scaleSrcX, scaleSrcY);

        startX++;
        if (startX == scaleDestX) {
            startX = 0;
            startY++;
            if (startY == scaleDestY) {
                break;
            }
        }
    }
}

static void onTimerBusy(RomMapperSonyHbiV1* rm, UInt32 time)
{
    rm->status0 &= 0x7f;
}

static void onTimerDigitize(RomMapperSonyHbiV1* rm, UInt32 time)
{
    if (--rm->delay == 0) {
        rm->status0 |= 0x80;
        digitize(rm);
        boardTimerAdd(rm->timerBusy, boardSystemTime() + boardFrequency() / 60);
    }
    else {
        boardTimerAdd(rm->timerDigitize, boardSystemTime() + boardFrequency() / 60);
    }
}

static UInt8 read(RomMapperSonyHbiV1* rm, UInt16 address)
{
    UInt8 value = 0xff;

    if (address >= 0x8000) {
        return 0xff;
    }
    if (address >= 0x3e00 && address < 0x3f00) {
        UInt8 val = rm->vram[rm->vramLine][rm->vramOffset++];
        if (rm->vramOffset == 0) {
            rm->vramLine++;
            if (rm->vramLine == 212) {
                rm->vramLine = 0;
            }
        }
        return val;
    }
    if (address < 0x3ffc || address >= 0x3fff) {
        return rm->romData[address];
    }
    switch (address) {
    case 0x3ffc:
        rm->status0 ^= 060;
        value = rm->status0 | rm->command;
        break;
    case 0x3ffd:
        value = (UInt8)(((boardSystemTime() / (boardFrequency() / 60)) & 1) << 7) | 
                (archVideoInIsVideoConnected() ? 0 : 0x10) |
                (rm->startBlockY << 2) | rm->startBlockX;
        break;
    case 0x3ffe:
        value = (rm->mode << 6) | (rm->blockSizeY << 3) | rm->blockSizeX;
        break;
    case 0x3fff:
        value = rm->delay;
        break;
    }

    return value;
}

static void write(RomMapperSonyHbiV1* rm, UInt16 address, UInt8 value) 
{
    if (address >= 0x8000) {
        return;
    }
    if (address < 0x3ffc || address >= 0x4000) {
        return;
    }

    switch (address & 3) {
    case 0:
        rm->command = (value >> 0) & 3;
        rm->vramOffset = 0;
        rm->vramLine   = 0;
        switch (rm->command) {
        case 0:
            boardTimerRemove(rm->timerBusy);
            boardTimerRemove(rm->timerDigitize);
            rm->status0 &= 0x7f;
            break;
        case 1:
            digitize(rm);
            rm->status0 |= 0x80;
            boardTimerAdd(rm->timerBusy, boardSystemTime() + boardFrequency() / 60);
            break;
        case 2:
            if (rm->delay == 0) {
                rm->status0 |= 0x80;
                digitize(rm);
                boardTimerAdd(rm->timerBusy, boardSystemTime() + boardFrequency() / 60);
            }
            else
                boardTimerAdd(rm->timerDigitize, boardSystemTime() + boardFrequency() / 60);
            break;
        case 3:
            printf("HBI-V1 Command = 3\n");
        }
        break;
    case 1:
        rm->startBlockY = (value >> 2) & 3;
        rm->startBlockX = (value >> 0) & 3;
        break;
    case 2:
        rm->mode       = (value >> 6) & 3;
        rm->blockSizeY = (value >> 3) & 7;
        rm->blockSizeX = (value >> 0) & 7;
        break;
    case 3:
        rm->delay      = value;
        break;
    }
}

static void reset(RomMapperSonyHbiV1* rm)
{
    rm->command     = 0;
    rm->startBlockX = 0;
    rm->startBlockY = 0;
    rm->blockSizeX  = 0;
    rm->blockSizeY  = 0;
    rm->mode        = 0;
    rm->status0   = 0;
    rm->vramLine    = 0;
    rm->vramOffset  = 0;
    rm->delay       = 0;
}

int romMapperSonyHbiV1Create(const char* filename, UInt8* romData, int size,
                             int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperSonyHbiV1* rm;
    int pages = 4;
    int i;

    if ((startPage + pages) > 8) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperSonyHbiV1));

    rm->deviceHandle = deviceManagerRegister(ROM_SONYHBIV1, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, read, write, destroy, rm);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, size);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;
    
    rm->timerDigitize = boardTimerCreate(onTimerDigitize, rm);
    rm->timerBusy     = boardTimerCreate(onTimerBusy,     rm);

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

    reset(rm);

    return 1;
}

