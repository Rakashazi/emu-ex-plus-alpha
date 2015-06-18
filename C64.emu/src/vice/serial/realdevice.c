/*
 * realdevice.c - Real device access.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Michael Klein <nip@c64.org>
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

#include "log.h"
#include "opencbmlib.h"
#include "realdevice.h"
#include "serial.h"
#include "types.h"
#include "vsync.h"


/*#define DEBUG_RD*/


static log_t realdevice_log = LOG_DEFAULT;

static unsigned int realdevice_available = 0;

static unsigned int realdevice_enabled = 0;

static CBM_FILE realdevice_fd;

static opencbmlib_t opencbmlib;


void realdevice_open(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_open)(realdevice_fd, (BYTE)(device & 0x0f),
                             (BYTE)(secondary & 0x0f),
                             NULL, 0);

#ifdef DEBUG_RD
    log_debug("OPEN DEVICE %i SECONDARY %i", device & 0x0f, secondary & 0x0f);
#endif
}

void realdevice_close(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_close)(realdevice_fd, (BYTE)(device & 0x0f),
                              (BYTE)(secondary & 0x0f));

#ifdef DEBUG_RD
    log_debug("CLOSE DEVICE %i SECONDARY %i", device & 0x0f, secondary & 0x0f);
#endif
}

void realdevice_listen(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_listen)(realdevice_fd, (BYTE)(device & 0x0f),
                               (BYTE)(secondary & 0x0f));
#ifdef DEBUG_RD
    log_debug("LISTEN DEVICE %i SECONDARY %i", device & 0x0f,
              secondary & 0x0f);
#endif
}

void realdevice_talk(unsigned int device, BYTE secondary, void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_talk)(realdevice_fd, (BYTE)(device & 0x0f),
                             (BYTE)(secondary & 0x0f));
#ifdef DEBUG_RD
    log_debug("TALK DEVICE %i SECONDARY %i", device & 0x0f,
              secondary & 0x0f);
#endif
}

void realdevice_unlisten(void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_unlisten)(realdevice_fd);
#ifdef DEBUG_RD
    log_debug("UNLISTEN");
#endif
}

void realdevice_untalk(void (*st_func)(BYTE))
{
    vsync_suspend_speed_eval();

    (*opencbmlib.p_cbm_untalk)(realdevice_fd);
#ifdef DEBUG_RD
    log_debug("UNTALK");
#endif
}

void realdevice_write(BYTE data, void (*st_func)(BYTE))
{
    BYTE st;

#ifdef DEBUG_RD
    BYTE mydata = data;
#endif

    vsync_suspend_speed_eval();

    st = ((*opencbmlib.p_cbm_raw_write)(realdevice_fd, &data, 1) == 1)
         ? 0 : 0x83;

#ifdef DEBUG_RD
    log_debug("WRITE DATA %02x ST %02x", mydata, st);
#endif

    st_func(st);
}

BYTE realdevice_read(void (*st_func)(BYTE))
{
    BYTE st, data;

    vsync_suspend_speed_eval();

    st = ((*opencbmlib.p_cbm_raw_read)(realdevice_fd, &data, 1) == 1) ? 0 : 2;

#ifdef DEBUG_RD
    log_debug("READ %02x ST %02x", data, st);
#endif

    if ((*opencbmlib.p_cbm_get_eoi)(realdevice_fd)) {
        st |= 0x40;
    }

#ifdef DEBUG_RD
    log_debug("READ NEWST %02x", st);
#endif

    st_func(st);

    return data;
}

void realdevice_init(void)
{
    realdevice_log = log_open("Real Device");
#ifdef DEBUG_RD
    log_debug("realdevice_init()");
#endif

    if (opencbmlib_open(&opencbmlib) >= 0) {
        realdevice_available = 1;
    }
}

void realdevice_reset(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice_reset()");
#endif
    if (realdevice_enabled) {
        (*opencbmlib.p_cbm_reset)(realdevice_fd);
    }
}

int realdevice_enable(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice_enable()");
#endif
    if (realdevice_available == 0 &&
            opencbmlib_open(&opencbmlib) >= 0) {
        realdevice_available = 1;
    }

    if (realdevice_available == 0) {
        log_message(realdevice_log, "Real device access is not available!");
        return -1;
    }

    if (realdevice_enabled == 0) {
#ifdef DEBUG_RD
        log_debug("realdevice_enable: calling cbm_driver_open");
#endif
        if ((*opencbmlib.p_cbm_driver_open)(&realdevice_fd, 0) != 0) {
            log_message(realdevice_log,
                        "Cannot open %s, realdevice not available!",
                        (*opencbmlib.p_cbm_get_driver_name)(0));
            return -1;
        }

        log_message(realdevice_log, "%s opened.",
                    (*opencbmlib.p_cbm_get_driver_name)(0));
    }

#ifdef DEBUG_RD
    log_debug("realdevice_enable: realdevice_enabled++");
#endif
    realdevice_enabled++;

    return 0;
}

void realdevice_disable(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice_disable()");
#endif
    if (realdevice_enabled > 0) {
        realdevice_enabled--;

        if (realdevice_enabled == 0) {
            (*opencbmlib.p_cbm_driver_close)(realdevice_fd);

            log_message(realdevice_log, "%s closed.",
                        (*opencbmlib.p_cbm_get_driver_name)(0));

            opencbmlib_close();
            realdevice_available = 0;
        }
    }
}
