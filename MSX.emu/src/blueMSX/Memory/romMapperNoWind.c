/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNoWind.c,v $
**
** $Revision: 1.16 $
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
#include "romMapperNoWind.h"
#include "AmdFlash.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "Board.h"
#include "SaveState.h"
#include "sramLoader.h"

#include <stdlib.h>
#include <string.h>

#ifdef USE_NOWIND_DLL
#include "Disk.h"
#include "Properties.h"
#include <windows.h>

typedef void (*Nowind_DebugCb)(const char*);

typedef void (__cdecl *NoWind_Init)(void);
typedef void (__cdecl *NoWind_StartUp)(void);
typedef void (__cdecl *NoWind_Cleanup)(void);
typedef void (__cdecl *NoWind_SetImage)(int, const char*);
typedef void (__cdecl *NoWind_Write)(unsigned char);
typedef unsigned char (__cdecl *NoWind_Read)(void);
typedef void (__cdecl *NoWind_SetDebugCb)(Nowind_DebugCb);

static NoWind_Init       nowindusb_init       = NULL;
static NoWind_StartUp    nowindusb_startup    = NULL;
static NoWind_Cleanup    nowindusb_cleanup    = NULL;
static NoWind_SetImage   nowindusb_set_image  = NULL;
static NoWind_Write      nowindusb_write      = NULL;
static NoWind_Read       nowindusb_read       = NULL;
static NoWind_SetDebugCb nowindusb_set_debug_callback = NULL;
static HINSTANCE hLib = NULL;

void nowindLoadDll()
{
	// Load DLL.
	hLib = LoadLibrary("libnowindusbms.dll");

	if (!hLib)	{
        printf("Failed to load: libnowindusbms.dll\n");
		return;
	}

	nowindusb_init      = (NoWind_Init)    GetProcAddress(hLib, "nowindusb_init");
	nowindusb_startup   = (NoWind_StartUp) GetProcAddress(hLib, "nowindusb_startup");
	nowindusb_cleanup   = (NoWind_Cleanup) GetProcAddress(hLib, "nowindusb_cleanup");
	nowindusb_set_image = (NoWind_SetImage)GetProcAddress(hLib, "nowindusb_set_image");
	nowindusb_write     = (NoWind_Write)   GetProcAddress(hLib, "nowindusb_write");
	nowindusb_read      = (NoWind_Read)    GetProcAddress(hLib, "nowindusb_read");
    nowindusb_set_debug_callback = (NoWind_SetDebugCb) GetProcAddress(hLib, "nowindusb_set_debug_callback");
}

void nowindUnloadDll()
{
    nowindusb_init      = NULL;
    nowindusb_startup   = NULL;
    nowindusb_cleanup   = NULL;
    nowindusb_set_image = NULL;
    nowindusb_write     = NULL;
    nowindusb_read      = NULL;

    if (hLib != NULL) {
	    FreeLibrary(hLib);
	    hLib = NULL;
    }
}

static void debugCb(const char* message)
{
    printf(message);
}

#endif


typedef struct {
    int deviceHandle;
    AmdFlash* amdFlash;
    int slot;
    int sslot;
    int startPage;
    UInt8 romMapper;
    UInt8* flashPage;
} RomMapperNoWind;




static void updateMapper(RomMapperNoWind* rm, UInt8 page)
{
    rm->romMapper = page & 0x1f;
    rm->flashPage = amdFlashGetPage(rm->amdFlash, rm->romMapper * 0x4000);

    slotMapPage(rm->slot, rm->sslot, rm->startPage + 0, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, NULL, 0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->flashPage, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->flashPage + 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 4, NULL,          0, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 5, rm->flashPage + 0x2000, 1, 0);
}


static void saveState(RomMapperNoWind* rm)
{
    SaveState* state = saveStateOpenForWrite("mapperDumas");

    saveStateSet(state, "romMapper", rm->romMapper);

    saveStateClose(state);

    amdFlashSaveState(rm->amdFlash);
}

static void loadState(RomMapperNoWind* rm)
{
    SaveState* state = saveStateOpenForRead("mapperDumas");

    rm->romMapper = (UInt8)saveStateGet(state, "romMapper", 0);

    saveStateClose(state);

    amdFlashLoadState(rm->amdFlash);

    updateMapper(rm, rm->romMapper);
}

static void destroy(RomMapperNoWind* rm)
{
    amdFlashDestroy(rm->amdFlash);
#ifdef USE_NOWIND_DLL
    if (nowindusb_cleanup) nowindusb_cleanup();
    nowindUnloadDll();
#endif
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);

    free(rm);
}

static void reset(RomMapperNoWind* rm)
{
    amdFlashReset(rm->amdFlash);

    updateMapper(rm, 0);
}

static UInt8 read(RomMapperNoWind* rm, UInt16 address) 
{
    if ((address >= 0x2000 && address < 0x4000) || 
        (address >= 0x8000 && address < 0xa000))
    {
#ifdef USE_NOWIND_DLL
        if (nowindusb_read) {
            UInt8 value = nowindusb_read();
            return value;
        }
#endif
    }

    return 0xff;
}

static UInt8 peek(RomMapperNoWind* rm, UInt16 address) 
{
    return 0xff;
}

static void write(RomMapperNoWind* rm, UInt16 address, UInt8 value) 
{   
    if (address < 0x4000) {
        amdFlashWrite(rm->amdFlash, address + 0x4000 * rm->romMapper, value);
        return;
    }

    if ((address >= 0x4000 && address < 0x6000) || 
        (address >= 0x8000 && address < 0xa000)) 
    {
#ifdef USE_NOWIND_DLL
        if (nowindusb_write) nowindusb_write(value);
#endif
        return;
    }

    if ((address >= 0x6000 && address < 0x8000) || 
        (address >= 0xa000 && address < 0xc000)) 
    {
        // FIXME: Is the page selected based on address or data lines?
        updateMapper(rm, value & 0x1f);
    }
}

int romMapperNoWindCreate(int driveId, char* filename, UInt8* romData, 
                         int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    RomMapperNoWind* rm;

    rm = malloc(sizeof(RomMapperNoWind));

    rm->deviceHandle = deviceManagerRegister(ROM_NOWIND, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 6, read, peek, write, destroy, rm);

    if (filename == NULL) {
        filename = "nowind.rom";
    }
    rm->amdFlash = amdFlashCreate(AMD_TYPE_1, 0x80000, 0x10000, 0, romData, size, sramCreateFilenameWithSuffix(filename, "", ".rom"), 0);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

#ifdef USE_NOWIND_DLL
    nowindLoadDll();
    if (nowindusb_init)     nowindusb_init();
    if (nowindusb_startup)  nowindusb_startup();
    if (nowindusb_set_image) nowindusb_set_image(driveId, propGetGlobalProperties()->media.disks[
                                                 diskGetUsbDriveId(driveId, 0)].fileName);
    if (nowindusb_set_debug_callback) nowindusb_set_debug_callback(debugCb);
#endif

    reset(rm);

    return 1;
}
