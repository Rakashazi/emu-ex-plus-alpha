/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/wd33c93.c,v $
**
** $Revision: 1.14 $
**
** $Date: 2008-03-30 18:38:41 $
**
** Based on the WD33C93 emulation in MESS (www.mess.org).
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson, white cat
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
#include "wd33c93.h"
#include "ScsiDefs.h"
#include "ScsiDevice.h"
#include "ArchCdrom.h"
#include "Disk.h"
#include "Board.h"
#include "SaveState.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REG_OWN_ID      0x00
#define REG_CONTROL     0x01
#define REG_TIMEO       0x02
#define REG_TSECS       0x03
#define REG_THEADS      0x04
#define REG_TCYL_HI     0x05
#define REG_TCYL_LO     0x06
#define REG_ADDR_HI     0x07
#define REG_ADDR_2      0x08
#define REG_ADDR_3      0x09
#define REG_ADDR_LO     0x0a
#define REG_SECNO       0x0b
#define REG_HEADNO      0x0c
#define REG_CYLNO_HI    0x0d
#define REG_CYLNO_LO    0x0e
#define REG_TLUN        0x0f
#define REG_CMD_PHASE   0x10
#define REG_SYN         0x11
#define REG_TCH         0x12
#define REG_TCM         0x13
#define REG_TCL         0x14
#define REG_DST_ID      0x15
#define REG_SRC_ID      0x16
#define REG_SCSI_STATUS 0x17    // (r)
#define REG_CMD         0x18
#define REG_DATA        0x19
#define REG_QUEUE_TAG   0x1a
#define REG_AUX_STATUS  0x1f    // (r)

#define REG_CDBSIZE     0x00
#define REG_CDB1        0x03
#define REG_CDB2        0x04
#define REG_CDB3        0x05
#define REG_CDB4        0x06
#define REG_CDB5        0x07
#define REG_CDB6        0x08
#define REG_CDB7        0x09
#define REG_CDB8        0x0a
#define REG_CDB9        0x0b
#define REG_CDB10       0x0c
#define REG_CDB11       0x0d
#define REG_CDB12       0x0e

#define OWN_EAF         0x08    // ENABLE ADVANCED FEATURES

// SCSI STATUS
#define SS_RESET        0x00    // reset
#define SS_RESET_ADV    0x01    // reset w/adv. features
#define SS_XFER_END     0x16    // select and transfer complete
#define SS_SEL_TIMEOUT  0x42    // selection timeout
#define SS_DISCONNECT   0x85

// AUX STATUS
#define AS_DBR          0x01    // data buffer ready
#define AS_CIP          0x10    // command in progress, chip is busy
#define AS_BSY          0x20    // Level 2 command in progress
#define AS_LCI          0x40    // last command ignored
#define AS_INT          0x80

/*
#define MCI_IO          0x01
#define MCI_CD          0x02
#define MCI_MSG         0x04
#define MCI_COMMAND     MCI_CD
#define MCI_DATAIN      MCI_IO
#define MCI_DATAOUT     0
#define MCI_STATUS      (MCI_CD  | MCI_IO)
#define MCI_MSGIN       (MCI_MSG | MCI_CD | MCI_IO)
#define MCI_MSGOUT      (MCI_MSG | MCI_CD)
*/

/* command phase
0x00    NO_SELECT
0x10    SELECTED
0x20    IDENTIFY_SENT
0x30    COMMAND_OUT
0x41    SAVE_DATA_RECEIVED
0x42    DISCONNECT_RECEIVED
0x43    LEGAL_DISCONNECT
0x44    RESELECTED
0x45    IDENTIFY_RECEIVED
0x46    DATA_TRANSFER_DONE
0x47    STATUS_STARTED
0x50    STATUS_RECEIVED
0x60    COMPLETE_RECEIVED
*/

#define TARGET  wd33c93->dev[wd33c93->targetId]

struct WD33C93
{
    int         myId;
    int         targetId;
    UInt8       latch;
    UInt8       regs[32];
    SCSIDEVICE* dev[8];
    int         maxDev;
    SCSI_PHASE  phase;
    //SCSI_PHASE  nextPhase;
    //BoardTimer* timer;
    //UInt32    timeout;
    //int       timerRunning;
    int         counter;
    int         blockCounter;
    int         tc;
    int         devBusy;
    int         hdId;
    UInt8*      pBuf;
    UInt8*      buffer;
};

static FILE* scsiLog = NULL;

static void wd33c93Disconnect(WD33C93* wd33c93)
{
    if (wd33c93->phase != BusFree) {
        if ((wd33c93->targetId >= 0) && (wd33c93->targetId < wd33c93->maxDev)) {
            scsiDeviceDisconnect(TARGET);
        }
        if (wd33c93->regs[REG_SCSI_STATUS] != SS_XFER_END) {
            wd33c93->regs[REG_SCSI_STATUS] = SS_DISCONNECT;
        }
        wd33c93->regs[REG_AUX_STATUS]  = AS_INT;
        wd33c93->phase          = BusFree;
        //wd33c93->nextPhase    = -1;
    }

    //wd33c93->mci          = -1;
    wd33c93->tc             = 0;
    //wd33c93->atn          = 0;

    SCSILOG("busfree()\n\n");
    scsiDeviceLogFlush();
}

/*
static void wd33c93Irq(WD33C93* wd33c93, UInt32 time)
{
    //wd33c93->timerRunning = 0;

    // set IRQ flag
    wd33c93->regs[REG_AUX_STATUS] = AS_DBR | AS_INT;
    // clear busy flags
    wd33c93->regs[REG_AUX_STATUS] &= ~(AS_CIP | AS_BSY);

    // select w/ATN and transfer
    wd33c93->regs[REG_SCSI_STATUS] = SS_XFER_END;
    wd33c93->regs[REG_CMD_PHASE] = 0x60; // command successfully completed

    //setInt();
}
*/

static void wd33c93XferCb(WD33C93* wd33c93, int length)
{
    wd33c93->devBusy = 0;
}

static void wd33c93ExecCmd(WD33C93* wd33c93, UInt8 value)
{
    int atn = 0;

    if (wd33c93->regs[REG_AUX_STATUS] & AS_CIP) {
        SCSILOG("wd33c93ExecCmd() CIP error\n");
        return;
    }
    //wd33c93->regs[REG_AUX_STATUS] |= AS_CIP;
    wd33c93->regs[REG_CMD] = value;
    switch (value) {
    case 0x00: /* Reset controller (software reset) */
        SCSILOG("wd33c93 [CMD] Reset controller\n");
        memset(wd33c93->regs + 1, 0, 0x1a);
        wd33c93Disconnect(wd33c93);
        wd33c93->latch  = 0;
        wd33c93->regs[REG_SCSI_STATUS] =
            (wd33c93->regs[REG_OWN_ID] & OWN_EAF) ? SS_RESET_ADV : SS_RESET;
        break;

    case 0x02:  /* Assert ATN */
        SCSILOG("wd33c93 [CMD] Assert ATN\n");
        break;

    case 0x04:  /* Disconnect */
        SCSILOG("wd33c93 [CMD] Disconnect\n");
        wd33c93Disconnect(wd33c93);
        break;

    case 0x06:  /* Select with ATN (Lv2) */
        atn = 1;
    case 0x07:  /* Select Without ATN (Lv2) */
        SCSILOG1("wd33c93 [CMD] Select (ATN %d)\n", atn);
        wd33c93->targetId = wd33c93->regs[REG_DST_ID] & 7;
        wd33c93->regs[REG_SCSI_STATUS] = SS_SEL_TIMEOUT;
        wd33c93->tc = 0;
        wd33c93->regs[REG_AUX_STATUS] = AS_INT;
        break;

    case 0x08:  /* Select with ATN and transfer (Lv2) */
        atn = 1;
    case 0x09:  /* Select without ATN and Transfer (Lv2) */
        SCSILOG1("wd33c93 [CMD] Select and transfer (ATN %d)\n", atn);
        wd33c93->targetId = wd33c93->regs[REG_DST_ID] & 7;

        if (!wd33c93->devBusy &&
            wd33c93->targetId < wd33c93->maxDev &&
            /* wd33c93->targetId != wd33c93->myId  && */
            scsiDeviceSelection(TARGET)) {
            if (atn) scsiDeviceMsgOut(TARGET, wd33c93->regs[REG_TLUN] | 0x80);
            wd33c93->devBusy = 1;
            wd33c93->counter = scsiDeviceExecuteCmd(
                            TARGET, &wd33c93->regs[REG_CDB1],
                            &wd33c93->phase, &wd33c93->blockCounter);

            switch (wd33c93->phase) {
            case Status:
                wd33c93->devBusy = 0;
                wd33c93->regs[REG_TLUN] = scsiDeviceGetStatusCode(TARGET);
                scsiDeviceMsgIn(TARGET);
                wd33c93->regs[REG_SCSI_STATUS] = SS_XFER_END;
                wd33c93Disconnect(wd33c93);
                break;

            case Execute:
                wd33c93->regs[REG_AUX_STATUS]  = AS_CIP | AS_BSY;
                wd33c93->pBuf = wd33c93->buffer;
                break;

            default:
                wd33c93->devBusy = 0;
                wd33c93->regs[REG_AUX_STATUS]  = AS_CIP | AS_BSY | AS_DBR;
                wd33c93->pBuf = wd33c93->buffer;
            }
            //wd33c93->regs[REG_SRC_ID] |= wd33c93->regs[REG_DST_ID] & 7;
        } else {
            //wd33c93->timeout = boardSystemTime() + boardFrequency() / 16384;
            //wd33c93->timerRunning = 1;
            //boardTimerAdd(wd33c93->timer, wd33c93->timeout);
            SCSILOG1("wd33c93 %d timeout\n", wd33c93->targetId);
            wd33c93->tc = 0;
            wd33c93->regs[REG_SCSI_STATUS]  = SS_SEL_TIMEOUT;
            wd33c93->regs[REG_AUX_STATUS]   = AS_INT;
        }
        break;

    case 0x18:  /* Translate Address (Lv2) */
    default:
        SCSILOG1("wd33c93 [CMD] unsupport command %x\n", value);
        break;
    }
}
void wd33c93WriteAdr(WD33C93* wd33c93, UInt16 port, UInt8 value)
{
    wd33c93->latch = value & 0x1f;
}

void wd33c93WriteCtrl(WD33C93* wd33c93, UInt16 port, UInt8 value)
{
    //SCSILOG2("wd33c93 write #%X, %X\n", wd33c93->latch, value);
    switch (wd33c93->latch) {
    case REG_OWN_ID:
        wd33c93->regs[REG_OWN_ID] = value;
        wd33c93->myId = value & 7;
        SCSILOG1("wd33c93 myid = %X\n", wd33c93->myId);
        break;

    case REG_TCH:
        wd33c93->tc = (wd33c93->tc & 0x0000ffff) + (value << 16);
        break;

    case REG_TCM:
        wd33c93->tc = (wd33c93->tc & 0x00ff00ff) + (value << 8);
        break;

    case REG_TCL:
        wd33c93->tc = (wd33c93->tc & 0x00ffff00) + value;
        break;

    case REG_CMD_PHASE:
        SCSILOG1("wd33c93 CMD_PHASE = %X\n", value);
        wd33c93->regs[REG_CMD_PHASE] = value;
        break;

    case REG_CMD:
        wd33c93ExecCmd(wd33c93, value);
        return;     

    case REG_DATA:
        wd33c93->regs[REG_DATA] = value;
        if (wd33c93->phase == DataOut) {
            *wd33c93->pBuf++ = value;
            --wd33c93->tc;
            if (--wd33c93->counter == 0) {
                wd33c93->counter = scsiDeviceDataOut(TARGET, &wd33c93->blockCounter);
                if (wd33c93->counter) {
                    wd33c93->pBuf = wd33c93->buffer;
                    return;
                }
                wd33c93->regs[REG_TLUN] = scsiDeviceGetStatusCode(TARGET);
                scsiDeviceMsgIn(TARGET);
                wd33c93->regs[REG_SCSI_STATUS] = SS_XFER_END;
                wd33c93Disconnect(wd33c93);
            }
        }
        return;

    case REG_AUX_STATUS:
        return;

    default:
        if (wd33c93->latch <= REG_SRC_ID) {
            wd33c93->regs[wd33c93->latch] = value;
        }
        break;
    }
    wd33c93->latch++;
    wd33c93->latch &= 0x1f;
}

UInt8 wd33c93ReadAuxStatus(WD33C93* wd33c93, UInt16 port)
{
    UInt8 rv = wd33c93->regs[REG_AUX_STATUS];

    if (wd33c93->phase == Execute) {
        wd33c93->counter =
          scsiDeviceExecutingCmd(TARGET, &wd33c93->phase, &wd33c93->blockCounter);

        switch (wd33c93->phase) {
        case Status:
            wd33c93->regs[REG_TLUN] = scsiDeviceGetStatusCode(TARGET);
            scsiDeviceMsgIn(TARGET);
            wd33c93->regs[REG_SCSI_STATUS] = SS_XFER_END;
            wd33c93Disconnect(wd33c93);
            break;

        case Execute:
            break;

        default:
            wd33c93->regs[REG_AUX_STATUS] |= AS_DBR;
        }
    }

    return rv;
}

UInt8 wd33c93ReadCtrl(WD33C93* wd33c93, UInt16 port)
{
    UInt8 rv;

    switch (wd33c93->latch) {
    case REG_SCSI_STATUS:
        rv = wd33c93->regs[REG_SCSI_STATUS];
        //SCSILOG1("wd33c93 SCSI_STATUS = %X\n", rv);
        if (rv != SS_XFER_END) {
            wd33c93->regs[REG_AUX_STATUS] &= ~AS_INT;
        } else {
            wd33c93->regs[REG_SCSI_STATUS] = SS_DISCONNECT;
            wd33c93->regs[REG_AUX_STATUS]  = AS_INT;
        }
        break;

    case REG_DATA:
        if (wd33c93->phase == DataIn) {
            rv = *wd33c93->pBuf++;
            wd33c93->regs[REG_DATA] = rv;
            --wd33c93->tc;
            if (--wd33c93->counter == 0) {
                if (wd33c93->blockCounter > 0) {
                    wd33c93->counter = scsiDeviceDataIn(TARGET, &wd33c93->blockCounter);
                    if (wd33c93->counter) {
                        wd33c93->pBuf = wd33c93->buffer;
                        return rv;
                    }
                }
                wd33c93->regs[REG_TLUN] = scsiDeviceGetStatusCode(TARGET);
                scsiDeviceMsgIn(TARGET);
                wd33c93->regs[REG_SCSI_STATUS] = SS_XFER_END;
                wd33c93Disconnect(wd33c93);
            }
        } else {
            rv = wd33c93->regs[REG_DATA];
        }
        return rv;

    case REG_TCH:
        rv = (UInt8)((wd33c93->tc >> 16) & 0xff);
        break;

    case REG_TCM:
        rv = (UInt8)((wd33c93->tc >> 8) & 0xff);
        break;

    case REG_TCL:
        rv = (UInt8)(wd33c93->tc & 0xff);
        break;

    case REG_AUX_STATUS:
        return wd33c93ReadAuxStatus(wd33c93, port);

    default:
        rv = wd33c93->regs[wd33c93->latch];
        break;
    }
    //SCSILOG2("wd33c93 read #%X, %X\n", wd33c93->latch, rv);

    if (wd33c93->latch != REG_CMD) {
            wd33c93->latch++;
            wd33c93->latch &= 0x1f;
    }
    return rv;
}

UInt8 wd33c93Peek(WD33C93* wd33c93, UInt16 port)
{
    UInt8 rv;

    port &= 1;
    if (port == 0) {
        rv = wd33c93->regs[REG_AUX_STATUS];
    } else {
        switch (wd33c93->latch) {
        case REG_TCH:
            rv = (UInt8)((wd33c93->tc >> 16) & 0xff);
            break;
        case REG_TCM:
            rv = (UInt8)((wd33c93->tc >> 8) & 0xff);
            break;
        case REG_TCL:
            rv = (UInt8)(wd33c93->tc & 0xff);
            break;
        default:
            rv = wd33c93->regs[wd33c93->latch];
            break;
        }
    }
    return rv;
}

void wd33c93Reset(WD33C93* wd33c93, int scsireset)
{
    int i;

    SCSILOG("wd33c93 reset\n");
    scsiDeviceLogFlush();

    // initialized register
    memset(wd33c93->regs, 0, 0x1b);
    memset(wd33c93->regs + 0x1b, 0xff, 4);
    wd33c93->regs[REG_AUX_STATUS] = AS_INT;
    wd33c93->myId   = 0;
    wd33c93->latch  = 0;
    wd33c93->tc     = 0;
    wd33c93->phase  = BusFree;
    wd33c93->pBuf   = wd33c93->buffer;
    if (scsireset) {
        for (i = 0; i < wd33c93->maxDev; ++i) {
            scsiDeviceReset(wd33c93->dev[i]);
        }
    }
}

void  wd33c93Destroy(WD33C93* wd33c93)
{
    int i;

    for (i = 0; i < 8; ++i) {
        if (wd33c93->dev[i]) scsiDeviceDestroy(wd33c93->dev[i]);
    }

    //boardTimerDestroy(wd33c93->timer);

    SCSILOG("WD33C93 destroy\n");
    scsiDeviceLogClose();

    archCdromBufferFree(wd33c93->buffer);
    free(wd33c93);
}

static SCSIDEVICE* wd33c93ScsiDevCreate(WD33C93* wd33c93, int id)
{
#if 1
    // CD_UPDATE: Use dynamic parameters instead of hard coded ones
    int diskId, mode, type;

    diskId = diskGetHdDriveId(wd33c93->hdId, id);
    if (diskIsCdrom(diskId)) {
        mode = MODE_SCSI1 | MODE_UNITATTENTION | MODE_REMOVABLE | MODE_NOVAXIS;
        type = SDT_CDROM;
    } else {
        mode = MODE_SCSI1 | MODE_UNITATTENTION | MODE_FDS120 | MODE_REMOVABLE | MODE_NOVAXIS;
        type = SDT_DirectAccess;
    }
    return scsiDeviceCreate(id, diskId, wd33c93->buffer, NULL, type, mode,
                           (CdromXferCompCb)wd33c93XferCb, wd33c93);
#else
    SCSIDEVICE* dev;
    int mode;
    int type;

    if (id != 2) {
        mode = MODE_SCSI1 | MODE_UNITATTENTION | MODE_FDS120 | MODE_REMOVABLE | MODE_NOVAXIS;
        type = SDT_DirectAccess;
    } else {
        mode = MODE_SCSI1 | MODE_UNITATTENTION | MODE_REMOVABLE | MODE_NOVAXIS;
        type = SDT_CDROM;
    }
    dev = scsiDeviceCreate(id, diskGetHdDriveId(wd33c93->hdId, id),
            wd33c93->buffer, NULL, type, mode, (CdromXferCompCb)wd33c93XferCb, wd33c93);
    return dev;
#endif
}

WD33C93* wd33c93Create(int hdId)
{
    WD33C93* wd33c93 = malloc(sizeof(WD33C93));
    int i;

    scsiLog = scsiDeviceLogCreate();
    wd33c93->buffer  = archCdromBufferMalloc(BUFFER_SIZE);
    wd33c93->maxDev  = 8;
    wd33c93->hdId    = hdId;
    wd33c93->devBusy = 0;
    //wd33c93->timer = boardTimerCreate(wd33c93Irq, wd33c93);

    memset(wd33c93->dev, 0, sizeof(wd33c93->dev));
    for (i = 0; i < wd33c93->maxDev; ++i) {
        wd33c93->dev[i] = wd33c93ScsiDevCreate(wd33c93, i);
    }

    wd33c93Reset(wd33c93, 0);

    return wd33c93;
}

void  wd33c93SaveState(WD33C93* wd33c93)
{
    SaveState* state = saveStateOpenForWrite("wd33c93");
    int i;

    saveStateSet(state, "myId",         wd33c93->myId);
    saveStateSet(state, "targetId",     wd33c93->targetId);
    saveStateSet(state, "latch",        wd33c93->latch);
    saveStateSet(state, "phase",        wd33c93->phase);
    saveStateSet(state, "counter",      wd33c93->counter);
    saveStateSet(state, "blockCounter", wd33c93->blockCounter);
    saveStateSet(state, "tc",           wd33c93->tc);
    saveStateSet(state, "maxDev",       wd33c93->maxDev);
    saveStateSet(state, "pBuf",         wd33c93->pBuf - wd33c93->buffer);
    //saveStateGet(state, "timeout",    wd33c93->timeout);
    //saveStateGet(state, "timerRunning", wd33c93->timerRunning);

    saveStateSetBuffer(state, "regs",   wd33c93->regs,   sizeof(wd33c93->regs));
    saveStateSetBuffer(state, "buffer", wd33c93->buffer, BUFFER_SIZE);

    saveStateClose(state);

    for (i = 0; i < wd33c93->maxDev; ++i) {
        scsiDeviceSaveState(wd33c93->dev[i]);
    }
}

void  wd33c93LoadState(WD33C93* wd33c93)
{
    SaveState* state = saveStateOpenForRead("wd33c93");
    int old = wd33c93->maxDev;
    int i;

    wd33c93->myId         =        saveStateGet(state, "myId",          0);
    wd33c93->targetId     =        saveStateGet(state, "targetId",      0);
    wd33c93->latch        = (UInt8)saveStateGet(state, "latch",         0);
    wd33c93->phase        =        saveStateGet(state, "phase",         BusFree);
    wd33c93->counter      =        saveStateGet(state, "counter",       0);
    wd33c93->blockCounter =        saveStateGet(state, "blockCounter",  0);
    wd33c93->tc           =        saveStateGet(state, "tc",            0);
    wd33c93->maxDev       =        saveStateGet(state, "maxDev",        8);
    wd33c93->pBuf         =        saveStateGet(state, "pBuf",          0) + wd33c93->buffer;
    //wd33c93->timeout    =        saveStateGet(state, "timeout",       0);
    //wd33c93->timerRunning =      saveStateGet(state, "timerRunning",  0);

    saveStateGetBuffer(state, "regs",   wd33c93->regs,   sizeof(wd33c93->regs));
    saveStateGetBuffer(state, "buffer", wd33c93->buffer, BUFFER_SIZE);

    saveStateClose(state);

    if (old < wd33c93->maxDev) {
        for (i = old; i < wd33c93->maxDev; ++i) {
            wd33c93->dev[i] = wd33c93ScsiDevCreate(wd33c93, i);
        }
    }

    for (i = 0; i < wd33c93->maxDev; ++i) {
        scsiDeviceLoadState(wd33c93->dev[i]);
    }
/*  
    if (wd33c93->timerRunning) {
        boardTimerAdd(wd33c93->timer, wd33c93->timeout);
    }
*/
}
