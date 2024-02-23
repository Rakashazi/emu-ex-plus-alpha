/** \file   rawnetarch.c
 * \brief   raw ethernet interface, architecture-dependant stuff
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 * \author  Alceste Scalas <alceste.scalas@gmail.com>
 *
 * The following functions let the UI enumerate the available interfaces.
 *
 * First, rawnet_arch_enumadapter_open() is used to start enumeration.
 *
 * rawnet_arch_enumadapter() is then used to gather information for each adapter
 * present on the system, where:
 *
 * ppname points to a pointer which will hold the name of the interface
 * ppdescription points to a pointer which will hold the description of the
 * interface
 *
 * For each of these parameters, new memory is allocated, so it has to be
 * freed with lib_free(), except ppdescription, which can be `NULL`, though
 * calling lib_free() on `NULL` is safe.
 *
 * rawnet_arch_enumadapter_close() must be used to stop processing.
 *
 * Each function returns 1 on success, and 0 on failure.
 * rawnet_arch_enumadapter() only fails if there is no more adpater; in this
 * case, *ppname and *ppdescription are not altered.
 */

/*
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

#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "resources.h"
#include "util.h"

#include <stddef.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_RAWNET

#include "archdep_rawnet_capability.h"
#include "rawnetarch.h"

#ifdef WINDOWS_COMPILE
#include "rawnetarch_win32.c"
#endif

#ifdef UNIX_COMPILE
/* On Unix, we implement an abstraction layer to support two rawnet drivers:
 * one based on libpcap, and one based on TUN/TAP.
 */

/* Pointer to the rawnet driver in use. */
const rawnet_arch_driver_t *rawnet_arch_driver = NULL;
char *rawnet_arch_driver_name = NULL;

#define DRIVER_NAME_NONE        "none"

/* Resources configuration ***************************************************/

static int set_ethernet_driver(const char *name, void *param)
{
    const rawnet_arch_driver_t *old_driver = rawnet_arch_driver;
    const char *ifname;
    if (rawnet_arch_driver != NULL && name != NULL && strcmp(name, rawnet_arch_driver->name) == 0) {
        return 0;
    }

    if (rawnet_arch_driver == NULL && name == NULL) {
        return 0;
    }

    if (strcmp(name, DRIVER_NAME_NONE) == 0) {
        if (old_driver) {
            old_driver->deactivate();
        }
        /* If we have found no drivers at all, use "none". */
        rawnet_arch_driver = NULL;
        util_string_set(&rawnet_arch_driver_name, DRIVER_NAME_NONE);
        return 0;
    }
#ifdef HAVE_PCAP
    if (archdep_rawnet_capability() && (strcmp(name, rawnet_arch_driver_pcap.name) == 0)) {
        rawnet_arch_driver = &rawnet_arch_driver_pcap;
    }
#endif
#ifdef HAVE_TUNTAP
    if (strcmp(name, rawnet_arch_driver_tuntap.name) == 0) {
        rawnet_arch_driver = &rawnet_arch_driver_tuntap;
    }
#endif

    if (rawnet_arch_driver != NULL) {
        util_string_set(&rawnet_arch_driver_name, rawnet_arch_driver->name);

        if (old_driver == NULL) {
            /* This is the first time the driver is selected: VICE will
             * take care of calling rawnet_arch_activate() later
             */
            return 0;
        }
        /* We have switched driver, so we need to deactivate the old one
         * and activate the new one
         */
#ifdef RAWNET_DEBUG_ARCH
        log_message(rawnet_arch_log, "deactivating driver: %s.", old_driver->name);
        old_driver->deactivate();
#endif

        resources_get_string("ETHERNET_INTERFACE", &ifname);
        if (rawnet_arch_activate(ifname) == 0) {
            return -1;
        }
        return 0;
    }

    return -1; /* Unsupported driver */
}

static resource_string_t resources_string[] = {
    { "ETHERNET_DRIVER", NULL, RES_EVENT_NO, NULL,
      &rawnet_arch_driver_name, set_ethernet_driver, NULL },
      RESOURCE_STRING_LIST_END
};

static int rawnet_arch_resources_init_done = 0;

int rawnet_arch_resources_init(void)
{
    if (!rawnet_arch_resources_init_done) {
        const char *default_driver = DRIVER_NAME_NONE;

#ifdef HAVE_TUNTAP
        /* Default fallback if tuntap is available */
        default_driver = rawnet_arch_driver_tuntap.name;
#endif

#ifdef HAVE_PCAP
        if (archdep_rawnet_capability()) {
            default_driver = rawnet_arch_driver_pcap.name;
        }
#endif
        resources_string[0].factory_value = default_driver;

        if (resources_register_string(resources_string) < 0) {
            return -1;
        }
        /* make sure the respective function pointers are initialized, so the
           ETHERNET_INTERFACE init can use rawnet_get_standard_interface() to
           set the proper default value */
        set_ethernet_driver(default_driver, NULL);
        rawnet_arch_resources_init_done = 1;
    }
    return 0;
}

void rawnet_arch_resources_shutdown(void)
{
    if (rawnet_arch_driver_name != NULL) {
        lib_free(rawnet_arch_driver_name);
        rawnet_arch_driver_name = NULL;
    }
}
/* Command line options configuration ****************************************/

static const cmdline_option_t cmdline_options[] =
{
    { "-ethernetiodriver", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ETHERNET_DRIVER", NULL,
      "<Name>", "Set the low-level driver for Ethernet emulation (tuntap, pcap)." },
    CMDLINE_LIST_END
};

static int rawnet_arch_cmdline_init_done = 0;

int rawnet_arch_cmdline_options_init(void)
{
    if (!rawnet_arch_cmdline_init_done) {
        if (cmdline_register_options(cmdline_options) < 0) {
            return -1;
        }
        rawnet_arch_cmdline_init_done = 1;
    }
    return 0;
}

/* Rawnet functions and driver interface *************************************/

log_t rawnet_arch_log = LOG_ERR;

#ifdef RAWNET_DEBUG_PKTDUMP
void rawnet_arch_debug_output(const char *text, uint8_t *what, int count)
{
    char buffer[256];
    char *p = buffer;
    char *pbuffer1 = what;
    int len1 = count;
    int i;

    sprintf(buffer, "\n%s: length = %u\n", text, len1);
    fprintf(stderr, "%s", buffer);
    do {
        p = buffer;
        for (i=0; (i < 8) && len1 > 0; len1--, i++) {
            sprintf(p, "%02x ", (unsigned int)(unsigned char)*pbuffer1++);
            p += 3;
        }
        *(p-1) = '\n'; *p = 0;
        fprintf(stderr, "%s", buffer);
    } while (len1>0);
}
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */

int rawnet_arch_init(void)
{
    rawnet_arch_log = log_open("TFEARCH");
    return 1;
}

void rawnet_arch_pre_reset(void)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_pre_reset() (driver: %s).",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name);
#endif
    rawnet_arch_driver->pre_reset();
}

void rawnet_arch_post_reset(void)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_post_reset() (driver: %s).",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name);
#endif
    rawnet_arch_driver->post_reset();
}

int rawnet_arch_activate(const char *interface_name)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_activate() (driver: %s) called with interface: %s.",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name,
                interface_name == NULL ? "NULL" : interface_name);
#endif
    if (rawnet_arch_driver == NULL) {
        return -1;
    }
    return rawnet_arch_driver->activate(interface_name);
}

void rawnet_arch_deactivate(void)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_deactivate() (driver: %s).",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name);
#endif
    rawnet_arch_driver->deactivate();
}

void rawnet_arch_set_mac(const uint8_t mac[6])
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "New MAC address set (driver: %s): %02X:%02X:%02X:%02X:%02X:%02X.",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name,
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] );
#endif
    rawnet_arch_driver->set_mac(mac);
}

void rawnet_arch_set_hashfilter(const uint32_t hash_mask[2])
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "New hash filter set (driver: %s): %08X:%08X.",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name,
                hash_mask[1], hash_mask[0]);
#endif
    rawnet_arch_driver->set_hashfilter(hash_mask);
}

void rawnet_arch_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_recv_ctl() (driver: %s) called with the following parameters:",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name);
    log_message(rawnet_arch_log, "\tbBroadcast   = %s", bBroadcast ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbIA          = %s", bIA ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbMulticast   = %s", bMulticast ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbCorrect     = %s", bCorrect ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbPromiscuous = %s", bPromiscuous ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbIAHash      = %s", bIAHash ? "TRUE" : "FALSE");
#endif
    rawnet_arch_driver->recv_ctl(bBroadcast, bIA, bMulticast, bCorrect, bPromiscuous, bIAHash);
}

void rawnet_arch_line_ctl(int bEnableTransmitter, int bEnableReceiver)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_line_ctl() (driver: %s) called with the following parameters:",
                rawnet_arch_driver->name == NULL ? "NULL" : rawnet_arch_driver->name);
    log_message(rawnet_arch_log, "\tbEnableTransmitter = %s",
                bEnableTransmitter ? "TRUE" : "FALSE");
    log_message(rawnet_arch_log, "\tbEnableReceiver    = %s",
                bEnableReceiver ? "TRUE" : "FALSE");
#endif
    rawnet_arch_driver->line_ctl(bEnableTransmitter, bEnableReceiver);
}

void rawnet_arch_transmit(int force, int onecoll, int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log,
            "rawnet_arch_transmit() called (driver: %s), with: force = %s, onecoll = %s, "
            "inhibit_crc=%s, tx_pad_dis=%s, txlength=%u",
            rawnet_arch_driver->name,
            force ? "TRUE" : "FALSE",
            onecoll ? "TRUE" : "FALSE",
            inhibit_crc ? "TRUE" : "FALSE",
            tx_pad_dis ? "TRUE" : "FALSE",
            txlength);
#endif
#ifdef RAWNET_DEBUG_PKTDUMP
    rawnet_arch_debug_output("Transmit frame: ", txframe, txlength);
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */
    rawnet_arch_driver->transmit(force, onecoll, inhibit_crc, tx_pad_dis, txlength, txframe);
}

int rawnet_arch_receive(uint8_t *pbuffer, int *plen, int *phashed, int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast, int *pcrc_error)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log,
            "rawnet_arch_receive() called, with *plen=%u (driver: %s).",
            *plen, rawnet_arch_driver->name);
#endif
    return rawnet_arch_driver->receive(pbuffer, plen, phashed, phash_index, prx_ok, pcorrect_mac, pbroadcast, pcrc_error);
}

int rawnet_arch_enumadapter_open(void)
{
    if (rawnet_arch_driver != NULL) {
        return rawnet_arch_driver->enumadapter_open();
    }
    return 0;
}

int rawnet_arch_enumadapter(char **ppname, char **ppdescription)
{
    if (rawnet_arch_driver != NULL) {
        return rawnet_arch_driver->enumadapter(ppname, ppdescription);
    }
    return 0;
}

int rawnet_arch_enumadapter_close(void)
{
    if (rawnet_arch_driver != NULL) {
        return rawnet_arch_driver->enumadapter_close();
    }
    return 0;
}

char *rawnet_arch_get_standard_interface(void)
{
    if (rawnet_arch_driver != NULL) {
        /* The driver might be selected later on */
        return rawnet_arch_driver->get_standard_interface();
    }
    return NULL;
}

#endif /* ifdef UNIX_COMPILE */

static int rawnetdriverindex = -1;
static char *rawnetdrivernames[] = {
    "none",
#ifdef HAVE_TUNTAP
    "tuntap",
#endif
#ifdef HAVE_PCAP
    "pcap",
#endif
    NULL
};

static char *rawnetdriverdescs[] = {
    "none",
#ifdef HAVE_TUNTAP
    "tun/tap",
#endif
#ifdef HAVE_PCAP
    "PCAP",
#endif
    NULL
};

int rawnet_arch_enumdriver_open(void)
{
    rawnetdriverindex = 0;
    /* HACK! remove pcap from the list when its not available */
#ifdef HAVE_PCAP
    if (!archdep_rawnet_capability()) {
#ifdef HAVE_TUNTAP
        rawnetdrivernames[2] = NULL;
        rawnetdriverdescs[2] = NULL;
#else
        rawnetdrivernames[1] = NULL;
        rawnetdriverdescs[1] = NULL;
#endif
    }
#endif
    return 1;
}

int rawnet_arch_enumdriver(char **ppname, char **ppdescription)
{
    char *name, *desc;
    if (rawnetdriverindex >= 0) {
        name = rawnetdrivernames[rawnetdriverindex];
        desc = rawnetdriverdescs[rawnetdriverindex];
        if ((name == NULL) || (desc == NULL)) {
            rawnetdriverindex = -1;
            return 0;
        }
        if (ppname) {
            *ppname = lib_strdup(name);
        }
        if (ppdescription) {
            *ppdescription = lib_strdup(desc);
        }
        rawnetdriverindex++;
        return 1;
    }
    return 0;
}

int rawnet_arch_enumdriver_close(void)
{
    rawnetdriverindex = -1;
    return 1;
}

char *rawnet_arch_get_standard_driver(void)
{
#ifdef HAVE_PCAP
    if (archdep_rawnet_capability()) {
        return "pcap";
    }
#endif
#ifdef HAVE_TUNTAP
    return "tuntap";
#endif
    /* we should never come here */
    return "none";
}

#endif  /* ifdef HAVE_RAWNET */
