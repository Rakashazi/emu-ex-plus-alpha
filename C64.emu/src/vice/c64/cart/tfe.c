/*
 * tfe.c - TFE ("The final ethernet") emulation.
 *
 * Written by
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

#ifdef HAVE_TFE

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef DOS_TFE
#include <pcap.h>
#endif

#include "archdep.h"
#include "c64export.h"
#include "cartio.h"
#include "cartridge.h"
#include "cmdline.h"
#include "cs8900.h"
#include "crc32.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "rawnet.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "snapshot.h"
#include "util.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "mmc64.h"
#include "mmcreplay.h"
#include "retroreplay.h"
#include "tfe.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/*
    "The Final Ethernet" Cartridge

    - the original TFE cart simply contains a cs8900a mapped to de00.
    - this implementation also emulates various configurations of the
      very similar "rr-net" clockport addon.
    - for register documentation refer to the cs8900a datasheet and/or
      the respective register documentation for mmc64, retroreplay, mmcr

    FIXME: create better logic to handle clockports
*/

/* #define TFE_DEBUG */

#ifdef TFE_DEBUG
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */
/*    resources support functions                                            */

/* some prototypes are needed */
static void tfe_store(WORD io_address, BYTE byte);
static BYTE tfe_read(WORD io_address);
static BYTE tfe_peek(WORD io_address);

static io_source_t rrnet_io1_mmc64_device = {
    CARTRIDGE_NAME_RRNET " on " CARTRIDGE_NAME_MMC64 " Clockport",
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xde02, 0xde0f, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_TFE,
    0,
    0
};

static io_source_t rrnet_io1_retroreplay_device = {
    CARTRIDGE_NAME_RRNET " on " CARTRIDGE_NAME_RETRO_REPLAY " Clockport",
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xde02, 0xde0f, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* TODO: dump */
    CARTRIDGE_TFE,
    0,
    0
};

static io_source_t rrnet_io1_mmcreplay_device = {
    CARTRIDGE_NAME_RRNET " on " CARTRIDGE_NAME_MMC_REPLAY " Clockport",
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xde02, 0xde0f, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* dump */
    CARTRIDGE_TFE,
    0,
    0
};

static io_source_t rrnet_io1_device = {
    CARTRIDGE_NAME_RRNET,
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xde00, 0xde0f, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* dump */
    CARTRIDGE_TFE,
    0,
    0
};

static io_source_t tfe_io1_device = {
    CARTRIDGE_NAME_TFE,
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xde00, 0xdeff, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* dump */
    CARTRIDGE_TFE,
    0,
    0
};

static io_source_t rrnet_io2_mmc64_device = {
    CARTRIDGE_NAME_RRNET " on " CARTRIDGE_NAME_MMC64 " Clockport",
    IO_DETACH_RESOURCE,
    "ETHERNET_ACTIVE",
    0xdf22, 0xdf2f, 0x0f,
    0,
    tfe_store,
    tfe_read,
    tfe_peek,
    NULL, /* dump */
    CARTRIDGE_TFE,
    0,
    0
};

static c64export_resource_t export_res = {
    CARTRIDGE_NAME_TFE, 0, 0, &tfe_io1_device, NULL, CARTRIDGE_TFE
};

/* current configurations */
static io_source_t *tfe_current_device = &tfe_io1_device;
static io_source_list_t *tfe_list_item = NULL;

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/*
 This variable is used when we need to postpone the initialization
 because tfe_init() is not yet called
*/
static int should_activate = 0;

static log_t tfe_log = LOG_ERR;

/* Flag: Can we even use TFE, or is the hardware not available? */
static int tfe_cannot_use = 0;

/* Flag: Do we have the TFE enabled?  */
static int tfe_enabled = 0;

/* Flag: Do we use the "original" memory map or the memory map of the RR-Net? */
int tfe_as_rr_net = 0;

static char *tfe_interface = NULL;

/* Flag: swap io1/io2, currently only used for vic20 masC=uerade,
         but future usage of an io-swapper is possible */
static int tfe_io_swap = 0;

/* ------------------------------------------------------------------------- */
/*    initialization and deinitialization functions                          */

void tfe_reset(void)
{
    if (tfe_enabled && !should_activate) {
        cs8900_reset();
    }
}

#ifdef DOS_TFE
static void set_standard_tfe_interface(void)
{
    char *dev, errbuf[PCAP_ERRBUF_SIZE];
    dev = pcap_lookupdev(errbuf);
    util_string_set(&tfe_interface, dev);
}
#endif

static int tfe_activate(void)
{
#ifdef TFE_DEBUG
    log_message(tfe_log, "tfe_activate().");
#endif

    if (tfe_log != LOG_ERR) {
#ifdef DOS_TFE
        set_standard_tfe_interface();
#endif
        switch (cs8900_activate(tfe_interface)) {
            case -1:
                tfe_enabled = 0;
                break;
            case -2:
                tfe_enabled = 0;
                tfe_cannot_use = 1;
                break;
        }
    } else {
        should_activate = 1;
    }
    return 0;
}

static int tfe_deactivate(void)
{
#ifdef TFE_DEBUG
    log_message(tfe_log, "tfe_deactivate().");
#endif

    if (should_activate) {
        should_activate = 0;
    } else {
        tfe_enabled = 0;
        should_activate = 0;
        /* FIXME: WTH check for tfe_log here? */
        if (tfe_log != LOG_ERR) {
            return cs8900_deactivate();
        }
    }

    return 0;
}

void tfe_init(void)
{
    tfe_log = log_open("TFE");

    rawnet_set_should_accept_func(cs8900_should_accept);
    if (cs8900_init() < 0) {
        tfe_enabled = 0;
        tfe_cannot_use = 1;
    }

    if (should_activate) {
        should_activate = 0;
        if (tfe_activate() < 0) {
            tfe_enabled = 0;
            tfe_cannot_use = 1;
        }
    }
}

void tfe_detach(void)
{
#ifdef TFE_DEBUG
    log_message(tfe_log, "tfe_shutdown().");
#endif

    if (tfe_enabled) {
        cs8900_shutdown();
        tfe_enabled = 0;
        should_activate = 0;
#ifdef TFE_DEBUG
        log_message(tfe_log, "...2");
#endif
    }

#ifdef TFE_DEBUG
    log_message(tfe_log, "tfe_shutdown() done.");
#endif
}

/* ------------------------------------------------------------------------- */
/* retroreplay/mmc64 clockport changes helper function */

void tfe_clockport_changed(void)
{
    if (!tfe_as_rr_net) {
        tfe_current_device = &tfe_io1_device;
    } else {
        tfe_current_device = &rrnet_io1_device;
        if (mmc64_cart_enabled() && (mmc64_hw_clockport == 0xde02) && mmc64_clockport_enabled) {
            tfe_current_device = &rrnet_io1_mmc64_device;
        }
        if (mmc64_cart_enabled() && (mmc64_hw_clockport == 0xdf12) && mmc64_clockport_enabled) {
            tfe_current_device = &rrnet_io2_mmc64_device;
        }
        if (retroreplay_cart_enabled() && rr_clockport_enabled) {
            tfe_current_device = &rrnet_io1_retroreplay_device;
        }
        if (mmcreplay_cart_enabled() && mmcr_clockport_enabled) {
            tfe_current_device = &rrnet_io1_mmcreplay_device;
        }
    }
    /* if adapter is already enabled then reset the LAN chip */
    if (tfe_enabled) {
        io_source_unregister(tfe_list_item);
        c64export_remove(&export_res);
        export_res.io1 = tfe_current_device;
        if (c64export_add(&export_res) < 0) {
            DBG(("TFE: set tfe_clockport_changed: error\n"));
            tfe_list_item = NULL;
            tfe_enabled = 0;
            return;
        }
        if (machine_class == VICE_MACHINE_VIC20) {
            /* set correct addresses for masC=uerade */
            if (tfe_io_swap) {
                tfe_current_device->start_address = 0x9c00;
                tfe_current_device->end_address = 0x9fff;
            } else {
                tfe_current_device->start_address = 0x9800;
                tfe_current_device->end_address = 0x9bff;
            }
        }
        tfe_list_item = io_source_register(tfe_current_device);
        tfe_reset();
    }
}

/* ------------------------------------------------------------------------- */

/* ----- read byte from I/O range in VICE ----- */
static BYTE tfe_read(WORD io_address)
{
    if (tfe_as_rr_net) {
        /* rr status register is handled by rr cartidge */
        if (io_address < 0x02) {
            tfe_current_device->io_source_valid = 0;
            return 0;
        }
        io_address ^= 0x08;
    }
    tfe_current_device->io_source_valid = 1;

    if (!tfe_cannot_use) {
        return cs8900_read(io_address);
    }
    return 0;
}

/* ----- peek byte with no sideeffects from I/O range in VICE ----- */
static BYTE tfe_peek(WORD io_address)
{
    if (tfe_as_rr_net) {
        /* rr status register is handled by rr cartidge */
        if (io_address < 0x02) {
            return 0;
        }
        io_address ^= 0x08;
    }
    if (!tfe_cannot_use) {
        return cs8900_peek(io_address);
    }
    return 0;
}

/* ----- write byte to I/O range of VICE ----- */
static void tfe_store(WORD io_address, BYTE byte)
{
    if (tfe_as_rr_net) {
        /* rr control register is handled by rr cartidge */
        if (io_address < 0x02) {
            return;
        }
        io_address ^= 0x08;
    }

    if (!tfe_cannot_use) {
        cs8900_store(io_address, byte);
    }
}

static int set_tfe_disabled(int val, void *param)
{
    /* dummy function since we don't want "disabled" to be stored on disk */
    return 0;
}

static int set_tfe_rr_net(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!tfe_cannot_use) {
        if (!val) {
            /* TFE should not be used as rr net */
            if (tfe_as_rr_net) {
                tfe_as_rr_net = 0;
                tfe_clockport_changed();
            }
        } else {
            if (!tfe_as_rr_net) {
                tfe_as_rr_net = 1;
                tfe_clockport_changed();
            }
        }
    }
    return 0;
}

int tfe_cart_enabled(void)
{
    return tfe_enabled;
}

static int set_tfe_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!tfe_cannot_use) {
        if (!val) {
            /* TFE should be deactived */
            if (tfe_enabled) {
                tfe_enabled = 0;
                if (tfe_deactivate() < 0) {
                    DBG(("TFE: set disabled: error\n"));
                    return -1;
                }
                io_source_unregister(tfe_list_item);
                tfe_list_item = NULL;
                c64export_remove(&export_res);
            }
            return 0;
        } else {
            if (!tfe_enabled) {
                tfe_enabled = 1;
                if (tfe_activate() < 0) {
                    return -1;
                }
                export_res.io1 = tfe_current_device;
                if (c64export_add(&export_res) < 0) {
                    DBG(("TFE: set enabled: error\n"));
                    tfe_list_item = NULL;
                    tfe_enabled = 0;
                    return -1;
                }
                if (machine_class == VICE_MACHINE_VIC20) {
                    /* set correct addresses for masC=uerade */
                    if (tfe_io_swap) {
                        tfe_current_device->start_address = 0x9c00;
                        tfe_current_device->end_address = 0x9fff;
                    } else {
                        tfe_current_device->start_address = 0x9800;
                        tfe_current_device->end_address = 0x9bff;
                    }
                }
                tfe_list_item = io_source_register(tfe_current_device);
            }

            return 0;
        }
    }
    return 0;
}

static int set_tfe_interface(const char *name, void *param)
{
    if (tfe_interface != NULL && name != NULL && strcmp(name, tfe_interface) == 0) {
        return 0;
    }

    util_string_set(&tfe_interface, name);

    /* CV: if the last interface name was wrong then allow a retry with new name: */
    tfe_cannot_use = 0;

    if (tfe_enabled) {
        /* ethernet is enabled, make sure that the new name is
           taken account of
         */
        if (tfe_deactivate() < 0) {
            return -1;
        }
        if (tfe_activate() < 0) {
            return -1;
        }
        /* virtually reset the LAN chip */
        tfe_reset();
    }
    return 0;
}

int tfe_enable(void)
{
    return resources_set_int("ETHERNET_ACTIVE", 1);
}

static int set_tfe_io_swap(int value, void *param)
{
    int val = value ? 1 : 0;

    if (val == tfe_io_swap) {
        return 0;
    }

    if (tfe_enable()) {
        set_tfe_enabled(0, NULL);
        tfe_io_swap = val;
        set_tfe_enabled(1, NULL);
    } else {
        tfe_io_swap = val;
    }
    return 0;
}

static const resource_string_t resources_string[] = {
    { "ETHERNET_INTERFACE",
      ARCHDEP_ETHERNET_DEFAULT_DEVICE, RES_EVENT_NO, NULL,
      &tfe_interface, set_tfe_interface, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "ETHERNET_DISABLED", 0, RES_EVENT_NO, NULL,
      &tfe_cannot_use, set_tfe_disabled, NULL },
    { "ETHERNET_ACTIVE", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tfe_enabled, set_tfe_enabled, NULL },
    { "ETHERNET_AS_RR", 0, RES_EVENT_NO, NULL,
      &tfe_as_rr_net, set_tfe_rr_net, NULL },
    { NULL }
};

static const resource_int_t resources_mascuerade_int[] = {
    { "TFEIOSwap", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &tfe_io_swap, set_tfe_io_swap, NULL },
    { NULL }
};

int tfe_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    if (machine_class == VICE_MACHINE_VIC20) {
        if (resources_register_int(resources_mascuerade_int) < 0) {
            return -1;
        }
    }

    return resources_register_int(resources_int);
}

void tfe_resources_shutdown(void)
{
    lib_free(tfe_interface);
}

/* ------------------------------------------------------------------------- */
/*    commandline support functions                                          */

static const cmdline_option_t cmdline_options[] =
{
    { "-tfe", SET_RESOURCE, 0,
      NULL, NULL, "ETHERNET_ACTIVE", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TFE,
      NULL, NULL },
    { "+tfe", SET_RESOURCE, 0,
      NULL, NULL, "ETHERNET_ACTIVE", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TFE,
      NULL, NULL },
    { "-tfeif", SET_RESOURCE, 1,
      NULL, NULL, "ETHERNET_INTERFACE", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_TFE_INTERFACE,
      NULL, NULL },
    { "-tferrnet", SET_RESOURCE, 0,
      NULL, NULL, "ETHERNET_AS_RR", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TFE_AS_RRNET,
      NULL, NULL },
    { "+tferrnet", SET_RESOURCE, 0,
      NULL, NULL, "ETHERNET_AS_RR", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TFE_AS_RRNET,
      NULL, NULL },
    { NULL }
};

static const cmdline_option_t cmdline_mascuerade_options[] =
{
    { "-tfeioswap", SET_RESOURCE, 0,
      NULL, NULL, "TFEIOSwap", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MAP_CART_IO_2,
      NULL, NULL },
    { "+tfeioswap", SET_RESOURCE, 0,
      NULL, NULL, "TFEIOSwap", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_MAP_CART_IO_3,
      NULL, NULL },
    { NULL }
};

int tfe_cmdline_options_init(void)
{
    if (machine_class == VICE_MACHINE_VIC20) {
        if (cmdline_register_options(cmdline_mascuerade_options) < 0) {
            return -1;
        }
    }

    return cmdline_register_options(cmdline_options);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "CARTTFE"

/* FIXME: implement snapshot support */
int tfe_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int tfe_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

#endif /* #ifdef HAVE_TFE */
