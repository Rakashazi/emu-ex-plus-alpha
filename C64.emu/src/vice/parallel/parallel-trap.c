/*
 * parallel-trap.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

/* #define DEBUG_PARALLEL_TRAP */

#include "vice.h"

#include <stdio.h>

#include "attach.h"
#include "drive.h"
#include "drivetypes.h"
#include "log.h"
#include "parallel-trap.h"
#include "parallel.h"
#include "serial.h"
#include "types.h"

#ifdef DEBUG_PARALLEL_TRAP
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

#define SERIAL_NAMELENGTH 255


/* On which channel did listen happen to?  */
static uint8_t TrapDevice;
static uint8_t TrapSecondary;

/* Function to call when EOF happens in `serialreceivebyte()'.  */
static void (*eof_callback_func)(void);

/* Function to call when the `serialattention()' trap is called.  */
static void (*attention_callback_func)(void);

/* Logging goes here.  */
static log_t parallel_log = LOG_DEFAULT;

static uint8_t SerialBuffer[SERIAL_NAMELENGTH + 1];
static int SerialPtr;

/*
   On a real system an opened channel is affected only after having
   received and parsed the complete next open command.
   This patch tries to emulate this behavior and allows to keep the
   status channel continuously open while opening and closing other
   files on the same device.

   FIXME: test for regressions and either revert to, or remove old code
*/
#define DELAYEDCLOSE

static int parallelcommand(void)
{
    serial_t *p;
    uint8_t b;
    int channel;
    int i, st = 0;
    void *vdrive;
    unsigned int dnr;

    dnr = TrapDevice & 0x0f;
    if (dnr >= DRIVE_UNIT_MIN &&
        dnr < DRIVE_UNIT_MIN+NUM_DISK_UNITS &&
        diskunit_context[dnr - DRIVE_UNIT_MIN]->enable) {
            return PAR_STATUS_DEVICE_NOT_PRESENT +    /* device not present */
                   PAR_STATUS_TIME_OUT_ON_WRITE +
                   PAR_STATUS_TIME_OUT_ON_READ;
    }

    /* which device ? */
    p = serial_device_get(TrapDevice & 0x0f);
    vdrive = (void *)file_system_get_vdrive(TrapDevice & 0x0f);
    channel = TrapSecondary & 0x0f;

    /* if command on a channel, reset output buffer... */
    if ((TrapSecondary & 0xf0) != 0x60) {
        p->nextok[channel] = 0;
        p->lastok[channel] = 0;
    }
    switch (TrapSecondary & 0xf0) {
        case 0x60: /* Secondary address */
            /* Open Channel */
            if (p->isopen[channel] == ISOPEN_CLOSED) {
                p->isopen[channel] = ISOPEN_OPEN;
                st = (*(p->openf))(vdrive, NULL, 0, channel, NULL);
                for (i = 0; i < SerialPtr; i++) {
                    (*(p->putf))(vdrive, SerialBuffer[i], channel);
                }
                SerialPtr = 0;
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }

            if ((!st) && ((TrapDevice & 0xf0) == 0x40)) { /* isTalking */
                /* any error, except eof */
                st = parallel_trap_receivebyte(&b, 1) & 0xbf;
            }
            break;
        case 0xE0:
            /* Close File */
            p->isopen[channel] = ISOPEN_CLOSED;
            st = (*(p->closef))(vdrive, channel);
            break;
        case 0xF0:
            /* Open File */
            if (p->isopen[channel] != ISOPEN_CLOSED) {
#ifndef DELAYEDCLOSE
                if (p->isopen[channel] == ISOPEN_OPEN) {
                    log_warning(parallel_log, "Bogus close?");
                    (*(p->closef))(vdrive, channel);
                }
                p->isopen[channel] = ISOPEN_OPEN;
                SerialBuffer[SerialPtr] = 0;
                st = (*(p->openf))(vdrive, SerialBuffer, SerialPtr, channel, NULL);
                SerialPtr = 0;

                if (st) {
                    p->isopen[channel] = ISOPEN_CLOSED;
                    (*(p->closef))(vdrive, channel);
                    log_error(parallel_log, "Cannot open file. Status $%02x.", st);
                }
#else /* DELAYEDCLOSE */
                if (SerialPtr != 0 || channel == 0x0f) {
                    (*(p->closef))(vdrive, channel);
                    p->isopen[channel] = ISOPEN_OPEN;

                    SerialBuffer[SerialPtr] = 0;
                    st = (*(p->openf))(vdrive, SerialBuffer, SerialPtr, channel, NULL);
                    SerialPtr = 0;

                    if (st) {
                        p->isopen[channel] = ISOPEN_CLOSED;
                        (*(p->closef))(vdrive, channel);
                        log_error(parallel_log,
                                "Cannot open file. Status $%02x.",
                                (unsigned int)st);
                    }
                }
#endif /* DELAYEDCLOSE */
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }
            break;
        default:
            log_error(parallel_log, "Unknown command %02X.", TrapSecondary & 0xff);
    }
    return st;
}

int parallel_trap_attention(int b)
{
    int st = 0;
    serial_t *p;
    void *vdrive;

#ifdef DEBUG
    if (debug.ieee) {
        log_message(parallel_log, "ParallelAttention(%02x).", (unsigned int)b);
    }
#endif
    DBG(("parallel_trap_attention(%d)", b));

    if (b == 0x3f                                       /* unlisten */
        && (((TrapSecondary & 0xf0) == 0xf0)            /* open filename */
            || ((TrapSecondary & 0x0f) == 0x0f))) {     /* secaddr #15 */
        st = parallelcommand();
    } else {
        switch (b & 0xf0) {
            case 0x20:          /* Listen + device */
            case 0x40:          /* Talk + device */
                /* If this device is already emulated with TDE, don't
                 * try to react to it here. */
                {
                    int dnr = b & 0x0f;
                    if (dnr >= DRIVE_UNIT_MIN &&
                        dnr < DRIVE_UNIT_MIN+NUM_DISK_UNITS &&
                        diskunit_context[dnr - DRIVE_UNIT_MIN]->enable) {
                    } else {
                        TrapDevice = b;
                    }
                }
                break;

            case 0x60:          /* Secondary address */
            case 0xe0:          /* Close a file */
                if (TrapDevice != 0) {
                    TrapSecondary = b;
                    st |= parallelcommand();
                }
                break;

            case 0xf0:          /* Open File; needs the filename first */
                if (TrapDevice != 0) {
                    TrapSecondary = b;
                    p = serial_device_get(TrapDevice & 0x0f);
#ifndef DELAYEDCLOSE
                    vdrive = (void *)file_system_get_vdrive(TrapDevice & 0x0f);
                    if (p->isopen[b & 0x0f] == ISOPEN_OPEN) {
                        (*(p->closef))(vdrive, b & 0x0f);
                    }
#endif
                    p->isopen[b & 0x0f] = ISOPEN_AWAITING_NAME;
                }
                break;
        }
    }

    if (TrapDevice != 0) {
        p = serial_device_get(TrapDevice & 0x0f);
        if (!(p->inuse)) {
            st |= PAR_STATUS_DEVICE_NOT_PRESENT;
        }

        /* If it was a listen or talk or secondary addr or unlisten */
        if (((b & 0xf0) == 0x20) || ((b & 0xf0) == 0x40) || ((b & 0xf0) == 0x60)
            || (b == 0x3f)) {
            if (p->listenf) {
                /* send talk/listen/unlisten to emulated devices for
                    flushing of REL file write buffer. */
                if ((TrapDevice & 0x0f) >= DRIVE_UNIT_MIN) {
                    vdrive = (void *)file_system_get_vdrive(TrapDevice & 0x0f);
                    (*(p->listenf))(vdrive, TrapSecondary & 0x0f);
                }
            }
        }
    }

    if ((b == 0x3f) || (b == 0x5f)) {
        TrapDevice = 0;
        TrapSecondary = 0;
    }

    st |= TrapDevice << 8;

    if (attention_callback_func) {
        attention_callback_func();
    }

    return st;
}

int parallel_trap_sendbyte(uint8_t data)
{
    int st = 0;
    serial_t *p;
    void *vdrive;
    unsigned int dnr;

    DBG(("parallel_trap_sendbyte(%02x)", data));

    dnr = TrapDevice & 0x0f;
    if (dnr >= DRIVE_UNIT_MIN &&
        dnr < DRIVE_UNIT_MIN+NUM_DISK_UNITS &&
        diskunit_context[dnr - DRIVE_UNIT_MIN]->enable) {
            return PAR_STATUS_DEVICE_NOT_PRESENT +    /* device not present */
                   PAR_STATUS_TIME_OUT_ON_WRITE +
                   PAR_STATUS_TIME_OUT_ON_READ;
    }

    p = serial_device_get(TrapDevice & 0x0f);
    vdrive = (void *)file_system_get_vdrive(TrapDevice & 0x0f);

    if (p->inuse) {
        if (p->isopen[TrapSecondary & 0x0f] == ISOPEN_AWAITING_NAME) {
#ifdef DEBUG
            if (debug.ieee) {
                log_message(parallel_log,
                            "SerialSendByte[%2d] = %02x.", SerialPtr, data);
            }
#endif
            /* Store name here */
            if (SerialPtr < SERIAL_NAMELENGTH) {
                SerialBuffer[SerialPtr++] = data;
            }
        } else {
            /* Send to device */
            st = (*(p->putf))(vdrive, data, TrapSecondary & 0x0f);
        }
    } else {                    /* Not present */
        st = PAR_STATUS_DEVICE_NOT_PRESENT +
             PAR_STATUS_TIME_OUT_ON_WRITE +
             PAR_STATUS_TIME_OUT_ON_READ;
    }

    return st + (TrapDevice << 8);
}

int parallel_trap_receivebyte(uint8_t *data, int fake)
{
    int st = 0;
    int secadr = TrapSecondary & 0x0f;
    serial_t *p;
    void *vdrive;
    unsigned int dnr;

    DBG(("parallel_trap_receivebyte"));

    dnr = TrapDevice & 0x0f;
    if (dnr >= DRIVE_UNIT_MIN &&
        dnr < DRIVE_UNIT_MIN+NUM_DISK_UNITS &&
        diskunit_context[dnr - DRIVE_UNIT_MIN]->enable) {
            return 0x83;    /* device not present */
    }

    p = serial_device_get(TrapDevice & 0x0f);
    vdrive = (void *)file_system_get_vdrive(TrapDevice & 0x0f);

    /* first fill up buffers */
#if 0
    if (!p->lastok[secadr]) {
        p->lastok[secadr] = p->nextok[secadr];
        p->lastbyte[secadr] = p->nextbyte[secadr];
        p->lastst[secadr] = p->nextst[secadr];
        p->nextok[secadr] = 0;
#endif
    if (!p->lastok[secadr]) {
        p->lastst[secadr] =
            (*(p->getf))(vdrive, &(p->lastbyte[secadr]), secadr);
        p->lastok[secadr] = 1;
    }
#if 0
}
if ((!p->nextok[secadr]) && (!p->lastst[secadr])) {
    p->nextst[secadr] =
        (*(p->getf))(vdrive, &(p->nextbyte[secadr]), secadr);
    p->nextok[secadr] = 1;
}
#endif
    *data = p->lastbyte[secadr];
    if (!fake) {
        p->lastok[secadr] = 0;
    }
#if 0
    st = p->nextok[secadr] ? p->nextst[secadr] :
         (p->lastok[secadr] ? p->lastst[secadr] : 2);
#endif
    st = p->lastst[secadr]; /* added */
    st += TrapDevice << 8;

#ifdef DEBUG
    if (debug.ieee) {
        log_message(parallel_log,
                    "receive: sa=%02x lastb = %02x (data=%02x), "
                    "ok=%s, st=%04x, nextb = %02x, "
                    "ok=%s, st=%04x.",
                    (unsigned int)secadr,
                    p->lastbyte[secadr],
                    (int)*data,
                    p->lastok[secadr] ? "ok" : "no",
                    (unsigned int)(p->lastst[secadr]),
                    p->nextbyte[secadr],
                    p->nextok[secadr] ? "ok" : "no",
                    (unsigned int)(p->nextst[secadr]));
    }
#endif
#if 0
    if ((!fake) && p->nextok[secadr] && p->nextst[secadr]) {
        p->nextok[secadr] = 0;
    }
#endif
    if ((st & 0x40) && eof_callback_func != NULL) {
        eof_callback_func();
    }
    return st;
}

/* Specify a function to call when EOF happens in `serialreceivebyte()'.  */
void parallel_trap_eof_callback_set(void (*func)(void))
{
    eof_callback_func = func;
}

/* Specify a function to call when the `serialattention()' trap is called.  */
void parallel_trap_attention_callback_set(void (*func)(void))
{
    attention_callback_func = func;
}
