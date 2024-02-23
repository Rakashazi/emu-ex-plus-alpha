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

#ifdef HAVE_REALDEVICE

#include <stdio.h>

#include "opencbmlib.h"

#include "assert.h"
#include "log.h"
#include "realdevice.h"
#include "serial.h"
#include "types.h"
#include "vsync.h"

/* #define DEBUG_RD */


static log_t realdevice_log = LOG_DEFAULT;

static unsigned int realdevice_available = 0;

static unsigned int realdevice_enabled = 0;

static CBM_FILE realdevice_fd;

static opencbmlib_t opencbmlib;


void realdevice_open(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: OPEN FD:%p DEVICE:%u SECONDARY:%i",
        (void*)realdevice_fd, device & 0x0f, secondary & 0x0f);
#endif
    assert(opencbmlib.p_cbm_open != NULL);
    (*opencbmlib.p_cbm_open)(realdevice_fd, (uint8_t)(device & 0x0f),
                             (uint8_t)(secondary & 0x0f),
                             NULL, 0);
}

void realdevice_close(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: CLOSE FD:%p DEVICE:%u SECONDARY:%i",
        (void*)realdevice_fd, device & 0x0f, secondary & 0x0f);
#endif
    assert(opencbmlib.p_cbm_close != NULL);
    assert(realdevice_fd != NULL);
    (*opencbmlib.p_cbm_close)(realdevice_fd, (uint8_t)(device & 0x0f),
                              (uint8_t)(secondary & 0x0f));
}

void realdevice_listen(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: LISTEN FD:%p DEVICE:%u SECONDARY:%i",
        (void*)realdevice_fd, device & 0x0f, secondary & 0x0f);
#endif
    assert(opencbmlib.p_cbm_listen != NULL);
    assert(realdevice_fd != NULL);
    (*opencbmlib.p_cbm_listen)(realdevice_fd, (uint8_t)(device & 0x0f),
                               (uint8_t)(secondary & 0x0f));
}

void realdevice_talk(unsigned int device, uint8_t secondary, void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: TALK FD:%p DEVICE:%u SECONDARY:%i",
        (void*)realdevice_fd, device & 0x0f, secondary & 0x0f);
#endif
    assert(opencbmlib.p_cbm_talk != NULL);
    assert(realdevice_fd != NULL);
    (*opencbmlib.p_cbm_talk)(realdevice_fd, (uint8_t)(device & 0x0f),
                             (uint8_t)(secondary & 0x0f));
}

void realdevice_unlisten(void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: UNLISTEN FD:%p", (void*)realdevice_fd);
#endif
    assert(opencbmlib.p_cbm_unlisten != NULL);
    assert(realdevice_fd != NULL);
    (*opencbmlib.p_cbm_unlisten)(realdevice_fd);
}

void realdevice_untalk(void (*st_func)(uint8_t))
{
    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: UNTALK FD:%p", (void*)realdevice_fd);
#endif
    assert(opencbmlib.p_cbm_untalk != NULL);
    assert(realdevice_fd != NULL);
    (*opencbmlib.p_cbm_untalk)(realdevice_fd);
}

void realdevice_write(uint8_t data, void (*st_func)(uint8_t))
{
    uint8_t st;

#ifdef DEBUG_RD
    uint8_t mydata = data;
#endif

    vsync_suspend_speed_eval();

#ifdef DEBUG_RD
    log_debug("realdevice: WRITE FD:%p DATA:%02x", (void*)realdevice_fd, mydata);
#endif
    assert(opencbmlib.p_cbm_raw_write != NULL);
    assert(realdevice_fd != NULL);
    st = ((*opencbmlib.p_cbm_raw_write)(realdevice_fd, &data, 1) == 1)
         ? 0 : 0x83;

#ifdef DEBUG_RD
    log_debug("realdevice: WRITE FD:%p ST:%02x", (void*)realdevice_fd, st);
#endif

    st_func(st);
}

uint8_t realdevice_read(void (*st_func)(uint8_t))
{
    uint8_t st, data;

    vsync_suspend_speed_eval();

    assert(opencbmlib.p_cbm_raw_read != NULL);
    assert(realdevice_fd != NULL);
    st = ((*opencbmlib.p_cbm_raw_read)(realdevice_fd, &data, 1) == 1) ? 0 : 2;

#ifdef DEBUG_RD
    log_debug("realdevice: READ FD:%p DATA:%02x", (void*)realdevice_fd, data);
    log_debug("realdevice: READ FD:%p ST:%02x", (void*)realdevice_fd, st);
#endif

    assert(realdevice_fd != NULL);
    assert(opencbmlib.p_cbm_get_eoi != NULL);
    if ((*opencbmlib.p_cbm_get_eoi)(realdevice_fd)) {
        st |= 0x40;
    }

#ifdef DEBUG_RD
    log_debug("realdevice: READ FD:%p ST after EOI:%02x", (void*)realdevice_fd, st);
#endif

    st_func(st);

    return data;
}

void realdevice_init(void)
{
    realdevice_log = log_open("Real Device");
#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_init()");
#endif

    if (opencbmlib_open(&opencbmlib) >= 0) {
        realdevice_available = 1;
    }
#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_available: %u", realdevice_available);
#endif
}

void realdevice_reset(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_reset() FD:%p", (void*)realdevice_fd);
#endif
    if (realdevice_enabled) {
        assert(realdevice_fd != NULL);
        assert(opencbmlib.p_cbm_reset != NULL);
        (*opencbmlib.p_cbm_reset)(realdevice_fd);
    }
}

int realdevice_enable(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_enable() realdevice_available before: %u", realdevice_available);
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
        log_debug("realdevice: realdevice_enable: calling cbm_driver_open");
#endif
        assert(opencbmlib.p_cbm_driver_open != NULL);
        if ((*opencbmlib.p_cbm_driver_open)(&realdevice_fd, 0) != 0) {
            log_message(realdevice_log,
                        "Cannot open %s, realdevice not available!",
                        (*opencbmlib.p_cbm_get_driver_name)(0));
            return -1;
        }

        assert(realdevice_fd != NULL);
        assert(opencbmlib.p_cbm_get_driver_name != NULL);
        log_message(realdevice_log, "%s opened FD:%p.",
                    (*opencbmlib.p_cbm_get_driver_name)(0), (void*)realdevice_fd);
    }

#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_enable: realdevice_enabled++");
#endif
    realdevice_enabled++;

    return 0;
}

void realdevice_disable(void)
{
#ifdef DEBUG_RD
    log_debug("realdevice: realdevice_disable() realdevice_enabled: %u", realdevice_enabled);
#endif
    if (realdevice_enabled > 0) {
        realdevice_enabled--;

        if (realdevice_enabled == 0) {
            assert(realdevice_fd != NULL);
            assert(opencbmlib.p_cbm_driver_close != NULL);
            (*opencbmlib.p_cbm_driver_close)(realdevice_fd);

            assert(realdevice_fd != NULL);
            assert(opencbmlib.p_cbm_get_driver_name != NULL);
            log_message(realdevice_log, "%s FD:%p closed.",
                        (*opencbmlib.p_cbm_get_driver_name)(0), (void*)realdevice_fd);

            opencbmlib_close();
            realdevice_available = 0;
        }
    }
}
#endif
