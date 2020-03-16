/*
 * cs8900io.c - CS8900 I/O for TFE and RRNET (clockport) carts.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 *  Christian Vogelgsang <chris@vogelgsang.org>
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

#ifdef HAVE_RAWNET

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "cs8900.h"
#include "crc32.h"
#include "export.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "monitor.h"
#include "rawnet.h"
#include "resources.h"
#include "snapshot.h"
#include "uiapi.h"
#include "util.h"

#include "cs8900io.h"

/* #define CS8900IO_DEBUG */

#ifdef CS8900IO_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/*
 This variable is used when we need to postpone the initialization
 because cs8900io_init() is not yet called
*/
static int should_activate = 0;

static log_t cs8900io_log = LOG_ERR;

/* Flag: Can we even use CS8900 I/O, or is the hardware not available? */
static int cs8900io_cannot_use = 0;

/* Flag: Do we have the CS8900 I/O enabled?  */
static int cs8900io_enabled = 0;
static const char *cs8900io_owner = NULL;

static char *cs8900io_interface = NULL;

static int cs8900io_init_done = 0;
static int cs8900io_resources_init_done = 0;
static int cs8900io_cmdline_init_done = 0;


/** \brief  Keep track of default IF used as a factory value
 *
 */
static char *default_if;


/* ------------------------------------------------------------------------- */
/*    initialization and deinitialization functions                          */

void cs8900io_reset(void)
{
    if (cs8900io_enabled && !should_activate) {
        cs8900_reset();
    }
}

static int cs8900io_activate(void)
{
#ifdef CS8900IO_DEBUG
    log_message(cs8900io_log, "cs8900io_activate().");
#endif

    if (cs8900io_log != LOG_ERR) {
        switch (cs8900_activate(cs8900io_interface)) {
            case -1:
                cs8900io_enabled = 0;
                break;
            case -2:
                cs8900io_enabled = 0;
                cs8900io_cannot_use = 1;
                ui_error("Failed to initialize PCAP library, cannot use"
                       " ethernet based devices.\n\n"
                       "On Windows make sure the pcap DLL is installed,"
                       " on Unix make sure to run VICE as root, on MacOS"
                       " you're on your own.");
                return -1;
        }
    } else {
        should_activate = 1;
    }
    return 0;
}

static int cs8900io_deactivate(void)
{
#ifdef CS8900IO_DEBUG
    log_message(cs8900io_log, "cs8900io_deactivate().");
#endif

    if (should_activate) {
        should_activate = 0;
    } else {
        cs8900io_enabled = 0;
        should_activate = 0;
        /* FIXME: WTH check for cs8900io_log here? */
        if (cs8900io_log != LOG_ERR) {
            return cs8900_deactivate();
        }
    }

    return 0;
}

void cs8900io_init(void)
{
    if (!cs8900io_init_done) {
        cs8900io_log = log_open("CS8900 I/O");

        rawnet_set_should_accept_func(cs8900_should_accept);
        if (cs8900_init() < 0) {
            cs8900io_enabled = 0;
            cs8900io_cannot_use = 1;
        }

        if (should_activate) {
            should_activate = 0;
            if (cs8900io_activate() < 0) {
                cs8900io_enabled = 0;
                cs8900io_cannot_use = 1;
            }
        }
        cs8900io_init_done = 1;
    }
}

void cs8900io_detach(void)
{
#ifdef CS8900IO_DEBUG
    log_message(cs8900io_log, "cs8900io_shutdown().");
#endif

    if (cs8900io_enabled) {
        cs8900_shutdown();
        cs8900io_enabled = 0;
        should_activate = 0;
#ifdef CS8900IO_DEBUG
        log_message(cs8900io_log, "...2");
#endif
    }

#ifdef CS8900IO_DEBUG
    log_message(cs8900io_log, "cs8900io_shutdown() done.");
#endif
}

/* ------------------------------------------------------------------------- */

/* ----- read byte from I/O range in VICE ----- */
uint8_t cs8900io_read(uint16_t io_address)
{
    if (!cs8900io_cannot_use) {
        return cs8900_read(io_address);
    }
    return 0;
}

/* ----- peek byte with no sideeffects from I/O range in VICE ----- */
uint8_t cs8900io_peek(uint16_t io_address)
{
    return cs8900io_read(io_address);
}

/* ----- write byte to I/O range of VICE ----- */
void cs8900io_store(uint16_t io_address, uint8_t byte)
{
    if (!cs8900io_cannot_use) {
        cs8900_store(io_address, byte);
    }
}

int cs8900io_dump(void)
{
    return cs8900_dump();
}

static int set_cs8900io_disabled(int val, void *param)
{
    /* dummy function since we don't want "disabled" to be stored on disk */
    return 0;
}

int cs8900io_cart_enabled(void)
{
    return cs8900io_enabled;
}

int cs8900io_enable(const char *owner)
{
    if (!cs8900io_cannot_use) {
        if (!cs8900io_enabled) {
            if (cs8900io_activate() < 0) {
                return -1;
            }
            cs8900io_enabled = 1;
        } else {
            ui_error("CS8900 already in use by %s.", cs8900io_owner);
            return -1;
        }
        cs8900io_reset();
        cs8900io_owner = owner;
        return 0;
    }
    return -1;
}

int cs8900io_disable(void)
{
    if (!cs8900io_cannot_use) {
        /* CS8900 I/O should be deactived */
        if (cs8900io_enabled) {
            cs8900io_enabled = 0;
            if (cs8900io_deactivate() < 0) {
                DBG(("CS8900 I/O: set disabled: error\n"));
                return -1;
            }
            cs8900io_reset();
            cs8900io_owner = NULL;
        }
    }
    return 0;
}

static int set_cs8900io_interface(const char *name, void *param)
{
    if (cs8900io_interface != NULL && name != NULL && strcmp(name, cs8900io_interface) == 0) {
        return 0;
    }

    util_string_set(&cs8900io_interface, name);

    /* if the last interface name was wrong then allow a retry with new name: */
    cs8900io_cannot_use = 0;

    if (cs8900io_enabled) {
        /* ethernet is enabled, make sure that the new name is
           taken account of
         */
        if (cs8900io_deactivate() < 0) {
            return -1;
        }
        if (cs8900io_activate() < 0) {
            return -1;
        }
        /* virtually reset the LAN chip */
        cs8900io_reset();
    }
    return 0;
}

static resource_string_t resources_string[] = {
    { "ETHERNET_INTERFACE", NULL, RES_EVENT_NO, NULL,
      &cs8900io_interface, set_cs8900io_interface, NULL },
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "ETHERNET_DISABLED", 0, RES_EVENT_NO, NULL,
      &cs8900io_cannot_use, set_cs8900io_disabled, NULL },
    RESOURCE_INT_LIST_END
};

int cs8900io_resources_init(void)
{
    if (!cs8900io_resources_init_done) {

        /* allocated in src/arch/shared/rawnetarch_unix/win32/c */
        default_if = rawnet_get_standard_interface();

        if (default_if == NULL) {
            default_if = lib_strdup(ARCHDEP_ETHERNET_DEFAULT_DEVICE);
        }

        resources_string[0].factory_value = default_if;

        if (resources_register_string(resources_string) < 0 ||
            resources_register_int(resources_int) < 0) {
            return -1;
        }
        cs8900io_resources_init_done = 1;
    }
    return 0;
}

void cs8900io_resources_shutdown(void)
{
    /* clean up string allocated by resources_set_defaults() */
    if (cs8900io_interface != NULL) {
        lib_free(cs8900io_interface);
        cs8900io_interface = NULL;
    }
    if (default_if != NULL) {
        lib_free(default_if);
        default_if = NULL;
    }
}

/* ------------------------------------------------------------------------- */
/*    commandline support functions                                          */

static const cmdline_option_t cmdline_options[] =
{
    { "-cs8900ioif", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ETHERNET_INTERFACE", NULL,
      "<Name>", "Set the system ethernet interface" },
    CMDLINE_LIST_END
};

int cs8900io_cmdline_options_init(void)
{
    if (!cs8900io_cmdline_init_done) {
        if (cmdline_register_options(cmdline_options) < 0) {
            return -1;
        }
        cs8900io_cmdline_init_done = 1;
    }
    return 0;
}

#endif /* #ifdef HAVE_RAWNET */
