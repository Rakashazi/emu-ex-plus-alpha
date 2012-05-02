/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperNms1210Rs232.c,v $
**
** $Revision: 1.2 $
**
** $Date: 2009-04-30 03:53:28 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson, Johan van Leur
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
#include "romMapperNms1210Rs232.h"
#include "MediaDb.h"
#include "IoPort.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DebugDeviceManager.h"
#include "SaveState.h"
#include "Board.h"
#include "Z8530.h"
#include "I8254.h"
#include "ArchUart.h"
#include "Language.h"
#include <stdlib.h>
#include <string.h>

/*
  37H   ?   Misc. status level
  38H  R/W  Z8530 channel B command
  39H  R/W  Z8530 channel B data
  3AH  R/W  Z8530 channel A command
  3BH  R/W  Z8530 channel A data
  3CH  R/W  8253 counter 0
  3DH  R/W  8253 counter 1
  3EH  R/W  8253 counter 2
  3FH   W   8253 mode register
*/

#define STATUS_CD   0x01
#define STATUS_RI   0x02
#define STATUS_OUT2 0x40
#define STATUS_CTS  0x80

#define INTMASK_RXREADY 0x01
#define INTMASK_TXREADY 0x02
#define INTMASK_SBRK    0x04
#define INTMASK_OUT2    0x08

#define CMD_RSTERR 0x10
#define CMD_RTS    0x20
#define CMD_RESET  0x40
#define CMD_HUNT   0x80

typedef struct {
    int deviceHandle;
    int debugHandle;
    int slot;
    int sslot;
    int startPage;
    int serialLink;
    Z8530* z8530;
    I8254* i8254;
    UInt8 status;   // Status for CTS, PIT Timer/Counter 2, RI and CD
    UInt8 intmask;  // Interrupt mask
} NMS1210Rs232;

static NMS1210Rs232* nms1210Rs232;

/*****************************************
** Device Manager callbacks
******************************************
*/
static void saveState(NMS1210Rs232* nms1210Rs232)
{
    SaveState* state = saveStateOpenForWrite("NMS1210Rs232");

    saveStateSet(state, "status",  nms1210Rs232->status);
    saveStateSet(state, "intmask",  nms1210Rs232->intmask);

    saveStateClose(state);

    z8530SaveState(nms1210Rs232->z8530);
    i8254SaveState(nms1210Rs232->i8254);
}

static void loadState(NMS1210Rs232* nms1210Rs232)
{
    SaveState* state = saveStateOpenForRead("NMS1210Rs232");

    nms1210Rs232->status  = (UInt8)saveStateGet(state, "status",  0);
    nms1210Rs232->intmask  = (UInt8)saveStateGet(state, "intmask",  0);

    saveStateClose(state);
    
    z8530LoadState(nms1210Rs232->z8530);
    i8254LoadState(nms1210Rs232->i8254);
}

static void destroy(NMS1210Rs232* nms1210Rs232)
{
    ioPortUnregister(0x37);
    ioPortUnregister(0x38);
    ioPortUnregister(0x39);
    ioPortUnregister(0x3a);
    ioPortUnregister(0x3b);
    ioPortUnregister(0x3c);
    ioPortUnregister(0x3d);
    ioPortUnregister(0x3e);
    ioPortUnregister(0x3f);

    z8530Destroy(nms1210Rs232->z8530);
    i8254Destroy(nms1210Rs232->i8254);

    slotUnregister(nms1210Rs232->slot, nms1210Rs232->sslot, nms1210Rs232->startPage);
    deviceManagerUnregister(nms1210Rs232->deviceHandle);
    debugDeviceUnregister(nms1210Rs232->debugHandle);

    free(nms1210Rs232);
}

static void reset(NMS1210Rs232* nms1210Rs232) 
{
    z8530Reset(nms1210Rs232->z8530);
    i8254Reset(nms1210Rs232->i8254);
    nms1210Rs232->status = 0;
    nms1210Rs232->intmask = 0;
}

/*****************************************
** Slot callbacks
******************************************
*/
static UInt8 read(NMS1210Rs232* nms1210Rs232, UInt16 address) 
{
    return 0xff;
}

static UInt8 peek(NMS1210Rs232* nms1210Rs232, UInt16 address) 
{
    return 0xff;
}

static void write(NMS1210Rs232* nms1210Rs232, UInt16 address, UInt8 value)
{
}

/*****************************************
** IO Port callbacks
******************************************
*/
static UInt8 peekIo(NMS1210Rs232* nms1210Rs232, UInt16 ioPort) 
{
    return 0xff;
}

static UInt8 readIo(NMS1210Rs232* nms1210Rs232, UInt16 ioPort) 
{
    UInt8 value = 0xff;

    switch (ioPort) {
    case 0x37:
        break;
    case 0x38:
        value = z8530Read(nms1210Rs232->z8530, 0);
        break;
    case 0x39:
        value = z8530Read(nms1210Rs232->z8530, 2);
        break;
    case 0x3a:
        value = z8530Read(nms1210Rs232->z8530, 1);
        break;
    case 0x3b:
        value = z8530Read(nms1210Rs232->z8530, 3);
        break;
    case 0x3c:
    case 0x3d:
    case 0x3e:
        value = i8254Read(nms1210Rs232->i8254, ioPort & 0x03);
        break;
    }

    return value;
}

static void writeIo(NMS1210Rs232* nms1210Rs232, UInt16 ioPort, UInt8 value) 
{
    switch (ioPort) {
    case 0x37:
        break;
    case 0x38:
        z8530Write(nms1210Rs232->z8530, 0, value);
        break;
    case 0x39:
        z8530Write(nms1210Rs232->z8530, 2, value);
        break;
    case 0x3a:
        z8530Write(nms1210Rs232->z8530, 1, value);
        break;
    case 0x3b:
        z8530Write(nms1210Rs232->z8530, 3, value);
        break;
    case 0x3c:
    case 0x3d:
    case 0x3e:
    case 0x3f:
        i8254Write(nms1210Rs232->i8254, ioPort & 0x03, value);
        break;
    }
}

/*****************************************
** I8251 callbacks
******************************************
*/
static int rs232transmit(NMS1210Rs232* nms1210Rs232, UInt8 value) {
    return 0;
}

static int rs232signal(NMS1210Rs232* nms1210Rs232) {
    return 0;
}

static void setDataBits(NMS1210Rs232* nms1210Rs232, int value) {
}

static void setStopBits(NMS1210Rs232* nms1210Rs232, int value) {
}

static void setParity(NMS1210Rs232* nms1210Rs232, int value) {
}

static void setRxReady(NMS1210Rs232* nms1210Rs232, int status)
{
    if (~nms1210Rs232->intmask & INTMASK_RXREADY) {
        if (status)
            boardSetInt(1);
        else
            boardClearInt(1);
    }
}

static void setDtr(NMS1210Rs232* nms1210Rs232, int status) {
}

static void setRts(NMS1210Rs232* nms1210Rs232, int status) {
}

static int getDtr(NMS1210Rs232* nms1210Rs232) {
    return 0;
}

static int getRts(NMS1210Rs232* nms1210Rs232) {
    return 0;
}

/*****************************************
** I8254 callbacks
******************************************
*/
static void pitOut0(NMS1210Rs232* nms1210Rs232, int state) 
{
    //msxRs232->i8251->rxClk(msxRs232, state);
}
static void pitOut1(NMS1210Rs232* nms1210Rs232, int state) 
{
    //msxRs232->i8251->txClk(msxRs232, state);
}

static void pitOut2 (NMS1210Rs232* nms1210Rs232, int state) 
{
    nms1210Rs232->status = (state) ? nms1210Rs232->status|STATUS_OUT2 : nms1210Rs232->status&~STATUS_OUT2;
}

/*****************************************
** ARCH UART callbacks
******************************************
*/
static void romMapperMsxRs232ReceiveCallback(UInt8 value)
{
//    i8251RxData(msxRs232->i8251, value);
}

/*****************************************
** Debug callbacks
******************************************
*/

static void getDebugInfo(NMS1210Rs232* nms1210Rs232, DbgDevice* dbgDevice)
{
    DbgIoPorts* ioPorts;
    int i;

    ioPorts = dbgDeviceAddIoPorts(dbgDevice, langDbgDevRs232(), 8);
    for (i = 0; i < 7; i++) {
        dbgIoPortsAddPort(ioPorts, i, 0x80 + i, DBG_IO_READWRITE, peekIo(nms1210Rs232, 0x80 + i));
    }
    dbgIoPortsAddPort(ioPorts, 1, 0x87, DBG_IO_WRITE, 0);
}

/*****************************************
** NMS1210 RS-232 Create Method
******************************************
*/
int romMapperNms1210Rs232Create(int slot, int sslot, int startPage)
{
    DeviceCallbacks callbacks = {destroy, reset, saveState, loadState};
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    int pages = 4;
    int i;

    if ((startPage + pages) > 8) {
        return 0;
    }

    nms1210Rs232 = malloc(sizeof(NMS1210Rs232));

    nms1210Rs232->deviceHandle = deviceManagerRegister(ROM_MSXRS232, &callbacks, nms1210Rs232);
    nms1210Rs232->debugHandle = debugDeviceRegister(DBGTYPE_BIOS, langDbgDevRs232(), &dbgCallbacks, nms1210Rs232);

    slotRegister(slot, sslot, startPage, pages, read, peek, write, destroy, nms1210Rs232);

    nms1210Rs232->slot  = slot;
    nms1210Rs232->sslot = sslot;
    nms1210Rs232->startPage  = startPage;

    for (i = 0; i < pages; i++) {
        slotMapPage(slot, sslot, i + startPage, NULL, 0, 0);
    }

//    nms1210Rs232->i8251 = i8251Create(rs232transmit, rs232signal, setDataBits, setStopBits, setParity, 
//                                 setRxReady, setDtr, setRts, getDtr, getRts, nms1210Rs232);
    nms1210Rs232->z8530 = z8530Create(nms1210Rs232);

    nms1210Rs232->i8254 = i8254Create(3686400, pitOut0, pitOut1, pitOut2, nms1210Rs232);

    nms1210Rs232->serialLink = archUartCreate(romMapperMsxRs232ReceiveCallback);

    ioPortRegister(0x37, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x38, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x39, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3a, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3b, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3c, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3d, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3e, readIo, writeIo, nms1210Rs232);
    ioPortRegister(0x3f, NULL, writeIo, nms1210Rs232);

    reset(nms1210Rs232);

    return 1;
}
