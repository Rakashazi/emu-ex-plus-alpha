/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/ft245.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:41 $
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
#include "ft245.h"
#include "Board.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>

////////////////////////////////////////////////////////////////////////////
///     Ft245UsbHost
////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include "Disk.h"

typedef UInt8 (*ReadCb)(void*);
typedef void  (*WriteCb)(void*, UInt8);

typedef struct {
    int state;

    UInt8 reg_a;
    UInt8 reg_f;
    UInt8 reg_b;
    UInt8 reg_c;
    UInt8 reg_d;
    UInt8 reg_e;
    UInt8 reg_h;
    UInt8 reg_l;
    UInt16 parameterCount;

    UInt8 writeBuffer[64 * 1024];
    UInt16 writePointer;
    UInt8  transferBuffer[64 * 1024];
    UInt8  deviceBuffer[64 *1024];
    UInt16 devicePointer;
    UInt8  fileName[12];
    UInt16 fileLength;
    
    char debugString[250];

    int driveId;

    ReadCb  readCb;
    WriteCb writeCb;
    void*   ref;
    BoardTimer* timer;

} Ft245UsbHost;


#define ST_WAIT                 1
#define ST_AFDETECTED           2
#define ST_AF05DETECTED         3
#define ST_AF66DETECTED         4
#define ST_GETPARAMETERS        5
#define ST_DEBUGSTRING          6
#define ST_DEBUGDUMPREGISTERS   7
#define ST_WRITE                8
#define ST_DEVICEFILENAME       9


#define CMD_DISKIOREADPAGE0AND1      0
#define CMD_DISKIOREADPAGE2AND3      2
#define CMD_DISKIOWRITEPAGE0AND1     4
#define CMD_DISKIOWRITEPAGE2AND3     6
#define CMD_DISKCHANGED              8
#define CMD_LOWLEVELEXIT            10
#define CMD_BOOTOPTIONS             12
#define CMD_SETDATE                 14
#define CMD_EXECUTECODE             16
#define CMD_LOADCODE01              18
#define CMD_LOADCODE23              20


#define ERR_WRITEPROTECTED          0
#define ERR_NOTREADY                2
#define ERR_DATAERROR               4
#define ERR_SEEKERROR               6
#define ERR_RECORDNOTFOUND          8
#define ERR_WRITEFAULT             10
#define ERR_OTHERERROR             12
#define ERR_BADPARAMETER           12
#define ERR_INSUFFICIENTMEMORY     14
#define ERR_DSKFMT_OTHERERROR      16


static void ft245UsbHostReset(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

static void onTimer(Ft245UsbHost* host, UInt32 time)
{
    if (diskChanged(host->driveId)) {
        char sectorBuffer[512];
        
        _diskRead2(host->driveId, sectorBuffer, 1, 1);

        host->writeCb(host->ref, 0x00);
        host->writeCb(host->ref, 0x00);
        host->writeCb(host->ref, sectorBuffer[0] & 255);
    }
    
    boardTimerAdd(host->timer, boardSystemTime() + boardFrequency());
}

static Ft245UsbHost* ft245UsbHostCreate(int driveId, ReadCb readCb, WriteCb writeCb, void* ref)
{
    Ft245UsbHost* host = calloc(1, sizeof(Ft245UsbHost));

    host->driveId = diskGetUsbDriveId(driveId, 0);

    host->readCb  = readCb;
    host->writeCb = writeCb;
    host->ref     = ref;

    host->timer = boardTimerCreate(onTimer, host);
    boardTimerAdd(host->timer, boardSystemTime() + boardFrequency());

    ft245UsbHostReset(host);

    return host;
}

static void ft245UsbHostDestroy(Ft245UsbHost* host)
{
    boardTimerDestroy(host->timer);
    free(host);
}

void ft245UsbHostSaveState(Ft245UsbHost* host)
{
}

void ft245UsbHostLoadState(Ft245UsbHost* host)
{
}

static void ft245UsbHostSendCommand(Ft245UsbHost* host, UInt8 command)
{
    printf("Sending USB command %d\n", command);
//    if (command == 10) actionEmuTogglePause();
    host->writeCb(host->ref, 0xaf);
    host->writeCb(host->ref, 0x05);
    host->writeCb(host->ref, command);
    host->writeCb(host->ref, 0x00);
}

void ft245UsbHostTransferSectors(Ft245UsbHost* host, UInt16 address, UInt16 amount, UInt8* data) 
{
    UInt16 i;

    printf("Going to read %u bytes to address: 0x%04X\n", amount, address);

    host->writeCb(host->ref, address & 0xff);
    host->writeCb(host->ref, address >> 8);
    host->writeCb(host->ref, amount & 0xff);
    host->writeCb(host->ref, amount >> 8);

    for (i = 0; i < amount; i++) {
        host->writeCb(host->ref, data[i]);
    }
    host->writeCb(host->ref, 0xaf);
    host->writeCb(host->ref, 0x07);
}

static void ft245UsbHostDiskioWriteExit(Ft245UsbHost* host, int error, UInt8 errorCode) 
{
    ft245UsbHostSendCommand(host, CMD_LOWLEVELEXIT);
    if (error) {
        host->writeCb(host->ref, host->reg_f | 1);
        host->writeCb(host->ref, errorCode);
        host->writeCb(host->ref, host->reg_c);
        host->writeCb(host->ref, 1);
    } 
    else {
        host->writeCb(host->ref, host->reg_f & 0xfe);
        host->writeCb(host->ref, host->reg_a);
        host->writeCb(host->ref, host->reg_c);
        host->writeCb(host->ref, 0);
    }
}

static void ft245UsbHostDskio(Ft245UsbHost* host) 
{
    UInt16 sectorAmount;
    UInt16 startSector;
    UInt16 transferAddress;

    sectorAmount = host->reg_b;
    if (sectorAmount == 0) {
        sectorAmount = 256; 
    }

    startSector = host->reg_e + (host->reg_d * 256);
    if (host->reg_c < 0x80) {
        startSector += host->reg_c << 16;
    }

    transferAddress = host->reg_l + (host->reg_h * 256);
       
    if (host->reg_f & 1) {
        // diskio write
        if (diskReadOnly(host->driveId)) {
            ft245UsbHostDiskioWriteExit(host, 1, ERR_WRITEPROTECTED);
            host->state = ST_WAIT;
            return;
        }
        host->writePointer = 0;
        if (transferAddress >= 0x8000) {
            ft245UsbHostSendCommand(host, CMD_DISKIOWRITEPAGE2AND3);
            host->writeCb(host->ref, transferAddress & 0xff);
            host->writeCb(host->ref, transferAddress >> 8);
            host->writeCb(host->ref, (sectorAmount * 512) & 0xff);
            host->writeCb(host->ref, (sectorAmount * 512) >> 8);
        } 
        else {
            UInt16 endAddress;

            ft245UsbHostSendCommand(host, CMD_DISKIOWRITEPAGE0AND1);
            host->writeCb(host->ref, transferAddress & 0xff);
            host->writeCb(host->ref, transferAddress >> 8);
            endAddress = transferAddress + (sectorAmount * 512);
            
            if (endAddress > 0x8000) {
                host->writeCb(host->ref, (0x8000 - transferAddress) & 0xff);
                host->writeCb(host->ref, (0x8000 - transferAddress) >> 8);
                ft245UsbHostSendCommand(host, CMD_DISKIOWRITEPAGE2AND3);
                host->writeCb(host->ref, (0x8000) & 0xff);
                host->writeCb(host->ref, (0x8000) >> 8);
                host->writeCb(host->ref, (endAddress - 0x8000) & 0xff);
                host->writeCb(host->ref, (endAddress - 0x8000) >> 8);
            }
            else {
                host->writeCb(host->ref, (endAddress - transferAddress) & 0xff);
                host->writeCb(host->ref, (endAddress - transferAddress) >> 8);
            }
        }
        
        // set the S_WRITE, so the UsbHost is ready to receive data
        // the state will become S_WAIT after enough data has been
        // received to complete the command
        host->state = ST_WRITE;
    } 
    else {
        // diskio read
        int rv = _diskRead2(host->driveId, host->transferBuffer, startSector, sectorAmount);

        printf("Reading sector %d - %d, %s\n", startSector, sectorAmount, (rv ? "OK" : "FAILED"));

        if (transferAddress >= 0x8000) {
            ft245UsbHostSendCommand(host, CMD_DISKIOREADPAGE2AND3);
            ft245UsbHostTransferSectors(host, transferAddress, sectorAmount * 512, host->transferBuffer);
        } 
        else {
            UInt16 endAddress;

            ft245UsbHostSendCommand(host, CMD_DISKIOREADPAGE0AND1);
            endAddress = transferAddress + (sectorAmount * 512);
            
            if (endAddress > 0x8000) {
                ft245UsbHostTransferSectors(host, transferAddress, 0x8000 - transferAddress, host->transferBuffer);
                ft245UsbHostSendCommand(host, CMD_DISKIOREADPAGE2AND3);
                ft245UsbHostTransferSectors(host, 0x8000, endAddress - 0x8000, host->transferBuffer + 0x8000 - transferAddress);
            } else {
                ft245UsbHostTransferSectors(host, transferAddress, endAddress - transferAddress, host->transferBuffer);
                ft245UsbHostDiskioWriteExit(host, 0, 0); 
            }
        }
        // back to the S_WAIT state, ready for new commands
        host->state = ST_WAIT;
    }
}

static void ft245UsbHostDskchg(Ft245UsbHost* host) 
{
    host->state = ST_WAIT;
}

static void ft245UsbHostGetdpb(Ft245UsbHost* host) 
{
    host->state = ST_WAIT;
}

static void ft245UsbHostChoice(Ft245UsbHost* host) 
{
    host->state = ST_WAIT;
}

static void ft245UsbHostDskfmt(Ft245UsbHost* host) 
{
    if (diskReadOnly(host->driveId)) {
        ft245UsbHostDiskioWriteExit(host, 1, ERR_WRITEPROTECTED);
        host->state = ST_WAIT;
        return;
    }

    ft245UsbHostSendCommand(host, CMD_LOWLEVELEXIT);
    host->writeCb(host->ref, host->reg_f | 1);
    host->writeCb(host->ref, ERR_DSKFMT_OTHERERROR);
    host->writeCb(host->ref, host->reg_c);
    host->writeCb(host->ref, host->reg_b);

    host->state = ST_WAIT;
}

static void ft245UsbHostBootOptions(Ft245UsbHost* host) 
{
    ft245UsbHostSendCommand(host, CMD_BOOTOPTIONS);
    host->writeCb(host->ref, 0x00);
    host->writeCb(host->ref, 0x02);

    host->state = ST_WAIT;
}

static void ft245UsbHostSetDate(Ft245UsbHost* host) 
{
    time_t td = time(NULL);
	struct tm *tm = localtime(&td);
    
    ft245UsbHostSendCommand(host, CMD_SETDATE);
    host->writeCb(host->ref, (tm->tm_year + 1900) & 0xff);
    host->writeCb(host->ref, (tm->tm_year + 1900) >> 8);
    host->writeCb(host->ref, tm->tm_mday);
    host->writeCb(host->ref, tm->tm_mon + 1);

    host->state = ST_WAIT;
}

static void ft245UsbHostDeviceOpen(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

static void ft245UsbHostDeviceClose(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

static void ft245UsbHostDeviceRandomIO(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

static void ft245UsbHostDeviceOutput(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

static void ft245UsbHostDeviceInput(Ft245UsbHost* host)
{
    host->state = ST_WAIT;
}

void ft245UsbHostTrigger(Ft245UsbHost* host) 
{
    switch (host->state) {
    case ST_WAIT:
        if (host->readCb(host->ref) == 0xaf) {
            host->state = ST_AFDETECTED;
        }
        break;
    
    case ST_AFDETECTED:
        switch (host->readCb(host->ref)) {
        case 0x05: 
            host->parameterCount = 0;
            host->state = ST_AF05DETECTED; 
            break;
        case 0x66: 
            host->state = ST_AF66DETECTED; 
            break;
        case 0xff:
            host->state = ST_WAIT;
            break;
        case 0xaf: break;
        default: 
            host->state = ST_WAIT;
        }
        break;

    case ST_AF66DETECTED:
        switch (host->readCb(host->ref)) {
        case 0:
            host->state = ST_DEBUGSTRING;
            host->debugString[0] = 0;
            break;
        case 1:
            host->parameterCount = 0;
            host->state = ST_DEBUGDUMPREGISTERS;
            break;
        case 2:
            {
                int i;
                for (i = 0; i < 32768; i++) host->writeCb(host->ref, 0);
                host->state = ST_WAIT;
            }
            break;
        case 3:
            host->parameterCount = 0;
            host->state = 123;
            break;
        default:
            host->state = ST_WAIT;
        }
        break;

    case ST_DEBUGSTRING: 
        {
            UInt8 temp = host->readCb(host->ref);
            if (temp == 0) {
                host->state = ST_WAIT;
            } 
            else sprintf(host->debugString, "%s%c", host->debugString, temp);
            break;
        }
    
    case ST_AF05DETECTED:
        switch (host->parameterCount++) {
        case 0: host->reg_c = host->readCb(host->ref); break;
        case 1: host->reg_b = host->readCb(host->ref); break;
        case 2: host->reg_e = host->readCb(host->ref); break;
        case 3: host->reg_d = host->readCb(host->ref); break;
        case 4: host->reg_l = host->readCb(host->ref); break;
        case 5: host->reg_h = host->readCb(host->ref); break;
        case 6: host->reg_f = host->readCb(host->ref); break;
        case 7: host->reg_a = host->readCb(host->ref); break;
        case 8:
            {
                UInt8 command = host->readCb(host->ref);
                printf("Got USB Command: %d\n", command);
                switch (command) {
                case 0: ft245UsbHostDskio(host); break;
                case 1: ft245UsbHostDskchg(host); break;
                case 2: ft245UsbHostGetdpb(host); break;
                case 3: ft245UsbHostChoice(host); break;
                case 4: ft245UsbHostDskfmt(host); break;
                case 6: ft245UsbHostBootOptions(host); break;
                case 7: ft245UsbHostSetDate(host); break;
                case 8:
                    host->parameterCount = 0;
                    host->state = ST_DEVICEFILENAME;
                    break;
                case 9: ft245UsbHostDeviceClose(host); break;
                case 10: ft245UsbHostDeviceRandomIO(host); break;
                case 11: ft245UsbHostDeviceOutput(host); break;
                case 12: ft245UsbHostDeviceInput(host); break;
                default:
                    host->state = ST_WAIT;
                }
            }
            break;
        }
        break;

    case ST_WRITE: 
        {
            UInt32 startSector = host->reg_e + (256 * host->reg_d);
            UInt32 size = 512 * host->reg_b;

            host->writeBuffer[host->writePointer++] = host->readCb(host->ref);
           
            if (host->writePointer == size) {
                _diskWrite2(host->driveId, host->writeBuffer, startSector, host->reg_b);
                ft245UsbHostDiskioWriteExit(host, 0, 0);
                host->state = ST_WAIT;
            }
            break;
        }
    case ST_DEVICEFILENAME: 
        {
            UInt8 tmp = host->readCb(host->ref);
            if ((tmp >= 'a') && (tmp <= 'z')) tmp &= 0xdf;
            host->fileName[host->parameterCount++] = tmp;
            if (host->parameterCount == 11) {
                ft245UsbHostDeviceOpen(host);
            }
        }
        break;
    }
}



////////////////////////////////////////////////////////////////////////////
///     FIFO
////////////////////////////////////////////////////////////////////////////

typedef struct {
    int head;
    int len;
    int size;
    UInt8 data[1];
} Fifo;

static Fifo* fifoCreate(int size)
{
    Fifo* fifo = calloc(1, sizeof(Fifo) + size);

    fifo->size = size;

    return fifo;
}

static void fifoDestroy(Fifo* fifo)
{
    free(fifo);
}

void fifoSaveState(Fifo* fifo)
{
}

void fifoLoadState(Fifo* fifo)
{
}

void fifoReset(Fifo* fifo)
{
    fifo->len = 0;
}

static int fifoIsFull(Fifo* fifo)
{
    return fifo->len == fifo->size;
}

static int fifoIsEmpty(Fifo* fifo)
{
    return fifo->len == 0;
}

static void fifoPush(Fifo* fifo,UInt8 value) 
{
    if (!fifoIsFull(fifo)) {
        fifo->head = (fifo->head + 1) % fifo->size;
        fifo->data[fifo->head] = value;
        fifo->len++;
    }
}

static UInt8 fifoPop(Fifo* fifo) 
{
    if (!fifoIsEmpty(fifo)) {
        fifo->len--;
        return fifo->data[(fifo->head + fifo->size - fifo->len) % fifo->size];
    }
    return 0xff;
}

static UInt8 fifoFront(Fifo* fifo) 
{
    if (!fifoIsEmpty(fifo)) {
        return fifo->data[(fifo->head + fifo->size - (fifo->len - 1)) % fifo->size];
    }
    return 0xff;
}




////////////////////////////////////////////////////////////////////////////
///     FT245
////////////////////////////////////////////////////////////////////////////

typedef struct FT245
{
    Fifo* sendFifo;
    Fifo* recvFifo;
    UInt32 dataRecvTime;
    Ft245UsbHost* usbHost;
};


void ft245SaveState(FT245* ft)
{
    fifoSaveState(ft->recvFifo);
    fifoSaveState(ft->sendFifo);
    ft245UsbHostSaveState(ft->usbHost);
}

void ft245LoadState(FT245* ft)
{
    fifoLoadState(ft->recvFifo);
    fifoLoadState(ft->sendFifo);
    ft245UsbHostLoadState(ft->usbHost);
}

static UInt8 hostRead(FT245* ft)
{
    UInt8 value = fifoPop(ft->sendFifo);
    return value;
}

static void hostSend(FT245* ft, UInt8 value)
{
    if (fifoIsEmpty(ft->recvFifo)) {
        ft->dataRecvTime = boardSystemTime();
    }
    fifoPush(ft->recvFifo, value);
}

FT245* ft245Create(int driveId)
{
    FT245* ft = malloc(sizeof(FT245));

    ft->recvFifo = fifoCreate(65536);
    ft->sendFifo = fifoCreate(65536);

    ft->usbHost = ft245UsbHostCreate(driveId, hostRead, hostSend, ft);

    return ft;
}

void ft245Destroy(FT245* ft)
{
    ft245UsbHostDestroy(ft->usbHost);
    fifoDestroy(ft->recvFifo);
    fifoDestroy(ft->sendFifo);
    free(ft);
}

void ft245Reset(FT245* ft)
{
    fifoReset(ft->recvFifo);
    fifoReset(ft->sendFifo);
}

UInt8 ft245Peek(FT245* ft)
{
    UInt32 elapsed = boardSystemTime() - ft->dataRecvTime;
    if (elapsed < boardFrequency() / (1000000 / 1250)) {
        return 0xff;
    } 
    return fifoFront(ft->recvFifo);
}

UInt8 ft245Read(FT245* ft)
{
    UInt8 value;

    UInt32 elapsed = boardSystemTime() - ft->dataRecvTime;
    if (elapsed < boardFrequency() / (1000000 / 1250)) {
        return 0xff;
    } 

    value = fifoPop(ft->recvFifo);

    return value;
}

UInt8 ft245GetTxe(FT245* ft)
{
    return fifoIsEmpty(ft->sendFifo);
}

UInt8 ft245GetRxf(FT245* ft)
{
    return fifoIsFull(ft->recvFifo);
}

void ft245Write(FT245* ft, UInt8 value)
{
    fifoPush(ft->sendFifo, value);
    ft245UsbHostTrigger(ft->usbHost);
}
