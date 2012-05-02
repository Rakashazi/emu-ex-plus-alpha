/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Z80/R800SaveState.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:48 $
**
** Author: Daniel Vik
**
** Description: Save/load state of an R800 object
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
#include "R800SaveState.h"
#include "SaveState.h"
#include <stdio.h>


#define r800LoadRegisterState(state, regs, index) {             \
    regs.AF.W  = (UInt16)saveStateGet(state, "AF"   #index, 0); \
    regs.BC.W  = (UInt16)saveStateGet(state, "BC"   #index, 0); \
    regs.DE.W  = (UInt16)saveStateGet(state, "DE"   #index, 0); \
    regs.HL.W  = (UInt16)saveStateGet(state, "HL"   #index, 0); \
    regs.IX.W  = (UInt16)saveStateGet(state, "IX"   #index, 0); \
    regs.IY.W  = (UInt16)saveStateGet(state, "IY"   #index, 0); \
    regs.PC.W  = (UInt16)saveStateGet(state, "PC"   #index, 0); \
    regs.SP.W  = (UInt16)saveStateGet(state, "SP"   #index, 0); \
    regs.AF1.W = (UInt16)saveStateGet(state, "AF1"  #index, 0); \
    regs.BC1.W = (UInt16)saveStateGet(state, "BC1"  #index, 0); \
    regs.DE1.W = (UInt16)saveStateGet(state, "DE1"  #index, 0); \
    regs.HL1.W = (UInt16)saveStateGet(state, "HL1"  #index, 0); \
    regs.SH.W  = (UInt16)saveStateGet(state, "SH"   #index, 0); \
    regs.I     = (UInt8) saveStateGet(state, "I"    #index, 0); \
    regs.R     = (UInt8) saveStateGet(state, "R"    #index, 0); \
    regs.R2    = (UInt8) saveStateGet(state, "R2"   #index, 0); \
    regs.iff1  = (UInt8) saveStateGet(state, "iff1" #index, 0); \
    regs.iff2  = (UInt8) saveStateGet(state, "iff2" #index, 0); \
    regs.im    = (UInt8) saveStateGet(state, "im"   #index, 0); \
    regs.halt  = (UInt8) saveStateGet(state, "halt" #index, 0); \
    regs.ei_mode = (UInt8) saveStateGet(state, "ei_mode" #index, 0); \
}

#define r800SaveRegisterState(state, regs, index) {             \
    saveStateSet(state, "AF"   #index, regs.AF.W);              \
    saveStateSet(state, "BC"   #index, regs.BC.W);              \
    saveStateSet(state, "DE"   #index, regs.DE.W);              \
    saveStateSet(state, "HL"   #index, regs.HL.W);              \
    saveStateSet(state, "IX"   #index, regs.IX.W);              \
    saveStateSet(state, "IY"   #index, regs.IY.W);              \
    saveStateSet(state, "PC"   #index, regs.PC.W);              \
    saveStateSet(state, "SP"   #index, regs.SP.W);              \
    saveStateSet(state, "AF1"  #index, regs.AF1.W);             \
    saveStateSet(state, "BC1"  #index, regs.BC1.W);             \
    saveStateSet(state, "DE1"  #index, regs.DE1.W);             \
    saveStateSet(state, "HL1"  #index, regs.HL1.W);             \
    saveStateSet(state, "SH"   #index, regs.SH.W);              \
    saveStateSet(state, "I"    #index, regs.I);                 \
    saveStateSet(state, "R"    #index, regs.R);                 \
    saveStateSet(state, "R2"   #index, regs.R2);                \
    saveStateSet(state, "iff1" #index, regs.iff1);              \
    saveStateSet(state, "iff2" #index, regs.iff2);              \
    saveStateSet(state, "im"   #index, regs.im);                \
    saveStateSet(state, "halt" #index, regs.halt);              \
    saveStateSet(state, "ei_mode" #index, regs.ei_mode);        \
}

void r800LoadState(R800* r800)
{
    SaveState* state = saveStateOpenForRead("r800");
    char tag[32];
    int i;

    r800->systemTime =         saveStateGet(state, "systemTime", 0);
    r800->vdpTime    =         saveStateGet(state, "vdpTime",    0);
    r800->cachePage  = (UInt16)saveStateGet(state, "cachePage",  0);
    r800->dataBus    = (UInt8) saveStateGet(state, "dataBus",    0);
    r800->intState   =         saveStateGet(state, "intState",   0);
    r800->nmiState   =         saveStateGet(state, "nmiState",   0);
    r800->nmiEdge    =         saveStateGet(state, "nmiEdge",    0);
    r800->cpuMode    =         saveStateGet(state, "cpuMode",    0);
    r800->oldCpuMode =         saveStateGet(state, "oldCpuMode", 0);
    
    for (i = 0; i < sizeof(r800->delay) / sizeof(r800->delay[0]); i++) {
        sprintf(tag, "delay%d", i);
        r800->delay[i] = saveStateGet(state, tag, 0);
    }

    r800LoadRegisterState(state, r800->regs,        00);
    r800LoadRegisterState(state, r800->regBanks[0], 01);
    r800LoadRegisterState(state, r800->regBanks[1], 02);

    saveStateClose(state);
}

void r800SaveState(R800* r800)
{
    SaveState* state = saveStateOpenForWrite("r800");
    char tag[32];
    int i;

    saveStateSet(state, "systemTime", r800->systemTime);
    saveStateSet(state, "vdpTime",    r800->vdpTime);
    saveStateSet(state, "cachePage",  r800->cachePage);
    saveStateSet(state, "dataBus",    r800->dataBus);
    saveStateSet(state, "intState",   r800->intState);
    saveStateSet(state, "nmiState",   r800->nmiState);
    saveStateSet(state, "nmiEdge",    r800->nmiEdge);
    saveStateSet(state, "cpuMode",    r800->cpuMode);
    saveStateSet(state, "oldCpuMode", r800->oldCpuMode);

    for (i = 0; i < sizeof(r800->delay) / sizeof(r800->delay[0]); i++) {
        sprintf(tag, "delay%d", i);
        saveStateSet(state, tag, r800->delay[i]);
    }

    r800SaveRegisterState(state, r800->regs,        00);
    r800SaveRegisterState(state, r800->regBanks[0], 01);
    r800SaveRegisterState(state, r800->regBanks[1], 02);

    saveStateClose(state);
}
