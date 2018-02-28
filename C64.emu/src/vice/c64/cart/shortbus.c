/*
 * shortbus.c - IDE64 Short Bus emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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
#include <stdlib.h>
#include <string.h>

#include "snapshot.h"

#include "shortbus_digimax.h"

#ifdef HAVE_PCAP
#include "shortbus_etfe.h"
#endif

/* TODO */
#if 0
#include "shortbus_duart.h"
#include "shortbus_eth64.h"
#endif

int shortbus_resources_init(void)
{
    if (shortbus_digimax_resources_init() < 0) {
        return -1;
    }

#ifdef HAVE_PCAP
    if (shortbus_etfe_resources_init() < 0) {
        return -1;
    }
#endif

/* TODO */
#if 0
    if (shortbus_duart_resources_init() < 0) {
        return -1;
    }

    if (shortbus_eth64_resources_init() < 0) {
        return -1;
    }
#endif

    return 0;
}

void shortbus_resources_shutdown(void)
{
    shortbus_digimax_resources_shutdown();

#ifdef HAVE_PCAP
    shortbus_etfe_resources_shutdown();
#endif

/* TODO */
#if 0
    shortbus_duart_resources_shutdown();
    shortbus_eth64_resources_shutdown();
#endif

}

int shortbus_cmdline_options_init(void)
{
    if (shortbus_digimax_cmdline_options_init() < 0) {
        return -1;
    }

#ifdef HAVE_PCAP
    if (shortbus_etfe_cmdline_options_init() < 0) {
        return -1;
    }
#endif

/* TODO */
#if 0
    if (shortbus_duart_cmdline_options_init() < 0) {
        return -1;
    }

    if (shortbus_eth64_cmdline_options_init() < 0) {
        return -1;
    }
#endif

    return 0;
}

extern void shortbus_unregister(void)
{
    shortbus_digimax_unregister();

#ifdef HAVE_PCAP
    shortbus_etfe_unregister();
#endif

/* TODO */
#if 0
    shortbus_duart_unregister();
    shortbus_eth64_unregister();
#endif
}


extern void shortbus_register(void)
{
    shortbus_digimax_register();

#ifdef HAVE_PCAP
    shortbus_etfe_register();
#endif

/* TODO */
#if 0
    shortbus_duart_register();
    shortbus_eth64_register();
#endif
}

void shortbus_reset(void)
{
    shortbus_digimax_reset();

#ifdef HAVE_PCAP
    shortbus_etfe_reset();
#endif

/* TODO */
#if 0
    shortbus_duart_reset();
    shortbus_eth64_reset();
#endif
}

/* ------------------------------------------------------------------------- */

/* SHORTBUS snapshot module format:

   type  | name           | description
   ------------------------------------
   BYTE  | amount         | amount of active shortbus devices
   BYTE  | digimax active | digimax active flag
   BYTE  | duart active   | duart active flag
   BYTE  | etfe active    | etfe active flag
   BYTE  | eth64 active   | eth64 active flag
 */

static char snap_module_name[] = "SHORTBUS";
#define SNAP_MAJOR 0
#define SNAP_MINOR 0

int shortbus_write_snapshot_module(snapshot_t *s)
{
    snapshot_module_t *m;
    int active_devices = 0;
    int devices[4] = { 0, 0, 0, 0 };

    if (shortbus_digimax_enabled()) {
        ++active_devices;
        devices[0] = 1;
    }

#ifdef HAVE_PCAP
    if (shortbus_etfe_enabled()) {
        ++active_devices;
        devices[2] = 1;
    }
#endif

/* TODO */
#if 0
    if (shortbus_duart_enabled()) {
        ++active_devices;
        devices[1] = 1;
    }
    if (shortbus_eth64_enabled()) {
        ++active_devices;
        devices[3] = 1;
    }
#endif

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, (BYTE)active_devices) < 0
        || SMW_B(m, (BYTE)devices[0]) < 0
        || SMW_B(m, (BYTE)devices[1]) < 0
        || SMW_B(m, (BYTE)devices[2]) < 0
        || SMW_B(m, (BYTE)devices[3]) < 0) {
        snapshot_module_close(m);
        return -1;
    }
    snapshot_module_close(m);

    if (active_devices) {
        if (devices[0]) {
            if (shortbus_digimax_write_snapshot_module(s) < 0) {
                return -1;
            }
        }

#ifdef HAVE_PCAP
        if (devices[2]) {
            if (shortbus_etfe_write_snapshot_module(s) < 0) {
                return -1;
            }
        }
#endif

        /* TODO */
#if 0
        if (devices[1]) {
            if (shortbus_duart_write_snapshot_module(s) < 0) {
                return -1;
            }
        }
        if (devices[3]) {
            if (shortbus_eth64_write_snapshot_module(s) < 0) {
                return -1;
            }
        }
#endif
    }

    return 0;
}

int shortbus_read_snapshot_module(snapshot_t *s)
{
    BYTE major_version, minor_version;
    snapshot_module_t *m;
    int active_devices;
    int devices[4];

    m = snapshot_module_open(s, snap_module_name, &major_version, &minor_version);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (major_version > SNAP_MAJOR || minor_version > SNAP_MINOR) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        goto fail;
    }

    if (0
        || SMR_B_INT(m, &active_devices) < 0
        || SMR_B_INT(m, &devices[0]) < 0
        || SMR_B_INT(m, &devices[1]) < 0
        || SMR_B_INT(m, &devices[2]) < 0
        || SMR_B_INT(m, &devices[3]) < 0) {
        goto fail;
    }

    snapshot_module_close(m);

    if (active_devices) {
        if (devices[0]) {
            if (shortbus_digimax_read_snapshot_module(s) < 0) {
                return -1;
            }        
        }

#ifdef HAVE_PCAP
        if (devices[2]) {
            if (shortbus_etfe_read_snapshot_module(s) < 0) {
                return -1;
            }        
        }
#endif

        /* TODO */
#if 0
        if (devices[1]) {
            if (shortbus_duart_read_snapshot_module(s) < 0) {
                return -1;
            }        
        }
        if (devices[3]) {
            if (shortbus_eth64_read_snapshot_module(s) < 0) {
                return -1;
            }        
        }
#endif
    }
    return 0;

fail:
    snapshot_module_close(m);
    return -1;
}
