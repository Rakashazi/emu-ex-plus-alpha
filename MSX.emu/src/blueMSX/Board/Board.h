/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/Board.h,v $
**
** $Revision: 1.40 $
**
** $Date: 2007-03-20 02:30:31 $
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
#ifndef BOARD_H
#define BOARD_H
 
#include "MsxTypes.h"
#include "MediaDb.h"
#include "Machine.h"
#include "VDP.h"
#include "AudioMixer.h"
#include <stdio.h>

typedef struct {
    struct {
        int     inserted;
        RomType type;
        char    name[512];
        char    inZipName[512];
    } carts[2];
    struct {
        int  inserted;
        char name[512];
        char inZipName[512];
    } disks[64];
    struct {
        int  inserted;
        char name[512];
        char inZipName[512];
    } tapes[1];
    struct {
        VdpSyncMode vdpSyncMode;
    } video;
} BoardDeviceInfo;

typedef struct {
    int  cartridgeCount;
    int  diskdriveCount;
    int  casetteCount;

    void* cpuRef;

    void   (*destroy)();
    void   (*softReset)();
    void   (*loadState)();
    void   (*saveState)();
    int    (*getRefreshRate)();
    UInt8* (*getRamPage)(int);
    void   (*setDataBus)(void*, UInt8, UInt8, int);

    void   (*run)(void*);
    void   (*stop)(void*);
    void   (*setInt)(void*);
    void   (*clearInt)(void*);
    void   (*setCpuTimeout)(void*, UInt32);
    void   (*setBreakpoint)(void*, UInt16);
    void   (*clearBreakpoint)(void*, UInt16);

    void   (*changeCartridge)(void*, int, int);
} BoardInfo;

void boardInit(UInt32* systemTime);

int boardRun(Machine* machine, 
             BoardDeviceInfo* deviceInfo,
             Mixer* mixer,
             char* stateFile,
             int frequency,
             int reversePeriod,
             int reverseBufferCnt,
             int (*syncCallback)(int, int));

void boardRewind();
void boardEnableSnapshots(int enable);

BoardType boardGetType();

void boardSetMachine(Machine* machine);
void boardReset();

void boardSetDataBus(UInt8 value, UInt8 defaultValue, int setDefault);

UInt64 boardSystemTime64();

void boardCaptureStart(const char* filename);
void boardCaptureStop();
int boardCaptureHasData();
int boardCaptureIsRecording();
int boardCaptureIsPlaying();
int boardCaptureCompleteAmount();

UInt8 boardCaptureUInt8(UInt8 logId, UInt8 value);

void boardSaveState(const char* stateFile, int screenshot);

void boardSetFrequency(int frequency);
int  boardGetRefreshRate();

void boardSetBreakpoint(UInt16 address);
void boardClearBreakpoint(UInt16 address);

void   boardSetInt(UInt32 irq);
void   boardClearInt(UInt32 irq);
UInt32 boardGetInt(UInt32 irq);

UInt8* boardGetRamPage(int page);
UInt32 boardGetRamSize();
UInt32 boardGetVramSize();

int boardUseRom();
int boardUseMegaRom();
int boardUseMegaRam();
int boardUseFmPac();

RomType boardGetRomType(int cartNo);

typedef enum { HD_NONE, HD_SUNRISEIDE, HD_BEERIDE, HD_GIDE, HD_RSIDE,
               HD_MEGASCSI, HD_WAVESCSI, HD_GOUDASCSI, HD_NOWIND } HdType;
HdType boardGetHdType(int hdIndex);


const char* boardGetBaseDirectory();

Mixer* boardGetMixer();

int boardChangeCartridge(int cartNo, RomType romType, const char* cart, const char* cartZip);
void boardChangeDiskette(int driveId, char* fileName, const char* fileInZipFile);
void boardChangeCassette(int tapeId, char* name, const char* fileInZipFile);

int  boardGetCassetteInserted();

#define boardFrequency() (6 * 3579545)

static UInt32 boardSystemTime() {
    extern UInt32* boardSysTime;
    return *boardSysTime;
}

UInt64 boardSystemTime64();

typedef void (*BoardTimerCb)(void* ref, UInt32 time);

typedef struct BoardTimer BoardTimer;

BoardTimer* boardTimerCreate(BoardTimerCb callback, void* ref);
void boardTimerDestroy(BoardTimer* timer);
void boardTimerAdd(BoardTimer* timer, UInt32 timeout);
void boardTimerRemove(BoardTimer* timer);
void boardTimerCheckTimeout(void* dummy);
UInt32 boardCalcRelativeTimeout(UInt32 timerFrequency, UInt32 nextTimeout);

void   boardOnBreakpoint(UInt16 pc);

int boardInsertExternalDevices();
int boardRemoveExternalDevices();

// The following methods are more generic config than board specific
// They should be moved from board.
void boardSetDirectory(char* dir);

void boardSetFdcTimingEnable(int enable);
int  boardGetFdcTimingEnable();
void boardSetFdcActive();

void boardSetYm2413Oversampling(int value);
int  boardGetYm2413Oversampling();
void boardSetY8950Oversampling(int value);
int  boardGetY8950Oversampling();
void boardSetMoonsoundOversampling(int value);
int  boardGetMoonsoundOversampling();

void boardSetYm2413Enable(int value);
int  boardGetYm2413Enable();
void boardSetY8950Enable(int value);
int  boardGetY8950Enable();
void boardSetMoonsoundEnable(int value);
int  boardGetMoonsoundEnable();
void boardSetVideoAutodetect(int value);
int  boardGetVideoAutodetect();

void boardSetPeriodicCallback(BoardTimerCb cb, void* reference, UInt32 frequency);

void chdirToMachineBaseDir(char *prevWDir, size_t prevWDirSize);
void chdirToPrevWorkingDir(char *prevWDir);

#endif /* BOARD_H */

