/*
 * fsdrive.c - Filesystem based serial emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#include "vice.h"

#include <stdio.h>

#include "attach.h"
#include "fsdrive.h"
#include "log.h"
#include "serial.h"
#include "types.h"

#define SERIAL_NAMELENGTH 255

static log_t fsdrive_log = LOG_ERR;

static BYTE SerialBuffer[SERIAL_NAMELENGTH + 1];
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

/* Handle Serial Bus Commands under Attention.  */
static BYTE serialcommand(unsigned int device, BYTE secondary)
{
    serial_t *p;
    void *vdrive;
    int channel;
    int i;
    BYTE st = 0;

    /*
     * which device ?
     */
    p = serial_device_get(device & 0x0f);
    channel = secondary & 0x0f;

    if ((device & 0x0f) >= 8) {
        vdrive = (void *)file_system_get_vdrive(device & 0x0f);
    } else {
        vdrive = NULL;
    }

    /* if command on a channel, reset output buffer... */
    if ((secondary & 0xf0) != 0x60) {
        p->nextok[channel] = 0;
    }
    switch (secondary & 0xf0) {
        /*
         * Open Channel
         */
        case 0x60:
            if (p->isopen[channel] == 1) {
                p->isopen[channel] = 2;
                st = (BYTE)((*(p->openf))(vdrive, NULL, 0, channel, NULL));
                for (i = 0; i < SerialPtr; i++) {
                    (*(p->putf))(vdrive, ((BYTE)(SerialBuffer[i])), channel);
                }
                SerialPtr = 0;
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }
            break;

        /*
         * Close File
         */
        case 0xE0:
            p->isopen[channel] = 0;
            st = (BYTE)((*(p->closef))(vdrive, channel));
            break;

        /*
         * Open File
         */
        case 0xF0:
            if (p->isopen[channel]) {
#ifndef DELAYEDCLOSE
                if (p->isopen[channel] == 2) {
                    log_warning(fsdrive_log, "Bogus close?");
                    (*(p->closef))(vdrive, channel);
                }
                p->isopen[channel] = 2;
                SerialBuffer[SerialPtr] = 0;
                st = (BYTE)((*(p->openf))(vdrive, SerialBuffer, SerialPtr,
                                          channel, NULL));
                SerialPtr = 0;

                if (st) {
                    p->isopen[channel] = 0;
                    (*(p->closef))(vdrive, channel);

                    log_error(fsdrive_log, "Cannot open file. Status $%02x.", st);
                }
#else
                if (SerialPtr != 0 || channel == 0x0f) {
                    (*(p->closef))(vdrive, channel);
                    p->isopen[channel] = 2;
                    SerialBuffer[SerialPtr] = 0;
                    st = (BYTE)((*(p->openf))(vdrive, SerialBuffer, SerialPtr,
                                              channel, NULL));
                    SerialPtr = 0;
                    if (st) {
                        p->isopen[channel] = 0;
                        (*(p->closef))(vdrive, channel);

                        log_error(fsdrive_log, "Cannot open file. Status $%02x.", st);
                    }
                }
#endif
            }
            if (p->flushf) {
                (*(p->flushf))(vdrive, channel);
            }
            break;

        default:
            log_error(fsdrive_log, "Unknown command %02X.", secondary & 0xff);
    }

    return st;
}

/* ------------------------------------------------------------------------- */

void fsdrive_open(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    serial_t *p;
#ifndef DELAYEDCLOSE
    void *vdrive;
#endif

    p = serial_device_get(device & 0x0f);
#ifndef DELAYEDCLOSE
    if (p->isopen[secondary & 0x0f] == 2) {
        if ((device & 0x0f) >= 8) {
            vdrive = (void *)file_system_get_vdrive(device & 0x0f);
        } else {
            vdrive = NULL;
        }
        (*(p->closef))(vdrive, secondary & 0x0f);
    }
#endif
    p->isopen[secondary & 0x0f] = 1;
}

void fsdrive_close(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    BYTE st;

    st = serialcommand(device, secondary);
    st_func(st);
}

void fsdrive_listentalk(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    BYTE st;
    serial_t *p;
    void *vdrive;

    st = serialcommand(device, secondary);
    st_func(st);

    p = serial_device_get(device & 0x0f);
    if (p->listenf) {
        /* send listen/talk to emulated devices for flushing of
           REL file write buffer. */
        if ((device & 0x0f) >= 8) {
            vdrive = (void *)file_system_get_vdrive(device & 0x0f);
            (*(p->listenf))(vdrive, secondary & 0x0f);
        }
    }
}

void fsdrive_unlisten(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    BYTE st;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);
    if ((secondary & 0xf0) == 0xf0
        || (secondary & 0x0f) == 0x0f) {
        st = serialcommand(device, secondary);
        st_func(st);
        /* Flush serial read ahead buffer too.  */
        p->nextok[secondary & 0x0f] = 0;
    } else if (p->listenf) {
        /* send unlisten to emulated devices for flushing of
           REL file write buffer. */
        if ((device & 0x0f) >= 8) {
            vdrive = (void *)file_system_get_vdrive(device & 0x0f);
            (*(p->listenf))(vdrive, secondary & 0x0f);
        }
    }
}

void fsdrive_untalk(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
}

void fsdrive_write(unsigned int device, BYTE secondary, BYTE data, void (*st_func)(BYTE))
{
    BYTE st;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);

    if ((device & 0x0f) >= 8) {
        vdrive = (void *)file_system_get_vdrive(device & 0x0f);
    } else {
        vdrive = NULL;
    }

    if (p->inuse) {
        if (p->isopen[secondary & 0x0f] == 1) {
            /* Store name here */
            if (SerialPtr < SERIAL_NAMELENGTH) {
                SerialBuffer[SerialPtr++] = data;
            }
        } else {
            /* Send to device */
            st = (*(p->putf))(vdrive, data, (int)(secondary & 0x0f));
            st_func(st);
        }
    } else {                    /* Not present */
        st_func(0x83);
    }
}

BYTE fsdrive_read(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    int st = 0, secadr = secondary & 0x0f;
    BYTE data;
    serial_t *p;
    void *vdrive;

    p = serial_device_get(device & 0x0f);

    if ((device & 0x0f) >= 8) {
        vdrive = (void *)file_system_get_vdrive(device & 0x0f);
    } else {
        vdrive = NULL;
    }

#if 0
    /* Get next byte if necessary.  */
    if (!(p->nextok[secadr]))
#endif
    st = (*(p->getf))(vdrive, &(p->nextbyte[secadr]), secadr);

    /* Move byte from buffer to output.  */
    data = p->nextbyte[secadr];
    p->nextok[secadr] = 0;
#if 0
    /* Fill buffer again.  */
    if (!st) {
        st = (*(p->getf))(vdrive, &(p->nextbyte[secadr]), secadr);
        if (!st) {
            p->nextok[secadr] = 1;
        }
    }
#endif
    st_func((BYTE)st);

    return data;
}

void fsdrive_reset(void)
{
    unsigned int i, j;
    serial_t *p;
    void *vdrive;

    for (i = 0; i < SERIAL_MAXDEVICES; i++) {
        p = serial_device_get(i);
        if (p->inuse) {
            for (j = 0; j < 16; j++) {
                if (p->isopen[j]) {
                    vdrive = (void *)file_system_get_vdrive(i);
                    p->isopen[j] = 0;
                    (*(p->closef))(vdrive, j);
                }
            }
        }
    }
}

void fsdrive_init(void)
{
    fsdrive_log = log_open("FSDrive");
}
