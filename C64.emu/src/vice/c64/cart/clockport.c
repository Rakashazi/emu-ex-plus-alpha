/*
 * clockport.c - ClockPort emulation.
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

#include "clockport.h"
#include "lib.h"

#ifdef HAVE_PCAP
#include "clockport-rrnet.h"
#endif

#ifdef USE_MPG123
#include "clockport-mp3at64.h"
#endif

/* TODO */
#if 0
#include "clockport_eth64_ii.h"
#include "clockport_silver_surfer.h"
#include "clockport_cw3_sid.h"
#endif

clockport_supported_devices_t clockport_supported_devices[] = {
    { CLOCKPORT_DEVICE_NONE,          "None" },
#ifdef HAVE_PCAP
    { CLOCKPORT_DEVICE_RRNET,         "RRNet" },
#endif
#ifdef USE_MPG123
    { CLOCKPORT_DEVICE_MP3_64,        "MP3@64" },
#endif
#if 0
    { CLOCKPORT_DEVICE_ETH64_II,      "ETH64-II" },
    { CLOCKPORT_DEVICE_SILVER_SURFER, "Silver Surfer" },
    { CLOCKPORT_DEVICE_CW3_SID,       "CatWeasel MKIII SID" },
#endif
    { -1,                             NULL }
};

char *clockport_device_id_to_name(int id)
{
    int i;

    for (i = 0; clockport_supported_devices[i].name; ++i) {
        if (clockport_supported_devices[i].id == id) {
            return clockport_supported_devices[i].name;
        }
    }
    return "Unknown";
}

static clockport_device_list_t clockport_device_head = { NULL, NULL };

int clockport_resources_init(void)
{
    /* Init clockport devices */
#ifdef HAVE_PCAP
    clockport_rrnet_init();
#endif

#ifdef USE_MPG123
    clockport_mp3at64_init();
#endif

    /* TODO */
#if 0
    clockport_eth64_ii_init();
    clockport_silver_surfer_init();
    clockport_cw3_sid_init();
#endif

    return 0;
}

void clockport_resources_shutdown(void)
{
    clockport_device_list_t *current = &clockport_device_head;

    while (current->next != NULL) {
        if (current->next->device) {
            clockport_close_device(current->next->device);
        }
    }

    /* Shutdown clockport devices */
#ifdef HAVE_PCAP
    clockport_rrnet_shutdown();
#endif

#ifdef USE_MPG123
    clockport_mp3at64_shutdown();
#endif

    /* TODO */
#if 0
    clockport_eth64_ii_shutdown();
    clockport_silver_surfer_shutdown();
    clockport_cw3_sid_shutdown();
#endif
}

clockport_device_t *clockport_open_device(int deviceid, char *owner)
{
    clockport_device_list_t *current = &clockport_device_head;
    clockport_device_t *retval = NULL;
    clockport_device_list_t *entry = NULL;

    switch (deviceid) {
#ifdef HAVE_PCAP
        case CLOCKPORT_DEVICE_RRNET:
            retval = clockport_rrnet_open_device(owner);
            break;
#endif

#ifdef USE_MPG123
        case CLOCKPORT_DEVICE_MP3_64:
            retval = clockport_mp3at64_open_device(owner);
            break;
#endif

        /* TODO */
#if 0
        case CLOCKPORT_DEVICE_ETH64_II:
            retval = clockport_eth64_ii_open_device(owner);
            break;
        case CLOCKPORT_DEVICE_SILVER_SURFER:
            retval = clockport_silver_surfer_open_device(owner);
            break;
        case CLOCKPORT_DEVICE_CW3_SID:
            retval = clockport_cw3_sid_open_device(owner);
            break;
#endif
    }

    if (retval) {
        entry = lib_malloc(sizeof(clockport_device_list_t));
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = entry;
        entry->device = retval;
        entry->next = NULL;
    }
    return retval;
}

void clockport_close_device(clockport_device_t *device)
{
    clockport_device_list_t *current = &clockport_device_head;
    clockport_device_list_t *prev = NULL;

    while (current->next != NULL) {
        prev = current;
        current = current->next;
        if (current->device == device) {
            current->device->close(device);
            prev->next = current->next;
            lib_free(current);
        }
    }
}
