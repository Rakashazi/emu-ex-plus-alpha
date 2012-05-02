/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNettouYakyuu.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-05-19 19:25:59 $
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
#include "romMapperNettouYakyuu.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "SamplePlayer.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    SamplePlayer* samplePlayer;
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    UInt32 romMask;
    int romMapper[4];
} RomMapperNettouYakyuu;

static void saveState(RomMapperNettouYakyuu* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperNettouYakyuu");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        saveStateSet(state, tag, rm->romMapper[i]);
    }

    saveStateClose(state);
}

static void loadState(RomMapperNettouYakyuu* rm)
{
    SaveState* state = saveStateOpenForRead("mapperNettouYakyuu");
    char tag[16];
    int i;

    for (i = 0; i < 4; i++) {
        sprintf(tag, "romMapper%d", i);
        rm->romMapper[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + (rm->romMapper[i]&rm->romMask) * 0x2000, (rm->romMapper[i]>>7^1)&1, 0);
    }
}


#ifdef NO_EMBEDDED_SAMPLES

/* like standard ASCII8 */

static void destroy(RomMapperNettouYakyuu* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperNettouYakyuu* rm, UInt16 address, UInt8 value) 
{
	int bank;
	
	address += 0x4000;
	
	if (address < 0x6000 || address > 0x7fff) return;
	
	bank = (address & 0x1800) >> 11;
	
	if (rm->romMapper[bank] != value) {
		UInt8* bankData = rm->romData + ((int)(value&rm->romMask) << 13);
		slotMapPage(rm->slot, rm->sslot, rm->startPage + bank, bankData, value>>7^1, 0);
	}
	rm->romMapper[bank] = value;
}

int romMapperNettouYakyuuCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperNettouYakyuu* rm;
    int i;

    int origSize = size;
    
    size = 0x8000;
    while (size < origSize) {
        size *= 2;
    }

    rm = malloc(sizeof(RomMapperNettouYakyuu));

    rm->deviceHandle = deviceManagerRegister(ROM_NETTOUYAKYUU, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, write, destroy, rm);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, origSize);
    rm->romMask = size / 0x2000 - 1;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper[0] = 0;
    rm->romMapper[1] = 0;
    rm->romMapper[2] = 0;
    rm->romMapper[3] = 0;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }

    return 1;
}

#else

#include "NettouYakyuuSamples.h"

static void destroy(RomMapperNettouYakyuu* rm)
{
    samplePlayerDestroy(rm->samplePlayer);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperNettouYakyuu* rm, UInt16 address, UInt8 value) 
{
	int index,idle;
	const UInt8* attack_sample=NULL;
	const UInt8* loop_sample=NULL;
	UInt32 attack_sample_size=0;
	UInt32 loop_sample_size=0;
	
	address += 0x4000;
	
	/* bankswitch like ASCII8 */
	if (address>=0x6000&&address<=0x7fff) {
		int bank = (address & 0x1800) >> 11;
		
		if (rm->romMapper[bank] != value) {
			UInt8* bankData = rm->romData + ((int)(value&rm->romMask) << 13);
			slotMapPage(rm->slot, rm->sslot, rm->startPage + bank, bankData, value>>7^1, 0);
		}
		rm->romMapper[bank] = value;
		return;
	}
	
	/* sample player */
	if (!(rm->romMapper[((address>>13)-2)&3]&0x80)) return; /* page not redirected to sample player */
	
	samplePlayerDoSync(rm->samplePlayer);
	index=samplePlayerGetIndex(rm->samplePlayer);
	idle=samplePlayerIsIdle(rm->samplePlayer);
	
	/* bit 7=0: reset */
	if (!(value&0x80)) {
		samplePlayerReset(rm->samplePlayer);
		samplePlayerSetIndex(rm->samplePlayer,0);
		return;
	}
	
	/* bit 6=1: no retrigger */
	if (value&0x40) {
		if (!idle) samplePlayerStopAfter(rm->samplePlayer,samplePlayerIsLooping(rm->samplePlayer)!=0);
		return;
	}
	
	switch (value&0xf) {
		#define S(c, x) case c: loop_sample=nettou_##x; loop_sample_size=sizeof(nettou_##x) / sizeof(nettou_##x[0]); break
		S(0x0,0); S(0x1,1); S(0x2,2); S(0x3,3); S(0x4,4); S(0x5,5); S(0x6,6); S(0x7,7);
		S(0x8,8); S(0x9,9); S(0xa,a); S(0xb,b); S(0xc,c); S(0xd,d); S(0xe,e); S(0xf,f);
		#undef S
		default: break;
	}
	
	if (!idle) {
		if (samplePlayerIsLooping(rm->samplePlayer)) {
			attack_sample=samplePlayerGetLoopBuffer(rm->samplePlayer);
			attack_sample_size=samplePlayerGetLoopBufferSize(rm->samplePlayer);
		}
		else {
			attack_sample=samplePlayerGetAttackBuffer(rm->samplePlayer);
			attack_sample_size=samplePlayerGetAttackBufferSize(rm->samplePlayer);
		}
	}
	
	samplePlayerWrite(rm->samplePlayer,attack_sample,attack_sample_size,loop_sample,loop_sample_size);
	samplePlayerSetIndex(rm->samplePlayer,index);
}

int romMapperNettouYakyuuCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, saveState, loadState };
    RomMapperNettouYakyuu* rm;
    int i;

    int origSize = size;
    
    size = 0x8000;
    while (size < origSize) {
        size *= 2;
    }

    rm = malloc(sizeof(RomMapperNettouYakyuu));
    rm->samplePlayer = samplePlayerCreate(boardGetMixer(), MIXER_CHANNEL_PCM, 8, 11025);
    rm->deviceHandle = deviceManagerRegister(ROM_NETTOUYAKYUU, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, write, destroy, rm);

    rm->romData = calloc(1, size);
    memcpy(rm->romData, romData, origSize);
    rm->romMask = size / 0x2000 - 1;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    rm->romMapper[0] = 0;
    rm->romMapper[1] = 0;
    rm->romMapper[2] = 0;
    rm->romMapper[3] = 0;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + rm->romMapper[i] * 0x2000, 1, 0);
    }

    return 1;
}

#endif
