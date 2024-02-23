/** \file   rawnetarch_tuntap.c
 * \brief   Raw ethernet driver for Unix, based on TUN/TAP.
 *
 * \author  Alceste Scalas <alceste.scalas@gmail.com>
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

#include <stdint.h>

#include "vice.h"

#ifdef HAVE_RAWNET
#ifdef HAVE_TUNTAP

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "archdep_defs.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "rawnetarch.h"
#include "resources.h"
#include "util.h"

/* These includes are Linux-specific. By tweaking them, the rest
 * of the code should also work on FreeBSD and NetBSD */
#include <linux/if.h>
#include <linux/if_tun.h>

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/* "Clone device" (/dev/net/tun) file descriptor */
static int rawnet_arch_tuntap_tun_fd = -1;

/* Populated while enumerating network interfaces */
struct if_nameindex *enum_interfaces = NULL;
int enum_interfaces_idx = -1;

static int rawnet_arch_tuntap_enumadapter_open(void)
{
    assert((enum_interfaces == NULL) && (enum_interfaces_idx == -1));
    enum_interfaces = if_nameindex();
    if (enum_interfaces == NULL) {
        log_message(rawnet_arch_log, "ERROR in rawnet_arch_enumadapter_open: '%s'",
                    strerror(errno));
        return 0;
    }
    enum_interfaces_idx = 0;
    return 1;
}

/** \brief  Get exactly one TUN/TAP device
 *
 * The \a ppname and \a ppdescription are heap-allocated via lib_strdup()
 * and should thus be freed after use with lib_free().
 *
 * \param[out]  ppname          device name
 * \param[out]  ppdescription   device description
 *
 * \return  bool (1 on success, 0 on failure)
 */
static int rawnet_arch_tuntap_enumadapter(char **ppname, char **ppdescription)
{
    const struct if_nameindex *i = &enum_interfaces[enum_interfaces_idx];

    assert((enum_interfaces != NULL) && (enum_interfaces_idx != -1));

    if ((i->if_index != 0) && (i->if_name != NULL)) {
        /* TODO: here it would be nice to only return TUN/TAP interfaces */
        *ppname = lib_strdup(i->if_name);
        *ppdescription = NULL;
        enum_interfaces_idx++;
        return 1;
    } else {
        return 0;
    }
}

static int rawnet_arch_tuntap_enumadapter_close(void)
{
    assert((enum_interfaces != NULL) && (enum_interfaces_idx != -1));

    if_freenameindex(enum_interfaces);
    enum_interfaces = NULL;
    enum_interfaces_idx = -1;

    return 1;
}

static int rawnet_tuntap_open_adapter(const char *interface_name)
{
    struct ifreq ifr;
    const char *tundev = "/dev/net/tun";

    /* Here it would be nice to use O_NONBLOCK, but this seems to cause
     * a massive loss of frames (at least on Linux). So, we'll use poll()
     */
    rawnet_arch_tuntap_tun_fd = open(tundev, O_RDWR);
    if (rawnet_arch_tuntap_tun_fd < 0) {
        log_message(rawnet_arch_log, "ERROR opening %s: '%s'", tundev, strerror(errno));
        return 0;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI; /* Ethernet headers | No pkt info */
    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ-1);

    if (ioctl(rawnet_arch_tuntap_tun_fd, TUNSETIFF, (void*)&ifr) < 0) {
        close(rawnet_arch_tuntap_tun_fd);
        log_message(rawnet_arch_log, "ERROR attaching to TAP interface %s: '%s'", interface_name, strerror(errno));
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------------------- */
/*    the architecture-dependend functions                                   */

static void rawnet_arch_tuntap_pre_reset(void)
{
}

static void rawnet_arch_tuntap_post_reset(void)
{
}

static int rawnet_arch_tuntap_activate(const char *interface_name)
{
    if (!rawnet_tuntap_open_adapter(interface_name)) {
        return 0;
    }
    return 1;
}

static void rawnet_arch_tuntap_deactivate(void)
{
    close(rawnet_arch_tuntap_tun_fd);
    rawnet_arch_tuntap_tun_fd = -1;
}

static void rawnet_arch_tuntap_set_mac(const uint8_t mac[6])
{
}

static void rawnet_arch_tuntap_set_hashfilter(const uint32_t hash_mask[2])
{
}

/* int bBroadcast   - broadcast */
/* int bIA          - individual address (IA) */
/* int bMulticast   - multicast if address passes the hash filter */
/* int bCorrect     - accept correct frames */
/* int bPromiscuous - promiscuous mode */
/* int bIAHash      - accept if IA passes the hash filter */

static void rawnet_arch_tuntap_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash)
{
}

static void rawnet_arch_tuntap_line_ctl(int bEnableTransmitter, int bEnableReceiver )
{
}

/** \brief  Transmit a frame
 *
 * \param[in]   force       Delete waiting frames in transmit buffer
 * \param[in]   onecoll     Terminate after just one collision
 * \param[in]   inhibit_crc Do not append CRC to the transmission
 * \param[in]   tx_pad_dis  Disable padding to 60 Bytes
 * \param[in]   txlength    Frame length
 * \param[in]   txframe     Pointer to the frame to be transmitted
 */
static void rawnet_arch_tuntap_transmit(int force, int onecoll, int inhibit_crc,
                                 int tx_pad_dis, int txlength, uint8_t *txframe)
{
    ssize_t res = write(rawnet_arch_tuntap_tun_fd, txframe, txlength);
    if (res < 0) {
        log_message(rawnet_arch_log, "ERROR transmitting frame: '%s'", strerror(errno));
    } else if (res != txlength) {
        /* We should never break a frame */
        log_message(rawnet_arch_log,
                "ERROR transmitting frame: only %" PRI_SSIZE_T " of %d bytes sent",
                res, txlength);
    }
}

/**
 * \brief   Check if a frame was received
 *
 * This function checks if there was a frame received. If so, it returns 1,
 * else 0.
 *
 * If there was no frame, none of the parameters is changed!
 *
 * If there was a frame, the following actions are done:
 *
 * - at maximum \a plen byte are transferred into the buffer given by \a pbuffer
 * - \a plen gets the length of the received frame, EVEN if this is more
 *   than has been copied to \a pbuffer!
 * - if the dest. address was accepted by the hash filter, \a phashed is set,
 *   else cleared.
 * - if the dest. address was accepted by the hash filter, \a phash_index is
 *   set to the number of the rule leading to the acceptance
 * - if the receive was ok (good CRC and valid length), \a *prx_ok is set, else
 *   cleared.
 * - if the dest. address was accepted because it's exactly our MAC address
 *   (set by rawnet_arch_set_mac()), \a pcorrect_mac is set, else cleared.
 * - if the dest. address was accepted since it was a broadcast address,
 *   \a pbroadcast is set, else cleared.
 * - if the received frame had a crc error, \a pcrc_error is set, else cleared
 *
 * \param[out]      buffer          where to store a frame
 * \param[in,out]   plen            IN: maximum length of frame to copy;
 *                                  OUT: length of received frame OUT
 *                                  can be bigger than IN if received frame was
 *                                  longer than supplied buffer
 * \param[out]      phashed         set if the dest. address is accepted by the
 *                                  hash filter
 * \param[out]      phash_index     hash table index if hashed == TRUE
 * \param[out]      prx_ok          set if good CRC and valid length
 * \param[out]      pcorrect_mac    set if dest. address is exactly our IA
 * \param[out]      pbroadcast      set if dest. address is a broadcast address
 * \param[out]      pcrc_error      set if received frame had a CRC error
*/
static int rawnet_arch_tuntap_receive(uint8_t *pbuffer, int *plen, int  *phashed,
        int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast,
        int *pcrc_error)
{
    struct pollfd pfd[1] = {{rawnet_arch_tuntap_tun_fd, POLLIN, 0}};
    ssize_t len;
    int pollres;

    assert((*plen & 1) == 0);

    pollres = poll(pfd, 1, 0);

    if (pollres == 0) {
        return 0;
    } else if (pollres < 0) {
        log_message(rawnet_arch_log, "ERROR polling for new frames: '%s'", strerror(errno));
        return 0;
    }

    if ((pfd[0].revents & (POLLERR | POLLHUP | POLLNVAL)) != 0) {
        log_message(rawnet_arch_log, "WARNING: unexpected event while polling for new frames");
        return 0;
    }

    len = read(rawnet_arch_tuntap_tun_fd, pbuffer, *plen);
    if (len == -1) {
        log_message(rawnet_arch_log, "ERROR receiving frame: '%s'", strerror(errno));
        return 0;
    }

#ifdef RAWNET_DEBUG_PKTDUMP
        rawnet_arch_debug_output("Received frame: ", pbuffer, len);
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */

    if (len & 1) {
        /* This is needed by cs8900.c */
        ++len;
    }
    *plen = (int)len;

    /* We don't decide if this frame fits the needs;
     * by setting all zero, we let tfe.c do the work for us
     */
    *phashed = 0;
    *phash_index = 0;
    *pbroadcast = 0;
    *pcorrect_mac = 0;
    *pcrc_error = 0;
    /* This frame has been received correctly */
    *prx_ok = 1;

    return 1;
}

/** \brief  Find default device on which to capture
 *
 * \return  name of standard interface
 */
static char *rawnet_arch_tuntap_get_standard_interface(void)
{
    return NULL; /* Use the interface configured for cs89000io */
}

/** Finally, let's expose the driver interface */
rawnet_arch_driver_t rawnet_arch_driver_tuntap = {
    "tuntap",
    rawnet_arch_tuntap_pre_reset,
    rawnet_arch_tuntap_post_reset,
    rawnet_arch_tuntap_activate,
    rawnet_arch_tuntap_deactivate,
    rawnet_arch_tuntap_set_mac,
    rawnet_arch_tuntap_set_hashfilter,

    rawnet_arch_tuntap_recv_ctl,

    rawnet_arch_tuntap_line_ctl,

    rawnet_arch_tuntap_transmit,

    rawnet_arch_tuntap_receive,

    rawnet_arch_tuntap_enumadapter_open,
    rawnet_arch_tuntap_enumadapter,
    rawnet_arch_tuntap_enumadapter_close,

    rawnet_arch_tuntap_get_standard_interface
};

#endif /* #ifdef HAVE_TUNTAP */
#endif /* #ifdef HAVE_RAWNET */
