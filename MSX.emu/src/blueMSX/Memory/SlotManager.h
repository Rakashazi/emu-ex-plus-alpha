/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/SlotManager.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-30 18:38:42 $
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
#ifndef SLOT_MANAGER_H
#define SLOT_MANAGER_H

#include "MsxTypes.h"


typedef UInt8 (*SlotRead)(void*, UInt16);
typedef void  (*SlotWrite)(void*, UInt16, UInt8);
typedef void  (*SlotEject)(void*);


void slotManagerCreate();
void slotManagerDestroy();

void slotManagerReset();

void slotLoadState();
void slotSaveState();

void slotWrite(void* ref, UInt16 address, UInt8 value);
UInt8 slotRead(void* ref, UInt16 address);
UInt8 slotPeek(void* ref, UInt16 address);

void slotRegister(int slot, int sslot, int startpage, int pages,
                  SlotRead readCb, SlotRead peekCb, SlotWrite writeCb, SlotEject ejectCb, void* ref);
void slotUnregister(int slot, int sslot, int startpage);

void slotRemove(int slot, int sslot);

void slotRegisterWrite0(SlotWrite writeCb, void* ref);
void slotUnregisterWrite0();

void slotSetRamSlot(int slot, int psl);
int slotGetRamSlot(int page);
void slotMapRamPage(int slot, int sslot, int page);

void slotMapPage(int slot, int sslot, int page, UInt8* pageData, 
                 int readEnable, int writeEnable);
void slotUnmapPage(int slot, int sslot, int page);
void slotUpdatePage(int slot, int sslot, int page, UInt8* pageData, 
                    int readEnable, int writeEnable);

void slotSetSubslotted(int slot, int subslotted);

#endif
