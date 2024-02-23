/** \file   rawnetarch_win32.c
 * \brief   Raw ethernet interface, Win32 stuff
 *
 * \author  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifdef HAVE_PCAP

/* #define WPCAP */

/*
    To compile with PCAP support, the respective header files must be present.
    These Headers are NOT in the VICE tree anymore, but have to be provided
    when compiling. One easy way to do this is installing libpcap in msys2. The
    more "correct" way would be downloading the SDK from the npcap website at
    https://npcap.com/#download and point the CFLAGS at them when configuring.

    To actually use the resulting binary with npcap, simple install it using
    the installer from https://npcap.com/#download (select winpcap compatible
    mode) and then run the emulator.
*/
#include <pcap.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "log.h"
#include "rawnet.h"
#include "rawnetarch.h"

typedef pcap_t *(*pcap_open_live_t)(const char *, int, int, int, char *);
typedef int (*pcap_dispatch_t)(pcap_t *, int, pcap_handler, u_char *);
typedef int (*pcap_setnonblock_t)(pcap_t *, int, char *);
typedef int (*pcap_datalink_t)(pcap_t *);
typedef int (*pcap_findalldevs_t)(pcap_if_t **, char *);
typedef void (*pcap_freealldevs_t)(pcap_if_t *);
typedef int (*pcap_sendpacket_t)(pcap_t *p, u_char *buf, int size);
typedef char *(*pcap_lookupdev_t)(char *);

/** #define RAWNET_DEBUG_ARCH 1 **/
/** #define RAWNET_DEBUG_PKTDUMP 1 **/

/* #define RAWNET_DEBUG_FRAMES - might be defined in rawnetarch.h ! */

#define RAWNET_DEBUG_WARN 1 /* this should not be deactivated */

static pcap_open_live_t p_pcap_open_live;
static pcap_dispatch_t p_pcap_dispatch;
static pcap_setnonblock_t p_pcap_setnonblock;
static pcap_findalldevs_t p_pcap_findalldevs;
static pcap_freealldevs_t p_pcap_freealldevs;
static pcap_sendpacket_t p_pcap_sendpacket;
static pcap_datalink_t p_pcap_datalink;
static pcap_lookupdev_t p_pcap_lookupdev;

static HINSTANCE pcap_library = NULL;

/* ------------------------------------------------------------------------- */
/*    variables needed                                                       */

static log_t rawnet_arch_log = LOG_ERR;

static pcap_if_t *EthernetPcapNextDev = NULL;
static pcap_if_t *EthernetPcapAlldevs = NULL;
static pcap_t *EthernetPcapFP = NULL;

static char EthernetPcapErrbuf[PCAP_ERRBUF_SIZE];

#ifdef RAWNET_DEBUG_PKTDUMP

static void debug_output(const char *text, uint8_t *what, int count)
{
    char buffer[256];
    char *p = buffer;
    char *pbuffer1 = what;
    int len1 = count;
    int i;

    sprintf(buffer, "\n%s: length = %u\n", text, len1);
    OutputDebugString(buffer);
    do {
        p = buffer;
        for (i = 0; (i < 8) && len1 > 0; len1--, i++) {
            sprintf( p, "%02x ", (unsigned int)(unsigned char)*pbuffer1++);
            p += 3;
        }
        *(p - 1) = '\n';
        *p = 0;
        OutputDebugString(buffer);
    } while (len1 > 0);
}
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */


static void EthernetPcapFreeLibrary(void)
{
    if (pcap_library) {
        if (!FreeLibrary(pcap_library)) {
            log_message(rawnet_arch_log, "FreeLibrary WPCAP.DLL failed!");
        }
        pcap_library = NULL;

        p_pcap_open_live = NULL;
        p_pcap_dispatch = NULL;
        p_pcap_setnonblock = NULL;
        p_pcap_findalldevs = NULL;
        p_pcap_freealldevs = NULL;
        p_pcap_sendpacket = NULL;
        p_pcap_datalink = NULL;
        p_pcap_lookupdev = NULL;
    }
}

/* since I don't like typing too much... */
#define GET_PROC_ADDRESS_AND_TEST( _name_ )                              \
    p_##_name_ = (_name_##_t)(void*)GetProcAddress(pcap_library, #_name_);     \
    if (!p_##_name_ ) {                                                  \
        log_message(rawnet_arch_log, "GetProcAddress " #_name_ " failed!"); \
        EthernetPcapFreeLibrary();                                            \
        return FALSE;                                                    \
    }

static BOOL EthernetPcapLoadLibrary(void)
{
    if (!pcap_library) {
        pcap_library = LoadLibrary(TEXT("wpcap.dll"));

        if (!pcap_library) {
            log_message(rawnet_arch_log, "LoadLibrary WPCAP.DLL failed!");
            return FALSE;
        }

        GET_PROC_ADDRESS_AND_TEST(pcap_open_live);
        GET_PROC_ADDRESS_AND_TEST(pcap_dispatch);
        GET_PROC_ADDRESS_AND_TEST(pcap_setnonblock);
        GET_PROC_ADDRESS_AND_TEST(pcap_findalldevs);
        GET_PROC_ADDRESS_AND_TEST(pcap_freealldevs);
        GET_PROC_ADDRESS_AND_TEST(pcap_sendpacket);
        GET_PROC_ADDRESS_AND_TEST(pcap_datalink);
        GET_PROC_ADDRESS_AND_TEST(pcap_lookupdev);
    }

    return TRUE;
}

#undef GET_PROC_ADDRESS_AND_TEST


/*
 These functions let the UI enumerate the available interfaces.

 First, rawnet_arch_enumadapter_open() is used to start enumeration.

 rawnet_arch_enumadapter is then used to gather information for each adapter present
 on the system, where:

   ppname points to a pointer which will hold the name of the interface
   ppdescription points to a pointer which will hold the description of the interface

   For each of these parameters, new memory is allocated, so it has to be
   freed with lib_free().

 rawnet_arch_enumadapter_close() must be used to stop processing.

 Each function returns 1 on success, and 0 on failure.
 rawnet_arch_enumadapter() only fails if there is no more adpater; in this case,
   *ppname and *ppdescription are not altered.
*/
int rawnet_arch_enumadapter_open(void)
{
    if (!EthernetPcapLoadLibrary()) {
        return 0;
    }

    if ((*p_pcap_findalldevs)(&EthernetPcapAlldevs, EthernetPcapErrbuf) == -1) {
        log_message(rawnet_arch_log, "ERROR in rawnet_arch_enumadapter_open: pcap_findalldevs: '%s'", EthernetPcapErrbuf);
        return 0;
    }

    if (!EthernetPcapAlldevs) {
        log_message(rawnet_arch_log, "ERROR in rawnet_arch_enumadapter_open, finding all pcap devices - Do we have the necessary privilege rights?");
        return 0;
    }

    EthernetPcapNextDev = EthernetPcapAlldevs;

    return 1;
}

int rawnet_arch_enumadapter(char **ppname, char **ppdescription)
{
    if (!EthernetPcapNextDev) {
        return 0;
    }

    if (EthernetPcapNextDev->name == NULL) {
        fprintf(stderr, "%s:%d:%s(): adapter name is NULL",
                __FILE__, __LINE__, __func__);
        log_error(LOG_ERR, "adapter name is NULL");
        return 0;
    }
    *ppname = lib_strdup(EthernetPcapNextDev->name);

    if (EthernetPcapNextDev->description == NULL) {
        fprintf(stderr, "%s:%d:%s(): adapter description is NULL",
                __FILE__, __LINE__, __func__);
        log_error(LOG_ERR, "adapter description is NULL");
        return 0;
    }
    *ppdescription = lib_strdup(EthernetPcapNextDev->description);

    EthernetPcapNextDev = EthernetPcapNextDev->next;

    return 1;
}

int rawnet_arch_enumadapter_close(void)
{
    if (EthernetPcapAlldevs) {
        (*p_pcap_freealldevs)(EthernetPcapAlldevs);
        EthernetPcapAlldevs = NULL;
    }
    return 1;
}

static BOOL EthernetPcapOpenAdapter(const char *interface_name)
{
    pcap_if_t *EthernetPcapDevice = NULL;

    if (!rawnet_enumadapter_open()) {
        return FALSE;
    } else {
        /* look if we can find the specified adapter */
        char *pname;
        char *pdescription;
        BOOL  found = FALSE;

        if (interface_name) {
            /* we have an interface name, try it */
            EthernetPcapDevice = EthernetPcapAlldevs;

            while (rawnet_enumadapter(&pname, &pdescription)) {
                if (strcmp(pname, interface_name) == 0) {
                    found = TRUE;
                }
                lib_free(pname);
                lib_free(pdescription);
                if (found) break;
                EthernetPcapDevice = EthernetPcapNextDev;
            }
        }

        if (!found) {
            /* just take the first adapter */
            EthernetPcapDevice = EthernetPcapAlldevs;
        }
    }

    EthernetPcapFP = (*p_pcap_open_live)(EthernetPcapDevice->name, 1700, 1, 20, EthernetPcapErrbuf);
    if (EthernetPcapFP == NULL) {
        log_message(rawnet_arch_log, "ERROR opening adapter: '%s'", EthernetPcapErrbuf);
        rawnet_enumadapter_close();
        return FALSE;
    }

    if ((*p_pcap_setnonblock)(EthernetPcapFP, 1, EthernetPcapErrbuf) < 0) {
        log_message(rawnet_arch_log, "WARNING: Setting PCAP to non-blocking failed: '%s'", EthernetPcapErrbuf);
    }

    /* Check the link layer. We support only Ethernet for simplicity. */
    if ((*p_pcap_datalink)(EthernetPcapFP) != DLT_EN10MB) {
        log_message(rawnet_arch_log, "ERROR: Ethernet works only on Ethernet networks.");
        rawnet_enumadapter_close();
        return FALSE;
    }

    rawnet_enumadapter_close();
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/*    the architecture-dependent functions                                   */

int rawnet_arch_init(void)
{
    rawnet_arch_log = log_open("EthernetARCH");

    if (!EthernetPcapLoadLibrary()) {
        return 0;
    }

    return 1;
}

void rawnet_arch_pre_reset( void )
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_pre_reset().");
#endif
}

void rawnet_arch_post_reset( void )
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_post_reset().");
#endif
}

int rawnet_arch_activate(const char *interface_name)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_activate().");
#endif
    if (!EthernetPcapOpenAdapter(interface_name)) {
        return 0;
    }
    return 1;
}

void rawnet_arch_deactivate( void )
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_deactivate().");
#endif
}

void rawnet_arch_set_mac( const uint8_t mac[6] )
{
#if defined(RAWNET_DEBUG_ARCH) || defined(RAWNET_DEBUG_FRAMES)
    log_message(rawnet_arch_log, "New MAC address set: %02X:%02X:%02X:%02X:%02X:%02X.", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif
}

void rawnet_arch_set_hashfilter(const uint32_t hash_mask[2])
{
#if defined(RAWNET_DEBUG_ARCH) || defined(RAWNET_DEBUG_FRAMES)
    log_message(rawnet_arch_log, "New hash filter set: %08X:%08X.", hash_mask[1], hash_mask[0]);
#endif
}

/* int bBroadcast   - broadcast */
/* int bIA          - individual address (IA) */
/* int bMulticast   - multicast if address passes the hash filter */
/* int bCorrect     - accept correct frames */
/* int bPromiscuous - promiscuous mode */
/* int bIAHash      - accept if IA passes the hash filter */


void rawnet_arch_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash)
{
#if defined(RAWNET_DEBUG_ARCH) || defined(RAWNET_DEBUG_FRAMES)
    log_message(rawnet_arch_log, "rawnet_arch_recv_ctl() called with the following parameters:" );
    log_message(rawnet_arch_log, "\tbBroadcast   = %s", bBroadcast ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbIA          = %s", bIA ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbMulticast   = %s", bMulticast ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbCorrect     = %s", bCorrect ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbPromiscuous = %s", bPromiscuous ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbIAHash      = %s", bIAHash ? "TRUE" : "FALSE" );
#endif
}

void rawnet_arch_line_ctl(int bEnableTransmitter, int bEnableReceiver)
{
#if defined(RAWNET_DEBUG_ARCH) || defined(RAWNET_DEBUG_FRAMES)
    log_message(rawnet_arch_log, "rawnet_arch_line_ctl() called with the following parameters:" );
    log_message(rawnet_arch_log, "\tbEnableTransmitter = %s", bEnableTransmitter ? "TRUE" : "FALSE" );
    log_message(rawnet_arch_log, "\tbEnableReceiver    = %s", bEnableReceiver ? "TRUE" : "FALSE" );
#endif
}

typedef struct Ethernet_PCAP_internal_s {
    unsigned int len;
    uint8_t *buffer;
} Ethernet_PCAP_internal_t;

/* Callback function invoked by libpcap for every incoming packet */
static void EthernetPcapPacketHandler(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
    Ethernet_PCAP_internal_t *pinternal = (void*)param;

    /* determine the count of bytes which has been returned,
     * but make sure not to overrun the buffer
     */
    if (header->caplen < pinternal->len) {
        pinternal->len = header->caplen;
    }

    memcpy(pinternal->buffer, pkt_data, pinternal->len);
}

/* the following function receives a frame.

   If there's none, it returns a -1.
   If there is one, it returns the length of the frame in bytes.

   It copies the frame to *buffer and returns the number of copied
   bytes as return value.

   At most 'len' bytes are copied.
*/
static int rawnet_arch_receive_frame(Ethernet_PCAP_internal_t *pinternal)
{
    int ret = -1;

    /* check if there is something to receive */
    if ((*p_pcap_dispatch)(EthernetPcapFP, 1, EthernetPcapPacketHandler, (void*)pinternal) != 0) {
        /* Something has been received */
        ret = pinternal->len;
    }

#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_receive_frame() called, returns %d.", ret);
#endif

    return ret;
}

/* int force       - FORCE: Delete waiting frames in transmit buffer */
/* int onecoll     - ONECOLL: Terminate after just one collision */
/* int inhibit_crc - INHIBITCRC: Do not append CRC to the transmission */
/* int tx_pad_dis  - TXPADDIS: Disable padding to 60 Bytes */
/* int txlength    - Frame length */
/* uint8_t *txframe   - Pointer to the frame to be transmitted */

void rawnet_arch_transmit(int force, int onecoll, int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe)
{
#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_transmit() called, with: force = %s, onecoll = %s, inhibit_crc=%s, tx_pad_dis=%s, txlength=%u",
                force ? "TRUE" : "FALSE",
                onecoll ? "TRUE" : "FALSE",
                inhibit_crc ? "TRUE" : "FALSE",
                tx_pad_dis ?  "TRUE" : "FALSE",
                txlength);
#endif

#ifdef RAWNET_DEBUG_PKTDUMP
    debug_output("Transmit frame: ", txframe, txlength);
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */

    if ((*p_pcap_sendpacket)(EthernetPcapFP, txframe, txlength) == -1) {
        log_message(rawnet_arch_log, "WARNING! Could not send packet!");
    }
}

/*
  rawnet_arch_receive()

  This function checks if there was a frame received.
  If so, it returns 1, else 0.

  If there was no frame, none of the parameters is changed!

  If there was a frame, the following actions are done:

  - at maximum *plen byte are transferred into the buffer given by pbuffer
  - *plen gets the length of the received frame, EVEN if this is more
    than has been copied to pbuffer!
  - if the dest. address was accepted by the hash filter, *phashed is set, else
    cleared.
  - if the dest. address was accepted by the hash filter, *phash_index is
    set to the number of the rule leading to the acceptance
  - if the receive was ok (good CRC and valid length), *prx_ok is set,
    else cleared.
  - if the dest. address was accepted because it's exactly our MAC address
    (set by rawnet_arch_set_mac()), *pcorrect_mac is set, else cleared.
  - if the dest. address was accepted since it was a broadcast address,
    *pbroadcast is set, else cleared.
  - if the received frame had a crc error, *pcrc_error is set, else cleared
*/

/* uint8_t *pbuffer     - where to store a frame */
/* int *plen         - IN: maximum length of frame to copy;
                       OUT: length of received frame
                            OUT can be bigger than IN if received frame was
                            longer than supplied buffer */
/* int *phashed      - set if the dest. address is accepted by the hash filter */
/* int *phash_index  - hash table index if hashed == TRUE */
/* int *prx_ok       - set if good CRC and valid length */
/* int *pcorrect_mac - set if dest. address is exactly our IA */
/* int *pbroadcast   - set if dest. address is a broadcast address */
/* int *pcrc_error   - set if received frame had a CRC error */

int rawnet_arch_receive(uint8_t *pbuffer, int *plen, int *phashed, int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast, int *pcrc_error)
{
    int len;

    Ethernet_PCAP_internal_t internal;

    internal.len = *plen;
    internal.buffer = pbuffer;

#ifdef RAWNET_DEBUG_ARCH
    log_message(rawnet_arch_log, "rawnet_arch_receive() called, with *plen=%u.", *plen);
#endif

    assert((*plen & 1) == 0);

    len = rawnet_arch_receive_frame(&internal);

    if (len != -1) {

#ifdef RAWNET_DEBUG_PKTDUMP
        debug_output("Received frame: ", internal.buffer, internal.len);
#endif /* #ifdef RAWNET_DEBUG_PKTDUMP */

        if (len & 1) {
            ++len;
        }

        *plen = len;

        /* we don't decide if this frame fits the needs;
         * by setting all zero, we let ethernet.c do the work
         * for us
         */
        *phashed = *phash_index = *pbroadcast = *pcorrect_mac = *pcrc_error = 0;

        /* this frame has been received correctly */
        *prx_ok = 1;

        return 1;
    }

    return 0;
}

char *rawnet_arch_get_standard_interface(void)
{
    char *dev, errbuf[PCAP_ERRBUF_SIZE];

    if (!EthernetPcapLoadLibrary()) {
        return NULL;
    }

    dev = (*p_pcap_lookupdev)(errbuf);

    if (!dev) {
        log_message(rawnet_arch_log, "Failed to get standard network interface: %s\n", errbuf);
        return NULL;
    }

    return lib_strdup(dev);
}

int rawnet_arch_resources_init(void)
{
    return 0;
}

int rawnet_arch_cmdline_options_init(void)
{
    return 0;
}

void rawnet_arch_resources_shutdown(void)
{

}

#endif /* #ifdef HAVE_PCAP */
