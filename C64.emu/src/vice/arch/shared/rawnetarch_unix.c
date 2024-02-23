/** \file   rawnetarch_unix.c
 * \brief   Raw ethernet driver for Unix, based on libpcap.
 *
 * \author  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#ifdef HAVE_PCAP

/*
 * if we have a pcap version with either pcap_sendpacket or pcap_inject,
 * do not use libnet anymore!
 */
#if defined(HAVE_PCAP_SENDPACKET) || defined(HAVE_PCAP_INJECT)
#undef HAVE_LIBNET
#endif

#include "pcap.h"

#ifdef HAVE_LIBNET
#include "libnet.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "rawnetarch.h"

/*
 *  FIXME:  rename all remaining tfe_ stuff to rawnet_
 */

#define RAWNET_DEBUG_WARN 1 /* this should not be deactivated
                             * If this should not be deactived, why is this
                             * here at all? --compyx
                             */


/** \brief  Only select devices that are PCAP_IF_UP
 *
 * Since on Linux pcap_findalldevs() returns all interfaces, including special
 * kernal devices such as nfqueue, filtering the list returned by pcap makes
 * sense. Should this filtering cause trouble on other Unices, this define can
 * be guarded with #ifdef SOME_UNIX_VERSION to disable the filtering.
 */
#ifdef PCAP_IF_UP
#define RAWNET_ONLY_IF_UP
#endif

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

/** \brief  Iterator for the list returned by pcap_findalldevs()
 */
static pcap_if_t *rawnet_pcap_dev_iter = NULL;


/** \brief  Device list returned by pcap_findalldevs()
 *
 * Can be `NULL` since pcap_findalldevs() considers not finding any devices a
 * succesful outcome.
 */
static pcap_if_t *rawnet_pcap_dev_list = NULL;


static pcap_t *rawnet_pcap_fp = NULL;

#ifdef HAVE_LIBNET
#ifdef VICE_USE_LIBNET_1_1
static libnet_t *TfeLibnetFP = NULL;
#else /* VICE_USE_LIBNET_1_1 */
static struct libnet_link_int *TfeLibnetFP = NULL;
#endif /* VICE_USE_LIBNET_1_1 */

static char TfeLibnetErrBuf[LIBNET_ERRBUF_SIZE];

#endif /* HAVE_LIBNET */


/** \brief  Buffer for pcap error messages
 */
static char rawnet_pcap_errbuf[PCAP_ERRBUF_SIZE];

static int rawnet_arch_pcap_enumadapter_open(void)
{
    if (pcap_findalldevs(&rawnet_pcap_dev_list, rawnet_pcap_errbuf) == -1) {
        log_message(rawnet_arch_log,
                "ERROR in rawnet_arch_enumadapter_open: pcap_findalldevs: '%s'",
                rawnet_pcap_errbuf);
        return 0;
    }

    if (!rawnet_pcap_dev_list) {
        log_message(rawnet_arch_log,
                "ERROR in rawnet_arch_enumadapter_open, finding all pcap "
                "devices - Do we have the necessary privilege rights?");
        return 0;
    }

    rawnet_pcap_dev_iter = rawnet_pcap_dev_list;
    return 1;
}


/** \brief  Get current pcap device iterator values
 *
 * The \a ppname and \a ppdescription are heap-allocated via lib_strdup()
 * and should thus be freed after use with lib_free(). Please not that
 * \a ppdescription can be `NULL` due to pcap_if_t->description being `NULL`,
 * so check against `NULL` before using it. Calling lib_free() on it is safe
 * though, free(`NULL`) is guaranteed to just do nothing.
 *
 * \param[out]  ppname          device name
 * \param[out]  ppdescription   device description
 *
 * \return  bool (1 on success, 0 on failure)
 */
static int rawnet_arch_pcap_enumadapter(char **ppname, char **ppdescription)
{
#ifdef RAWNET_ONLY_IF_UP
    /* only select devices that are up */
    while (rawnet_pcap_dev_iter != NULL
            && !(rawnet_pcap_dev_iter->flags & PCAP_IF_UP)) {
        rawnet_pcap_dev_iter = rawnet_pcap_dev_iter->next;
    }
#endif

    if (rawnet_pcap_dev_iter == NULL) {
        return 0;
    }

    *ppname = lib_strdup(rawnet_pcap_dev_iter->name);
    /* carefull: pcap_if_t->description can be NULL and lib_strdup() fails on
     * passing `NULL` */
    if (rawnet_pcap_dev_iter->description != NULL) {
        *ppdescription = lib_strdup(rawnet_pcap_dev_iter->description);
    } else {
        *ppdescription = NULL;
    }

    rawnet_pcap_dev_iter = rawnet_pcap_dev_iter->next;

    return 1;
}

static int rawnet_arch_pcap_enumadapter_close(void)
{
    if (rawnet_pcap_dev_list) {
        pcap_freealldevs(rawnet_pcap_dev_list);
        rawnet_pcap_dev_list = NULL;
    }
    return 1;
}

static int rawnet_pcap_open_adapter(const char *interface_name)
{
    rawnet_pcap_fp = pcap_open_live((char*)interface_name, 1700, 1, 20, rawnet_pcap_errbuf);
    if ( rawnet_pcap_fp == NULL) {
        log_message(rawnet_arch_log, "ERROR opening adapter: '%s'", rawnet_pcap_errbuf);
        return 0;
    }

    if (pcap_setnonblock(rawnet_pcap_fp, 1, rawnet_pcap_errbuf) < 0) {
        log_message(rawnet_arch_log, "WARNING: Setting PCAP to non-blocking failed: '%s'", rawnet_pcap_errbuf);
    }

    /* Check the link layer. We support only Ethernet for simplicity. */
    if (pcap_datalink(rawnet_pcap_fp) != DLT_EN10MB) {
        log_message(rawnet_arch_log, "ERROR: TFE works only on Ethernet networks.");
        return 0;
    }

#ifdef HAVE_LIBNET
    /* now, open the libnet device to be able to send afterwards */
#ifdef VICE_USE_LIBNET_1_1
    TfeLibnetFP = libnet_init(LIBNET_LINK, (char *)interface_name, TfeLibnetErrBuf);
#else /* VICE_USE_LIBNET_1_1 */
    TfeLibnetFP = libnet_open_link_interface(interface_name, TfeLibnetErrBuf);
#endif /* VICE_USE_LIBNET_1_1 */

    if (TfeLibnetFP == NULL) {
        log_message(rawnet_arch_log, "Libnet interface could not be opened: '%s'", TfeLibnetErrBuf);

        if (rawnet_pcap_fp) {
            pcap_close(rawnet_pcap_fp);
            rawnet_pcap_fp = NULL;
        }
        return 0;
    }
#endif /* HAVE_LIBNET */

    return 1;
}

/* ------------------------------------------------------------------------- */
/*    the architecture-dependend functions                                   */

static void rawnet_arch_pcap_pre_reset(void)
{
}

static void rawnet_arch_pcap_post_reset(void)
{
}

static int rawnet_arch_pcap_activate(const char *interface_name)
{
    if (!rawnet_pcap_open_adapter(interface_name)) {
        return 0;
    }
    return 1;
}

static void rawnet_arch_pcap_deactivate( void )
{
}

static void rawnet_arch_pcap_set_mac( const uint8_t mac[6] )
{
}

static void rawnet_arch_pcap_set_hashfilter(const uint32_t hash_mask[2])
{
}

/* int bBroadcast   - broadcast */
/* int bIA          - individual address (IA) */
/* int bMulticast   - multicast if address passes the hash filter */
/* int bCorrect     - accept correct frames */
/* int bPromiscuous - promiscuous mode */
/* int bIAHash      - accept if IA passes the hash filter */

static void rawnet_arch_pcap_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash)
{
}

static void rawnet_arch_pcap_line_ctl(int bEnableTransmitter, int bEnableReceiver )
{
}


/** \brief  Raw pcap packet
 */
typedef struct rawnet_pcap_internal_s {
    unsigned int len;   /**< length of packet data */
    uint8_t *buffer;    /**< packet data */
} rawnet_pcap_internal_t;


/** \brief  Callback function invoked by libpcap for every incoming packet
 *
 * \param[in,out]   param       reference to internal VICE packet struct
 * \param[in]       header      pcap header
 * \param[in]       pkt_data    packet data
 */
static void rawnet_pcap_packet_handler(u_char *param,
        const struct pcap_pkthdr *header, const u_char *pkt_data)
{
    rawnet_pcap_internal_t *pinternal = (void*)param;

    /* determine the count of bytes which has been returned,
     * but make sure not to overrun the buffer
     */
    if (header->caplen < pinternal->len) {
        pinternal->len = header->caplen;
    }

    memcpy(pinternal->buffer, pkt_data, pinternal->len);
}


/** \brief  Receives a frame
 *
 * If there's none, it returns a -1 in \a pinternal->len, if there is one,
 * it returns the length of the frame in bytes in \a pinternal->len.
 *
 * It copies the frame to \a buffer and returns the number of copied bytes as
 * the return value.
 *
 * \param[in,out]   pinternal   internal VICE packet struct
 *
 * \note    At most 'len' bytes are copied.
 *
 * \return  number of bytes copied or -1 on failure
 */
static int rawnet_arch_pcap_receive_frame(rawnet_pcap_internal_t *pinternal)
{
    int ret = -1;

    /* check if there is something to receive */
    if (pcap_dispatch(rawnet_pcap_fp, 1, rawnet_pcap_packet_handler,
                (void*)pinternal) != 0) {
        /* Something has been received */
        ret = pinternal->len;
    }

#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log,
            "rawnet_arch_receive_frame() called, returns %d.", ret);
#endif

    return ret;
}

#ifdef HAVE_LIBNET

# ifdef VICE_USE_LIBNET_1_1

#  define RAWNET_ARCH_TRANSMIT rawnet_arch_transmit_libnet_1_1

static void rawnet_arch_transmit_libnet_1_1(int force, int onecoll,
        int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe)
{
    /* we want to send via libnet */

    do {
        libnet_pblock_t *p;

        p = libnet_pblock_new(TfeLibnetFP, txlength);

        if (p == NULL) {
            log_message(rawnet_arch_log,
                    "WARNING! Could not send packet, libnet_pblock_probe() failed!");
            break;
        }

        if ( libnet_pblock_append(TfeLibnetFP, p, txframe, txlength) == -1 ) {
            log_message(rawnet_arch_log,
                    "WARNING! Could not send packet, libnet_pblock_append() failed!");
            break;
        }

        libnet_pblock_update(TfeLibnetFP, p, 0, LIBNET_PBLOCK_ETH_H);

        if ( libnet_write(TfeLibnetFP) == -1 ) {
            log_message(rawnet_arch_log,
                    "WARNING! Could not send packet, libnet_write() failed!");
            break;
        }

        libnet_pblock_delete(TfeLibnetFP, p);

    } while (0);
}

# else /* VICE_USE_LIBNET_1_1 */

#  define RAWNET_ARCH_TRANSMIT rawnet_arch_transmit_libnet_1_0

static void rawnet_arch_transmit_libnet_1_0(int force, int onecoll,
        int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe)
{
    u_char *plibnet_buffer = NULL;

    /* we want to send via libnet 1.0 */

    if (libnet_init_packet(txlength, &plibnet_buffer)==-1) {
        log_message(rawnet_arch_log, "WARNING! Could not send packet!");
    } else {
        if (plibnet_buffer) {
            memcpy(plibnet_buffer, txframe, txlength);
            libnet_write_link_layer(TfeLibnetFP, "eth0", plibnet_buffer, txlength);
            libnet_destroy_packet(&plibnet_buffer);
        } else {
            log_message(rawnet_arch_log,
                    "WARNING! Could not send packet: plibnet_buffer==NULL, "
                    "but libnet_init_packet() did NOT fail!!");
        }
    }

}

# endif

#else /* HAVE_LIBNET */

#  define RAWNET_ARCH_TRANSMIT rawnet_arch_transmit_pcap

 #if defined(HAVE_PCAP_INJECT)
  #define PCAP_INJECT pcap_inject
 #elif defined(HAVE_PCAP_SENDPACKET)
  #define PCAP_INJECT pcap_sendpacket
 #else
  #error SHOULD NOT HAPPEN: No libnet, but neither HAVE_PCAP_SENDPACKET nor HAVE_PCAP_INJECT are defined!
 #endif


/** \brief  Transmit a packet(?) via pcap
 *
 */
static void rawnet_arch_transmit_pcap(int force, int onecoll, int inhibit_crc,
        int tx_pad_dis, int txlength, uint8_t *txframe)
{
    /* we want to send via pcap */

    if (PCAP_INJECT(rawnet_pcap_fp, txframe, txlength) < 0) {
        log_message(rawnet_arch_log, "WARNING! Could not send packet!");
    }
}

#endif /* HAVE_LIBNET */


/** \brief  Transmit a frame
 *
 * \param[in]   force       Delete waiting frames in transmit buffer
 * \param[in]   onecoll     Terminate after just one collision
 * \param[in]   inhibit_crc Do not append CRC to the transmission
 * \param[in]   tx_pad_dis  Disable padding to 60 Bytes
 * \param[in]   txlength    Frame length
 * \param[in]   txframe     Pointer to the frame to be transmitted
 */
static void rawnet_arch_pcap_transmit(int force, int onecoll, int inhibit_crc,
                               int tx_pad_dis, int txlength, uint8_t *txframe)
{
    RAWNET_ARCH_TRANSMIT(force, onecoll, inhibit_crc, tx_pad_dis, txlength,
            txframe);
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
 * \param[out[      pbroadcast      set if dest. address is a broadcast address
 * \param[out]      pcrc_error      set if received frame had a CRC error
*/
static int rawnet_arch_pcap_receive(uint8_t *pbuffer, int *plen, int  *phashed,
        int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast,
        int *pcrc_error)
{
    int len;

    rawnet_pcap_internal_t internal = { *plen, pbuffer };

    assert((*plen & 1) == 0);

    len = rawnet_arch_pcap_receive_frame(&internal);

    if (len != -1) {

#ifdef RAWNET_DEBUG_PKTDUMP
        rawnet_arch_debug_output("Received frame: ", internal.buffer, internal.len);
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */

        if (len & 1) {
            ++len;
        }

        *plen = len;

        /* we don't decide if this frame fits the needs;
         * by setting all zero, we let tfe.c do the work
         * for us
         */
        *phashed =
        *phash_index =
        *pbroadcast =
        *pcorrect_mac =
        *pcrc_error = 0;

        /* this frame has been received correctly */
        *prx_ok = 1;

        return 1;
    }

    return 0;
}


/** \brief  Find default device on which to capture
 *
 * \return  name of standard interface
 *
 * \note    pcap_lookupdev() has been deprecated, so the correct way to get
 *          the default device is to use the first entry returned by
 *          pcap_findalldevs().
 *          See http://www.tcpdump.org/manpages/pcap_lookupdev.3pcap.html
 *
 * \return  default interface name or `NULL` when not found
 *
 * \note    free the returned value with lib_free() if not `NULL`
 */
static char *rawnet_arch_pcap_get_standard_interface(void)
{
    char *dev = NULL;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_if_t *list;

    if (pcap_findalldevs(&list, errbuf) == 0 && list != NULL) {
        dev = lib_strdup(list[0].name);
        pcap_freealldevs(list);
    }
    return dev;
}

/** Finally, let's expose the driver interface */
rawnet_arch_driver_t rawnet_arch_driver_pcap = {
    "pcap",
    rawnet_arch_pcap_pre_reset,
    rawnet_arch_pcap_post_reset,
    rawnet_arch_pcap_activate,
    rawnet_arch_pcap_deactivate,
    rawnet_arch_pcap_set_mac,
    rawnet_arch_pcap_set_hashfilter,

    rawnet_arch_pcap_recv_ctl,

    rawnet_arch_pcap_line_ctl,

    rawnet_arch_pcap_transmit,

    rawnet_arch_pcap_receive,

    rawnet_arch_pcap_enumadapter_open,
    rawnet_arch_pcap_enumadapter,
    rawnet_arch_pcap_enumadapter_close,

    rawnet_arch_pcap_get_standard_interface
};

#endif /* #ifdef HAVE_PCAP */
#endif /* #ifdef HAVE_RAWNET */
