/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/MB89352.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2007-03-28 17:35:35 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2007 Daniel Vik, white cat
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
/*
 * Notes:
 *  Not suppport padding transfer and interrupt signal. (Not used MEGA-SCSI)
 *  Message system might be imperfect. (Not used in MEGA-SCSI usually)
 */
#include "MB89352.h"
#include "ScsiDevice.h"
#include "ScsiDefs.h"
#include "ArchCdrom.h"
#include "Disk.h"
#include "SaveState.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define USE_DEBUGGER

#ifdef USE_DEBUGGER
#include "DebugDeviceManager.h"
#include "Language.h"
#endif

#define TARGET spc->scsiDevice[spc->targetId]

#define REG_BDID  0     // Bus Device ID        (r/w)
#define REG_SCTL  1     // Spc Control          (r/w)
#define REG_SCMD  2     // Command              (r/w)
#define REG_OPEN  3     //                      (open)
#define REG_INTS  4     // Interrupt Sense      (r/w)
#define REG_PSNS  5     // Phase Sense          (r)
#define REG_SDGC  5     // SPC Diag. Control    (w)
#define REG_SSTS  6     // SPC Status           (r)
#define REG_SERR  7     // SPC Error Status     (r/w?)
#define REG_PCTL  8     // Phase Control        (r/w)
#define REG_MBC   9     // Modified Byte Counter(r)
#define REG_DREG 10     // Data Register        (r/w)
#define REG_TEMP 11     // Temporary Register   (r/w)
                        //   Another value is maintained respec-
                        //   tively for writing and for reading

#define REG_TCH  12     // Transfer Counter High(r/w)
#define REG_TCM  13     // Transfer Counter Mid (r/w)
#define REG_TCL  14     // Transfer Counter Low (r/w)

#define REG_TEMPWR 13   // (TEMP register preservation place for writing)
#define FIX_PCTL   14   // (REG_PCTL & 7)

#define PSNS_IO     0x01
#define PSNS_CD     0x02
#define PSNS_MSG    0x04
#define PSNS_BSY    0x08
#define PSNS_SEL    0x10
#define PSNS_ATN    0x20
#define PSNS_ACK    0x40
#define PSNS_REQ    0x80

#define PSNS_SELECTION  PSNS_SEL
#define PSNS_COMMAND    PSNS_CD
#define PSNS_DATAIN     PSNS_IO
#define PSNS_DATAOUT    0
#define PSNS_STATUS     (PSNS_CD  | PSNS_IO)
#define PSNS_MSGIN      (PSNS_MSG | PSNS_CD | PSNS_IO)
#define PSNS_MSGOUT     (PSNS_MSG | PSNS_CD)

#define INTS_ResetCondition     0x01
#define INTS_SPC_HardError      0x02
#define INTS_TimeOut            0x04
#define INTS_ServiceRequited    0x08
#define INTS_CommandComplete    0x10
#define INTS_Disconnected       0x20
#define INTS_ReSelected         0x40
#define INTS_Selected           0x80

#define CMD_BusRelease      0
#define CMD_Select          1
#define CMD_ResetATN        2
#define CMD_SetATN          3
#define CMD_Transfer        4
#define CMD_TransferPause   5
#define CMD_Reset_ACK_REQ   6
#define CMD_Set_ACK_REQ     7

struct MB89352 {
    Int32 debugHandle;
    int myId;                       // SPC SCSI ID 0..7
    int targetId;                   // SCSI Device target ID 0..7
    int regs[16];                   // SPC register
    int rst;                        // SCSI bus reset signal
    int atn;                        // SCSI bus attention signal
    SCSI_PHASE phase;               //
    SCSI_PHASE nextPhase;           // for message system
    int isEnabled;                  // spc enable flag
    int isBusy;                     // spc now working
    int isTransfer;                 // hardware transfer mode
    int msgin;                      // Message In flag
    int counter;                    // read and written number of bytes
                                    // within the range in the buffer
    int blockCounter;               // Number of blocks outside buffer
                                    // (512bytes / block)
    int tc;                         // counter for hardware transfer
    int devBusy;                    // CDROM busy (buffer conflict prevention)
    SCSIDEVICE* scsiDevice[8];      //
    UInt8* pCdb;                    // cdb pointer
    UInt8* pBuffer;                 // buffer pointer
    UInt8  cdb[12];                 // Command Descripter Block
    UInt8* buffer;                  // buffer for transfer
};

static FILE* scsiLog = NULL;

static void mb89352XferCb(MB89352* spc, int length)
{
    spc->devBusy = 0;
}

static void mb89352Disconnect(MB89352* spc)
{
    if (spc->phase != BusFree) {
        if ((spc->targetId >= 0) && (spc->targetId < 8)) {
            scsiDeviceDisconnect(TARGET);
        }
        spc->regs[REG_INTS] |= INTS_Disconnected;
        spc->phase      = BusFree;
        spc->nextPhase  = -1;
    }

    spc->regs[REG_PSNS] = 0;
    spc->isBusy         = 0;
    spc->isTransfer     = 0;
    spc->counter        = 0;
    spc->tc             = 0;
    spc->atn            = 0;

    SCSILOG("busfree()\n\n");
    scsiDeviceLogFlush();
}

static void mb89352SoftReset(MB89352* spc)
{
    int i;

    spc->isEnabled = 0;

    for (i = 2; i < 15; ++i) {
        spc->regs[i] = 0;
    }
    spc->regs[15] = 0xff;               // un mapped
    memset(spc->cdb, 0, 12);

    spc->pCdb    = spc->cdb;
    spc->pBuffer = spc->buffer;
    spc->phase   = BusFree;
    mb89352Disconnect(spc);
}

void mb89352Reset(MB89352* spc, int scsireset)
{
    int i;

    spc->regs[REG_BDID] = 0x80;     // Initial value
    spc->regs[REG_SCTL] = 0x80;
    spc->rst   = 0;
    spc->atn   = 0;
    spc->myId  = 7;

    mb89352SoftReset(spc);

    if (scsireset) {
        for (i = 0; i < 8; ++i) {
            scsiDeviceReset(spc->scsiDevice[i]);
        }
    }
}

static void mb89352SetACKREQ(MB89352* spc, UInt8* value)
{
    // REQ check
    if ((spc->regs[REG_PSNS] & (PSNS_REQ | PSNS_BSY)) != (PSNS_REQ | PSNS_BSY)) {
        SCSILOG("set ACK/REQ: REQ/BSY check error\n");
        if (spc->regs[REG_PSNS] & PSNS_IO) { // SCSI -> SPC
            *value = 0xff;
        }
        return;
    }

    // phase check
    if (spc->regs[FIX_PCTL] != (spc->regs[REG_PSNS] & 7)) {
        SCSILOG("set ACK/REQ: phase check error\n");
        if (spc->regs[REG_PSNS] & PSNS_IO) { // SCSI -> SPC
            *value = 0xff;
        }
        if (spc->isTransfer) {
            spc->regs[REG_INTS] |= INTS_ServiceRequited;
        }
        return;
    }

    switch (spc->phase) {

    // Transfer phase (data in)
    case DataIn:
        *value = *spc->pBuffer;
        ++spc->pBuffer;
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_DATAIN;
        break;

    //Transfer phase (data out)
    case DataOut:
        *spc->pBuffer = *value;
        ++spc->pBuffer;
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_DATAOUT;
        break;

    // Command phase
    case Command:
        if (spc->counter < 0) {
            //Initialize command routine
            spc->pCdb    = spc->cdb;
            spc->counter = cdbLength(*value);
        }
        *spc->pCdb = *value;
        ++spc->pCdb;
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_COMMAND;
        break;

    // Status phase
    case Status:
        *value = scsiDeviceGetStatusCode(TARGET);
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_STATUS;
        break;

    // Message In phase
    case MsgIn:
        *value = scsiDeviceMsgIn(TARGET);
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_MSGIN;
        break;

    // Message Out phase
    case MsgOut:
        spc->msgin |= scsiDeviceMsgOut(TARGET, *value);
        spc->regs[REG_PSNS] = PSNS_ACK | PSNS_BSY | PSNS_MSGOUT;
        break;

    default:
        SCSILOG("set ACK/REQ code error\n");
        break;
    }
} // end of mb89352SetACKREQ()

static void mb89352ResetACKREQ(MB89352* spc)
{
    // ACK check
    if ((spc->regs[REG_PSNS] & (PSNS_ACK | PSNS_BSY)) != (PSNS_ACK | PSNS_BSY)) {
        SCSILOG("reset ACK/REQ: ACK/BSY check error\n");
        return;
    }

    // phase check
    if (spc->regs[FIX_PCTL] != (spc->regs[REG_PSNS] & 7)) {
        SCSILOG("reset ACK/REQ: phase check error\n");
        if (spc->isTransfer) {
            spc->regs[REG_INTS] |= INTS_ServiceRequited;
        }
        return;
    }

    switch (spc->phase) {
    // Transfer phase (data in)
    case DataIn:
        if (--spc->counter > 0) {
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAIN;
        } else {
            if (spc->blockCounter > 0) {
                spc->counter = scsiDeviceDataIn(TARGET, &spc->blockCounter);
                if (spc->counter) {
                    spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAIN;
                    spc->pBuffer = spc->buffer;
                    break;
                }
            }
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_STATUS;
            spc->phase = Status;
        }
        break;

    // Transfer phase (data out)
    case DataOut:
        if (--spc->counter > 0) {
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAOUT;
        } else {
            spc->counter = scsiDeviceDataOut(TARGET, &spc->blockCounter);
            if (spc->counter) {
                spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAOUT;
                spc->pBuffer = spc->buffer;
                break;
            }
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_STATUS;
            spc->phase = Status;
        }
        break;

    // Command phase
    case Command:
        if (--spc->counter > 0) {
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_COMMAND;
        } else {
            spc->pBuffer = spc->buffer; // reset buffer pointer
            spc->devBusy = 1;
            spc->counter =
             scsiDeviceExecuteCmd(TARGET, spc->cdb, &spc->phase, &spc->blockCounter);

            switch (spc->phase) {
            case DataIn:
                spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAIN;
                break;
            case DataOut:
                spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAOUT;
                break;
            case Status:
                spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_STATUS;
                break;
            case Execute:
                spc->regs[REG_PSNS] = PSNS_BSY;
                return;
            default:
                SCSILOG("phase error\n");
                break;
            }
            spc->devBusy = 0;
        }
        break;

    // Status phase
    case Status:
        spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGIN;
        spc->phase = MsgIn;
        break;

    // Message In phase
    case MsgIn:
        if (spc->msgin <= 0) {
            mb89352Disconnect(spc);
            break;
        }
        spc->msgin = 0;
        // throw to MsgOut...

    // Message Out phase
    case MsgOut:
        if (spc->msgin == -1) {
            mb89352Disconnect(spc);
            return;
        }

        if (spc->atn) {
            if (spc->msgin & 2) {
                mb89352Disconnect(spc);
                return;
            }
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGOUT;
            return;
        }

        if (spc->msgin & 1) {
            spc->phase = MsgIn;
        } else {
            if (spc->msgin & 4) {
                spc->phase = Status;
                spc->nextPhase = -1;
            } else {
                spc->phase = spc->nextPhase;
                spc->nextPhase = -1;
            }
        }

        spc->msgin = 0;

        switch (spc->phase) {
        case Command:
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_COMMAND;
            break;
        case DataIn:
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAIN;
            break;
        case DataOut:
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAOUT;
            break;
        case Status:
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_STATUS;
            break;
        case MsgIn:
            spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGIN;
            break;
        default:
            SCSILOG("MsgOut code error\n");
            break;
        }
        return;

    default:
        //assert(false);
        SCSILOG("reset ACK/REQ code error\n");
        break;
    }

    if (spc->atn) {
        spc->nextPhase = spc->phase;
        spc->phase = MsgOut;
        spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGOUT;
    }
} // end of mb89352ResetACKREQ()

UInt8 mb89352ReadDREG(MB89352* spc)
{
    if (spc->isTransfer && (spc->tc > 0)) {
        mb89352SetACKREQ(spc, (UInt8*)&spc->regs[REG_DREG]);
        mb89352ResetACKREQ(spc);
        //SCSILOG2("DREG read: %d %x\n", spc->tc, spc->regs[REG_DREG]);

        --spc->tc;
        if (spc->tc == 0) {
            spc->isTransfer = 0;
            spc->regs[REG_INTS] |= INTS_CommandComplete;
        }
        spc->regs[REG_MBC] = (spc->regs[REG_MBC] - 1) & 0x0f;
        return spc->regs[REG_DREG];
    } else {
        return 0xff;
    }
}

void mb89352WriteDREG(MB89352* spc, UInt8 value)
{
#ifdef USE_DEBUGGER
    spc->regs[REG_DREG] = value;
#endif

    if (spc->isTransfer && (spc->tc > 0)) {
        //SCSILOG2("DREG write: %d %x\n", spc->tc, value);

        mb89352SetACKREQ(spc, &value);
        mb89352ResetACKREQ(spc);

        --spc->tc;
        if (spc->tc == 0) {
            spc->isTransfer = 0;
            spc->regs[REG_INTS] |= INTS_CommandComplete;
        }
        spc->regs[REG_MBC] = (spc->regs[REG_MBC] - 1) & 0x0f;
    }
}

void mb89352WriteRegister(MB89352* spc, UInt8 reg, UInt8 value)
{
    int err;
    int flag;
    int x;
    int i;
    int cmd;

    //SCSILOG2("SPC write register: %x %x\n", reg, value);

    switch (reg) {

    case REG_DREG:
        // write data Register
        mb89352WriteDREG(spc, value);
        break;

    case REG_SCMD:
        if (!spc->isEnabled) {
            break;
        }

        // bus reset
        if (value & 0x10) {
            if (((spc->regs[REG_SCMD] & 0x10) == 0) & (spc->regs[REG_SCTL] == 0)) {
                SCSILOG("SPC: bus reset\n");
                spc->rst = 1;
                spc->regs[REG_INTS] |= INTS_ResetCondition;
                for (i = 0; i < 8; ++i) {
                    scsiDeviceBusReset(spc->scsiDevice[i]);
                }
                mb89352Disconnect(spc);  // alternative routine
            }
        } else {
            spc->rst = 0;
        }

        spc->regs[REG_SCMD] = value;

        cmd = value >> 5;
        //SCSILOG1("SPC command: %x\n", cmd);

        // execute spc command
        switch (cmd) {
        case CMD_Set_ACK_REQ:
            switch (spc->phase) {
            case DataIn:
            case Status:
            case MsgIn:
                mb89352SetACKREQ(spc, (UInt8*)&spc->regs[REG_TEMP]);
                break;
            default:
                mb89352SetACKREQ(spc, (UInt8*)&spc->regs[REG_TEMPWR]);
            }
            break;

        case CMD_Reset_ACK_REQ:
            mb89352ResetACKREQ(spc);
            break;

        case CMD_Select:
            if (spc->rst) {
                spc->regs[REG_INTS] |= INTS_TimeOut;
                break;
            }

            if (spc->regs[REG_PCTL] & 1) {
                SCSILOG1("reselection error %x", spc->regs[REG_TEMPWR]);
                spc->regs[REG_INTS] |= INTS_TimeOut;
                mb89352Disconnect(spc);
                break;
            }

            x = spc->regs[REG_BDID] & spc->regs[REG_TEMPWR];
            if (spc->phase == BusFree && x && x != spc->regs[REG_TEMPWR]) {
                x = spc->regs[REG_TEMPWR] & ~spc->regs[REG_BDID];

                // the targetID is calculated.
                // It is given priority that the number is large.
                for (spc->targetId = 0; spc->targetId < 8; ++spc->targetId) {
                    x = x >> 1;
                    if (x == 0) {
                        break;
                    }
                }

                if (!spc->devBusy && scsiDeviceSelection(TARGET)) {
                    SCSILOG1("selection: %d OK\n", spc->targetId);
                    spc->regs[REG_INTS] |= INTS_CommandComplete;
                    spc->isBusy  =  1;
                    spc->msgin   =  0;
                    spc->counter = -1;
                    err          =  0;
                    if (spc->atn) {
                        spc->phase          = MsgOut;
                        spc->nextPhase      = Command;
                        spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGOUT;
                    } else {
                        spc->phase          = Command;
                        spc->nextPhase      = -1;
                        spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_COMMAND;
                    }
                } else {
                    err = 1;
                }
            } else {
                err = 1;
                #ifdef SCSIDEBUG
                spc->targetId = -1;
                #endif
            }

            if (err) {
                SCSILOG1("selection: %d error\n", spc->targetId);
                spc->regs[REG_INTS] |= INTS_TimeOut;
                mb89352Disconnect(spc);
            }
            break;

        // hardware transfer
        case CMD_Transfer:
            SCSILOG2("CMD_Transfer %d (%d)\n", spc->tc, spc->tc / 512);
            if ((spc->regs[FIX_PCTL] == (spc->regs[REG_PSNS] & 7)) &&
                (spc->regs[REG_PSNS] & (PSNS_REQ | PSNS_BSY))) {
                spc->isTransfer = 1;            // set Xfer in Progress
            } else {
                spc->regs[REG_INTS] |= INTS_ServiceRequited;
                SCSILOG("phase error\n");
            }
            break;

        case CMD_BusRelease:
            SCSILOG("CMD_BusRelease\n");
            mb89352Disconnect(spc);
            break;

        case CMD_SetATN:
            SCSILOG("CMD_SetATN\n");
            spc->atn = PSNS_ATN;
            break;

        case CMD_ResetATN:
            SCSILOG("CMD_ResetATN\n");
            spc->atn = 0;
            break;

        case CMD_TransferPause:
            // nothing is done in the initiator.
            SCSILOG("CMD_TransferPause\n");
            break;
        }
        break;  // end of REG_SCMD

    // Reset Interrupts
    case REG_INTS:
        spc->regs[REG_INTS] &= ~value;
        if (spc->rst) spc->regs[REG_INTS] |= INTS_ResetCondition;
        //SCSILOG2("INTS reset: %x %x\n", value, spc->regs[REG_INTS]);
        break;

    case REG_TEMP:
        spc->regs[REG_TEMPWR] = value;
        break;

    case REG_TCL:
        spc->tc = (spc->tc & 0x00ffff00) + value;
        //SCSILOG1("set tcl: %d\n", spc->tc);
        break;

    case REG_TCM:
        spc->tc = (spc->tc & 0x00ff00ff) + (value << 8);
        //SCSILOG1("set tcm: %d\n", spc->tc);
        break;

    case REG_TCH:
        spc->tc = (spc->tc & 0x0000ffff) + (value << 16);
        //SCSILOG1("set tch: %d\n", spc->tc);
        break;

    case REG_PCTL:
        spc->regs[REG_PCTL] = value;
        spc->regs[FIX_PCTL] = value & 7;
        break;

    case REG_BDID:
        // set Bus Device ID
        value &= 7;
        spc->myId = value;
        spc->regs[REG_BDID] = (1 << value);
        break;

    // Nothing
    case REG_SDGC:
    case REG_SSTS:
    case REG_SERR:
    case REG_MBC:
    case 15:
        break;

    case REG_SCTL:
        flag = (value & 0xe0) ? 0 : 1;
        if (flag != spc->isEnabled) {
            spc->isEnabled = flag;
            if (!flag) {
                mb89352SoftReset(spc);
            }
        }
        // throw to default
    default:
        spc->regs[reg] = value;
    }
}  // end of mb89352WriteRegister()

static UInt8 mb89352GetSSTS(MB89352* spc)
{
    UInt8 result = 1;               // set fifo empty
    if (spc->isTransfer) {
        if (spc->regs[REG_PSNS] & PSNS_IO) { // SCSI -> SPC transfer
            if (spc->tc >= 8) {
                result = 2;         // set fifo full
            } else {
                if (spc->tc != 0) {
                    result = 0;     // set fifo 1..7 bytes
                }
            }
        }
    }

    if (spc->phase != BusFree) {
        result |= 0x80;             // set iniciator
    }

    if (spc->isBusy) {
        result |= 0x20;             // set SPC_BSY
    }

    if ((spc->phase >= Command) || spc->isTransfer) {
        result |= 0x10;             // set Xfer in Progress
    }

    if (spc->rst) {
        result |= 0x08;             // set SCSI RST
    }

    if (spc->tc == 0) {
        result |= 0x04;             // set tc = 0
    }
    return result;
}

UInt8 mb89352ReadRegister(MB89352* spc, UInt8 reg)
{
    UInt8 result;

    switch (reg) {
    case REG_DREG:
        result = mb89352ReadDREG(spc);
        break;

    case REG_PSNS:
        if (spc->phase == Execute) {
            spc->counter =
                 scsiDeviceExecutingCmd(TARGET, &spc->phase, &spc->blockCounter);

            if (spc->atn && spc->phase != Execute) {
                spc->nextPhase = spc->phase;
                spc->phase = MsgOut;
                spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_MSGOUT;
            } else {
                switch (spc->phase) {
                case DataIn:
                    spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAIN;
                    break;
                case DataOut:
                    spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_DATAOUT;
                    break;
                case Status:
                    spc->regs[REG_PSNS] = PSNS_REQ | PSNS_BSY | PSNS_STATUS;
                    break;
                case Execute:
                    spc->regs[REG_PSNS] = PSNS_BSY;
                    break;
                default:
                    SCSILOG("phase error\n");
                    break;
                }
            }
        }

        result = (UInt8)((spc->regs[REG_PSNS] | spc->atn) & 0xff);
        break;

    case REG_SSTS:
        result = mb89352GetSSTS(spc);
        break;

    case REG_TCL:
        result = (UInt8)(spc->tc & 0xff);
        break;

    case REG_TCM:
        result = (UInt8)((spc->tc >>  8) & 0xff);
        break;

    case REG_TCH:
        result = (UInt8)((spc->tc >> 16) & 0xff);
        break;

    default:
        result = (UInt8)(spc->regs[reg] & 0xff);
    }

    //SCSILOG2("SPC reg read: %x %x\n", reg, result);

    return result;

} // end of mb89352ReadRegister()

UInt8 mb89352PeekRegister(MB89352* spc, UInt8 reg)
{
    switch (reg) {
    case REG_DREG:
        if (spc->isTransfer && (spc->tc > 0)) {
            return (UInt8)(spc->regs[REG_DREG] & 0xff);
        } else {
            return 0xff;
        }

    case REG_PSNS:
        return (UInt8)((spc->regs[REG_PSNS] | spc->atn) & 0xff);

    case REG_SSTS:
        return mb89352GetSSTS(spc);

    case REG_TCH:
        return (UInt8)((spc->tc >> 16) & 0xff);

    case REG_TCM:
        return (UInt8)((spc->tc >>  8) & 0xff);

    case REG_TCL:
        return (UInt8)(spc->tc & 0xff);

    default:
        return (UInt8)(spc->regs[reg] & 0xff);
    }
}

void mb89352SaveState(MB89352* spc)
{
    char tag[8];
    int i;

    SaveState* state = saveStateOpenForWrite("mb89352");
    SCSILOG("Save state\n");

    saveStateSet(state, "myId",         spc->myId);
    saveStateSet(state, "targetId",     spc->targetId);
    saveStateSet(state, "rst",          spc->rst);
    saveStateSet(state, "phase",        spc->phase);
    saveStateSet(state, "nextPhase",    spc->nextPhase);
    saveStateSet(state, "isEnabled",    spc->isEnabled);
    saveStateSet(state, "isBusy",       spc->isBusy);
    saveStateSet(state, "isTransfer",   spc->isTransfer);
    saveStateSet(state, "counter",      spc->counter);
    saveStateSet(state, "blockCounter", spc->blockCounter);
    saveStateSet(state, "tc",           spc->tc);
    saveStateSet(state, "msgin",        spc->msgin);
    saveStateSet(state, "pCdb",         spc->pCdb - spc->cdb);
    saveStateSet(state, "pBuffer",      spc->pBuffer - spc->buffer);

    spc->regs[REG_PSNS] |= spc->atn;

    for (i = 0; i < 16; i++) {
        sprintf(tag, "regs%d", i);
        saveStateSet(state, tag, spc->regs[i]);
    }

    saveStateSetBuffer(state, "cdb", spc->cdb, 12);
    saveStateSetBuffer(state, "buffer", spc->buffer, BUFFER_SIZE);

    saveStateClose(state);

    for (i = 0; i < 8; ++i) {
        scsiDeviceSaveState(spc->scsiDevice[i]);
    }
}

void mb89352LoadState(MB89352* spc)
{
    char tag[8];
    int i;

    SaveState* state = saveStateOpenForRead("mb89352");
    SCSILOG("Load state\n");

    spc->myId           = saveStateGet(state, "myId", 7);
    spc->targetId       = saveStateGet(state, "targetId", 0);
    spc->rst            = saveStateGet(state, "rst", 0);
    spc->phase          = saveStateGet(state, "phase", BusFree);
    spc->nextPhase      = saveStateGet(state, "nextPhase", BusFree);
    spc->isEnabled      = saveStateGet(state, "isEnabled", 0);
    spc->isBusy         = saveStateGet(state, "isBusy", 0);
    spc->isTransfer     = saveStateGet(state, "isTransfer", 0);
    spc->counter        = saveStateGet(state, "counter", 0);
    spc->blockCounter   = saveStateGet(state, "blockCounter", 0);
    spc->tc             = saveStateGet(state, "tc", 0);
    spc->msgin          = saveStateGet(state, "msgin", 0);
    spc->pCdb           = saveStateGet(state, "pCdb", 0) + spc->cdb;
    spc->pBuffer        = saveStateGet(state, "pBuffer", 0) + spc->buffer;

    for (i = 0; i < 16; ++i) {
        sprintf(tag, "regs%d", i);
        spc->regs[i] = saveStateGet(state, tag, 0);
    }
    spc->regs[FIX_PCTL] = spc->regs[REG_PCTL] & 7;
    spc->atn = spc->regs[REG_PSNS] & PSNS_ATN;

    saveStateGetBuffer(state, "cdb", spc->cdb, 12);
    saveStateGetBuffer(state, "buffer", spc->buffer, BUFFER_SIZE);

    saveStateClose(state);

    for (i = 0; i < 8; ++i) {
        scsiDeviceLoadState(spc->scsiDevice[i]);
    }
}

#ifdef USE_DEBUGGER
static void mb89352GetDebugInfo(MB89352* spc, DbgDevice* dbgDevice)
{
    DbgRegisterBank* regBank;
    char str[8];
    int i;

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegs(), 16 + 12);
    spc->regs[REG_SSTS]  = mb89352GetSSTS(spc);
    spc->regs[REG_PSNS] |= spc->atn;

    for (i = 0; i < 11; i++) {
        sprintf(str, "R%d", i);
        dbgRegisterBankAddRegister(regBank,  i, str, 8, spc->regs[i]);
    }

    dbgRegisterBankAddRegister(regBank, 11, "R11R", 8, spc->regs[REG_TEMP]);
    dbgRegisterBankAddRegister(regBank, 12, "R11W", 8, spc->regs[REG_TEMPWR]);
    dbgRegisterBankAddRegister(regBank, 13, "R12",  8, (spc->tc >> 16) & 0xff);
    dbgRegisterBankAddRegister(regBank, 14, "R13",  8, (spc->tc >>  8) & 0xff);
    dbgRegisterBankAddRegister(regBank, 15, "R14",  8, spc->tc & 0xff);

    for (i = 0; i < 12; i++) {
        sprintf(str, "CDB%d", i);
        dbgRegisterBankAddRegister(regBank,  i + 16, str, 8, spc->cdb[i]);
    }
}
#endif

void mb89352Destroy(MB89352* spc)
{
    int i;
    for (i = 0; i < 8; ++i) {
        scsiDeviceDestroy(spc->scsiDevice[i]);
    }

#ifdef USE_DEBUGGER
    debugDeviceUnregister(spc->debugHandle);
#endif

    archCdromBufferFree(spc->buffer);
    free(spc);
    SCSILOG("spc destroy\n");
    scsiDeviceLogClose();}

MB89352* mb89352Create(int hdId)
{
    int i;
    int diskId;
    MB89352* spc;
    int deviceType, scsiMode;

    scsiLog = scsiDeviceLogCreate();
    SCSILOG("spc create\n");
    spc = malloc(sizeof(MB89352));
    spc->buffer  = archCdromBufferMalloc(BUFFER_SIZE);
    spc->devBusy = 0;

#ifdef USE_DEBUGGER
    DebugCallbacks dbgCallbacks = { (void*)mb89352GetDebugInfo, NULL, NULL, NULL };
    spc->debugHandle =
        debugDeviceRegister(DBGTYPE_PORT, "MB89352 SPC", &dbgCallbacks, spc);
#endif

    // this SCSI creating parameter is for MEGA-SCSI
    for (i = 0; i < 8; i++) {
        diskId = diskGetHdDriveId(hdId, i);
        if (diskIsCdrom(diskId)) {
            deviceType = SDT_CDROM;
            scsiMode   = MODE_SCSI2 | MODE_UNITATTENTION | MODE_REMOVABLE;
        }
        else {
            deviceType = SDT_DirectAccess;
#if 1
            scsiMode   = MODE_SCSI2 | MODE_MEGASCSI | MODE_FDS120 | MODE_REMOVABLE;
#else
            scsiMode   = MODE_SCSI2 | MODE_MEGASCSI | MODE_CHECK2 | MODE_FDS120 | MODE_REMOVABLE;
#endif
        }
        spc->scsiDevice[i] =
            scsiDeviceCreate(i, diskId, spc->buffer, NULL, deviceType,
                             scsiMode, (CdromXferCompCb)mb89352XferCb, spc);
    }

    mb89352Reset(spc, 0);
    return spc;
}

