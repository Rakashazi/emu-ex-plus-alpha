/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Board/Machine.h,v $
**
** $Revision: 1.18 $
**
** $Date: 2008-03-30 18:38:39 $
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
#ifndef MACHINE_H
#define MACHINE_H
 
#include "MsxTypes.h"
#include "MediaDb.h"
#include "VDP.h"
#include "AY8910.h"
#include <stdio.h>


typedef enum { 
    BOARD_UNKNOWN       = -1, 
    BOARD_MSX           = 0x0100 + 0x00, 
    BOARD_MSX_S3527     = 0x0100 + 0x01,
    BOARD_MSX_S1985     = 0x0100 + 0x02,
    BOARD_MSX_T9769B    = 0x0100 + 0x03,
    BOARD_MSX_T9769C    = 0x0100 + 0x04,
    BOARD_SVI           = 0x0200 + 0x00,
    BOARD_COLECO        = 0x0300 + 0x00,
    BOARD_COLECOADAM    = 0x0300 + 0x01,
    BOARD_SG1000        = 0x0400 + 0x00,
    BOARD_SF7000        = 0x0400 + 0x01,
    BOARD_SC3000        = 0x0400 + 0x02,
    BOARD_MSX_FORTE_II  = 0x0500 + 0x00,
    BOARD_MASK          = 0xff00
} BoardType;

typedef struct {
    RomType romType;
    char name[512];
    char inZipName[128];
    int slot;
    int subslot;
    int startPage;
    int pageCount;
    int error;
} SlotInfo;

typedef struct {
    char name[64];
    struct {
        BoardType type;
    } board;
    struct {
        int subslotted;
    } slot[4];
    struct {
        int slot;
        int subslot;
    } cart[2];
    struct {
        VdpVersion vdpVersion;
        int vramSize;
    } video;
    struct {
        int psgstereo;
        int psgpan[3];
    } audio;
    struct {
        int enable;
        int batteryBacked;
    } cmos;
    struct {
        int    hasR800;
        UInt32 freqZ80;
        UInt32 freqR800;
    } cpu;
    struct {
        int enabled;
        int count;
    } fdc;
    int slotInfoCount;
    SlotInfo slotInfo[32];
} Machine;


Machine* machineCreate(const char* machineName);
void machineDestroy(Machine* machine);

char** machineGetAvailable(int checkRoms);

int machineIsValid(const char* machineName, int checkRoms);

void machineUpdate(Machine* machine);

void machineSave(Machine* machine);

int machineInitialize(Machine* machine, UInt8** mainRam, UInt32* mainRamSize, UInt32* mainRamStart);

void machineLoadState(Machine* machine);
void machineSaveState(Machine* machine);

#endif

