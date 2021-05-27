/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/CRTC6845.c,v $
**
** $Revision: 1.46 $
**
** $Date: 2008-03-31 19:42:23 $
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

#include "CRTC6845.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Language.h"
#include "FrameBuffer.h"
#include <stdlib.h>
#include <string.h>

/*
   AR Address Register
   R0 Horizontal Total (Character)                       SVI Default: 107
   R1 Horizontal Displayed (Character)                   SVI Default: 80
   R2 Horizontal Sync Position (Character)               SVI Default: 88
   R3 Sync Width (Vertical-Raster, Horizontal-Character) SVI Default:  8
   R4 Vertical Total (Line)                              SVI Default: 38
   R5 Vertical Total Adjust (Raster)                     SVI Default: 5
   R6 Vertical Displayed (Line)                          SVI Default: 24
   R7 Vertical Sync Position (Line)                      SVI Default: 30
   R8 Interlace and Skew                                 SVI Default: 0
   R9 Maximum Raster Address (Raster)                    SVI Default: 7
   R10 Cursor Start Raster (Raster)
   R11 Cursor End Raster (Raster)
   R12 Start Address (H)
   R13 Start Address (L)
   R14 Cursor (H)
   R15 Cursor (L)
   R16 Light Pen (H)
   R17 Light Pen (L)
*/

#define DISPLAY_HEIGHT      240

typedef enum { CURSOR_DISABLED, CURSOR_BLINK, CURSOR_NOBLINK} crtcCursorModes;
typedef enum { CRTC_R0=0,   CRTC_R1=1,   CRTC_R2=2,   CRTC_R3=3,   CRTC_R4=4,   CRTC_R5=5,
               CRTC_R6=6,   CRTC_R7=7,   CRTC_R8=8,   CRTC_R9=9,   CRTC_R10=10, CRTC_R11=11,
               CRTC_R12=12, CRTC_R13=13, CRTC_R14=14, CRTC_R15=15, CRTC_R16=16, CRTC_R17=17 } crtcRegisters;

static const UInt8 crtcRegisterValueMask[18] = {
    0xff, 0xff, 0xff, 0xff, 0x7f, 0x1f,
    0x7f, 0x7f, 0xf3, 0x1f, 0x7f, 0x1f,
    0x3f, 0xff, 0x3f, 0xff, 0x3f, 0xff 
};

static void saveState(CRTC6845* crtc);
static void loadState(CRTC6845* crtc);

// Frame refresh timer info
#define REFRESH_PERIOD (boardFrequency() /  50)

static void crtcRenderVideoBuffer(CRTC6845* crtc)
{
    Pixel color[2];
    int x, y;
    int charWidth, charHeight;
    int Nr  = crtc->registers.reg[CRTC_R9] + 1; // Number of rasters per character
    FrameBuffer* crtcFrameBuffer = frameBufferFlipDrawFrame(); // Call once per frame

    crtc->frameCounter++;

    charWidth = crtc->registers.reg[CRTC_R1];
    if (charWidth >= crtc->registers.reg[CRTC_R0])
        charWidth = crtc->registers.reg[CRTC_R0] - 1;

    charHeight = crtc->registers.reg[CRTC_R6];
    if (charHeight >= crtc->registers.reg[CRTC_R4])
        charHeight = crtc->registers.reg[CRTC_R4] - 1;

    color[0] = videoGetColor(0, 0, 0);       // Cen be calculated once. The interface
    color[1] = videoGetColor(255, 255, 255); // must be updated (also for the VDP)

    for (y = 0; y < DISPLAY_HEIGHT; y++) {
        Pixel* linePtr = frameBufferGetLine(crtcFrameBuffer, y);
        int charRaster = y % Nr;
        int vadjust = 2; // Fix vertical adjust from regs (the value 4)
        int hadjust = 2; // Fix horizontal adjust from regs (the value 1)
        int charLine   = y / Nr - vadjust;                
        int charAddress = charLine * charWidth - hadjust; 

        if (charLine < 0 || charLine >= charHeight) {
            for (x = 0; x < crtc->displayWidth; x++) {
                linePtr[x] = color[0];
            }
            continue;
        }

        for (x = 0; x < crtc->charsPerLine; x++) {
            UInt8 pattern = 0;
            int   i;

            if (x >= hadjust && x < charWidth + hadjust) {
                pattern = crtc->romData[(16*crtc->vram[charAddress & crtc->vramMask]+charRaster) & crtc->romMask];

                if (charAddress == crtc->cursor.addressStart) {
                    if (((crtc->frameCounter - crtc->cursor.blinkstart) & crtc->cursor.blinkrate) || (crtc->cursor.mode==CURSOR_NOBLINK)) {
                        pattern ^= charRaster >= crtc->cursor.rasterStart && charRaster <= crtc->cursor.rasterEnd ? 0xff : 0;
                    }
                }
            }
            switch (crtc->charWidth) {
            default: linePtr[7] = color[(pattern >> 0) & 1];
            case 7:  linePtr[6] = color[(pattern >> 1) & 1];
            case 6:  linePtr[5] = color[(pattern >> 2) & 1];
            case 5:  linePtr[4] = color[(pattern >> 3) & 1];
            case 4:  linePtr[3] = color[(pattern >> 4) & 1];
            case 3:  linePtr[2] = color[(pattern >> 5) & 1];
            case 2:  linePtr[1] = color[(pattern >> 6) & 1];
            case 1:  linePtr[0] = color[(pattern >> 7) & 1];
            }
            linePtr += crtc->charWidth;

            for (i = 0; i < crtc->charSpace; i++) {
                *linePtr++ = color[0];
            }

            charAddress++;
        }
    }
}

static void crtcCursorUpdate(CRTC6845* crtc)
{
    switch (crtc->registers.reg[CRTC_R10] & 0x60 ) {
    case 32:
        crtc->cursor.mode = CURSOR_DISABLED;
        crtc->cursor.blinkrate = 0;
        break;
    case 64:
        crtc->cursor.mode = CURSOR_BLINK;
        crtc->cursor.blinkrate = 16;
        break;
    case 96:
        crtc->cursor.mode = CURSOR_BLINK;
        crtc->cursor.blinkrate = 32;
        break;
    default:
        crtc->cursor.mode = CURSOR_NOBLINK;
        crtc->cursor.blinkrate = 0;
    }
    crtc->cursor.blinkstart = crtc->frameCounter - crtc->cursor.blinkrate;
    crtc->cursor.rasterStart = crtc->registers.reg[CRTC_R10] & 0x1f;
}

// Timer callback that is called once every frame
static void crtcOnDisplay(CRTC6845* crtc, UInt32 time)
{
    // Render VRAM if video is connected
    if (crtc->videoEnabled)
        crtcRenderVideoBuffer(crtc);

    crtc->timeDisplay = time + REFRESH_PERIOD;
    boardTimerAdd(crtc->timerDisplay, crtc->timeDisplay);
}

// Callback called when video connector is enabled
static void crtcVideoEnable(CRTC6845* crtc)
{
    crtc->videoEnabled = 1;
}

// Callback called when video connector is disabled
static void crtcVideoDisable(CRTC6845* crtc)
{
    crtc->videoEnabled = 0;
}

UInt8 crtcRead(CRTC6845* crtc)
{
    if (crtc->registers.address < 18)
        return crtc->registers.reg[crtc->registers.address];

    return 0xff;
}

static void crtcUpdateRegister(CRTC6845* crtc, UInt8 address, UInt8 value)
{
    if (address < 16) {
        value &= crtcRegisterValueMask[address];
        crtc->registers.reg[address] = value;
        switch (address) {
        case CRTC_R10:
            crtcCursorUpdate(crtc);
            break;
        case CRTC_R11:
            crtc->cursor.rasterEnd = crtc->registers.reg[CRTC_R11];
            break;
        case CRTC_R14:
        case CRTC_R15:
            crtc->cursor.addressStart = (crtc->registers.reg[CRTC_R14] << 8) | crtc->registers.reg[CRTC_R15];
            crtc->cursor.blinkstart = crtc->frameCounter - crtc->cursor.blinkrate;
            break;
        }
    }
}

void crtcWrite(CRTC6845* crtc, UInt8 value)
{
    crtcUpdateRegister(crtc, crtc->registers.address, value);
}

void crtcWriteLatch(CRTC6845* crtc, UInt8 value)
{
    crtc->registers.address = value & 0x1f;
}

void crtcMemWrite(CRTC6845* crtc, UInt16 address, UInt8 value)
{
    crtc->vram[address & crtc->vramMask] = value;
    if (!crtc->videoEnabled && boardGetVideoAutodetect() && videoManagerGetCount() > 1) {
        videoManagerSetActive(crtc->videoHandle);
    }
}

UInt8 crtcMemRead(CRTC6845* crtc, UInt16 address)
{
    return crtc->vram[address & crtc->vramMask];
}

static void crtc6845Reset(CRTC6845* crtc)
{
    memset(&crtc->registers, 0, sizeof(crtc->registers));
    memset(&crtc->cursor, 0, sizeof(crtc->cursor));
    memset(crtc->vram, 0xff, crtc->vramMask + 1);
    crtc->frameCounter = 0;
}

static void crtc6845Destroy(CRTC6845* crtc)
{
    debugDeviceUnregister(crtc->debugHandle);
    deviceManagerUnregister(crtc->deviceHandle);
    videoManagerUnregister(crtc->videoHandle);
    boardTimerDestroy(crtc->timerDisplay);

    frameBufferDataDestroy(crtc->frameBufferData);

    free(crtc->vram);
    free(crtc->romData);

    free(crtc);
}

static void getDebugInfo(CRTC6845* crtc, DbgDevice* dbgDevice)
{
    DbgRegisterBank* regBank;
    int i;

    dbgDeviceAddMemoryBlock(dbgDevice, langDbgMemVram(), 0, 0, crtc->vramMask + 1, crtc->vram);
   
    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegs(), 16);

    for (i = 0; i < 16; i++) {
        char reg[4];
        sprintf(reg, "R%d", i);
        dbgRegisterBankAddRegister(regBank, i, reg,  8, crtc->registers.reg[i]);
    }
}

static int dbgWriteMemory(CRTC6845* crtc, char* name, void* data, int start, int size)
{
    if (strcmp(name, "VRAM") || (UInt32)start + size > crtc->vramMask + 1) {
        return 0;
    }

    memcpy(crtc->vram + start, data, size);

    return 1;
}

static int dbgWriteRegister(CRTC6845* crtc, char* name, int regIndex, UInt32 value)
{
    crtcUpdateRegister(crtc, (UInt8)regIndex, (UInt8)value);

    return 1;
}

CRTC6845* crtc6845Create(int frameRate, UInt8* romData, int size, int vramSize, 
                         int charWidth, int charSpace, int charsPerLine, int borderChars)
{
    CRTC6845* crtc;
    int logSize;
    int pixelZoom = 1;

    crtc = calloc(1, sizeof(CRTC6845));
    
    crtc->vram = malloc(vramSize);
    crtc->vramMask = vramSize - 1;

    for (logSize = 1; logSize < size; logSize <<= 1);

    crtc->romData = malloc(logSize);
    memset(crtc->romData, 0xff, logSize);
    crtc->romMask = logSize - 1;
    memcpy(crtc->romData, romData, size);

    crtc6845Reset(crtc);

    crtc->frameRate = frameRate;

    crtc->charWidth     = charWidth;
    crtc->charSpace     = charSpace;
    crtc->charsPerLine  = charsPerLine;
    crtc->displayWidth  = ((charWidth + charSpace) * (charsPerLine + borderChars)) & ~7;

    // The displayWidth calculation is necessary for the frame buffer rendering
    // since the frame buffer allows at most 320 pixel wide screens, although
    // each pixel can have two real pixels (the zoom argument)
    if (crtc->displayWidth > 320) {
        pixelZoom = 2;
        crtc->displayWidth /= 2;
    }
    if (crtc->displayWidth > 320) {
        crtc->displayWidth = 320;
    }

    // Create and start frame refresh timer
    crtc->timerDisplay = boardTimerCreate(crtcOnDisplay, crtc);
    crtc->timeDisplay = boardSystemTime() + REFRESH_PERIOD;
    boardTimerAdd(crtc->timerDisplay, crtc->timeDisplay);

    // Initialize device
    {
        DeviceCallbacks callbacks = { crtc6845Destroy, crtc6845Reset, saveState, loadState };
        DebugCallbacks dbgCallbacks = { getDebugInfo, dbgWriteMemory, dbgWriteRegister, NULL };
        crtc->deviceHandle = deviceManagerRegister(ROM_SVI80COL, &callbacks, crtc);
        crtc->debugHandle = debugDeviceRegister(DBGTYPE_VIDEO, langDbgDevCrtc6845(), &dbgCallbacks, crtc);
    }

    // Initialize video frame buffer
    {
        VideoCallbacks videoCallbacks = { crtcVideoEnable, crtcVideoDisable };
        crtc->frameBufferData = frameBufferDataCreate(crtc->displayWidth, DISPLAY_HEIGHT, pixelZoom);
        crtc->videoHandle = videoManagerRegister("CRTC6845", crtc->frameBufferData, &videoCallbacks, crtc);
    }

    return crtc;
}

static void saveState(CRTC6845* crtc)
{
    int index;

    SaveState* state = saveStateOpenForWrite("crtc6845");

    saveStateSet(state, "crtc->cursor.mode",         crtc->cursor.mode);
    saveStateSet(state, "crtc->cursor.rasterStart",  crtc->cursor.rasterStart);
    saveStateSet(state, "crtc->cursor.rasterEnd",    crtc->cursor.rasterEnd);
    saveStateSet(state, "crtc->cursor.addressStart", crtc->cursor.addressStart);
    saveStateSet(state, "crtc->cursor.blinkrate",    crtc->cursor.blinkrate);
    saveStateSet(state, "crtc->cursor.blinkstart",   crtc->cursor.blinkstart);

    for (index = 0; index < 18; index++) {
        char tag[32];
        sprintf(tag, "crtc->registers.reg[%d]", index);
        saveStateSet(state, tag, crtc->registers.reg[index]);
    }

    saveStateSet(state, "crtc->frameCounter",    crtc->frameCounter);
    saveStateSet(state, "crtc->frameRate",       crtc->frameRate);
    saveStateSet(state, "crtc->timeDisplay",     crtc->timeDisplay);
    saveStateSet(state, "crtc->vramMask",        crtc->vramMask);
    saveStateSet(state, "crtc->romMask",         crtc->romMask);
    saveStateSet(state, "crtc->charWidth",       crtc->charWidth);
    saveStateSet(state, "crtc->charSpace",       crtc->charSpace);
    saveStateSet(state, "crtc->charsPerLine",    crtc->charsPerLine);
    saveStateSet(state, "crtc->displayWidth",    crtc->displayWidth);

    saveStateSetBuffer(state, "crtc->vram", crtc->vram, crtc->vramMask + 1);

    saveStateClose(state);
}

static void loadState(CRTC6845* crtc)
{
    int index;

    SaveState* state = saveStateOpenForRead("crtc6845");

    crtc->cursor.mode         = saveStateGet(state, "crtc->cursor.mode",         0);
    crtc->cursor.rasterStart  = (UInt8)saveStateGet(state, "crtc->cursor.rasterStart",  0);
    crtc->cursor.rasterEnd    = (UInt8)saveStateGet(state, "crtc->cursor.rasterEnd",    0);
    crtc->cursor.addressStart = (UInt16)saveStateGet(state, "crtc->cursor.addressStart", 0);
    crtc->cursor.blinkrate    = saveStateGet(state, "crtc->cursor.blinkrate",    0);
    crtc->cursor.blinkstart   = saveStateGet(state, "crtc->cursor.blinkstart",   0);
    
    for (index = 0; index < 18; index++) {
        char tag[32];
    	sprintf(tag, "crtc->registers.reg[%d]", index);
        crtc->registers.reg[index] = (UInt8)saveStateGet(state, tag, 0);
    }

    crtc->frameCounter    = saveStateGet(state, "crtc->frameCounter",   0);
    crtc->frameRate       = saveStateGet(state, "crtc->frameRate",      0);
    crtc->timeDisplay     = saveStateGet(state, "crtc->timeDisplay",    boardSystemTime() + 100);
    crtc->vramMask        = saveStateGet(state, "crtc->vramMask",       0);
    crtc->romMask         = saveStateGet(state, "crtc->romMask",        0);
    crtc->charWidth       = saveStateGet(state, "crtc->charWidth",      0);
    crtc->charSpace       = saveStateGet(state, "crtc->charSpace",      0);
    crtc->charsPerLine    = saveStateGet(state, "crtc->charsPerLine",   0);
    crtc->displayWidth    = saveStateGet(state, "crtc->displayWidth",   0);

    saveStateGetBuffer(state, "crtc->vram", crtc->vram, crtc->vramMask + 1);

    saveStateClose(state);

    // Start frame refresh timer
    boardTimerAdd(crtc->timerDisplay, crtc->timeDisplay);
}
