/*
 * rawnetarch.h - raw ethernet interface
 *                 architecture-dependant stuff
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#include <stdint.h>

#ifdef HAVE_RAWNET
#else
  #error RAWNETARCH.H should not be included if HAVE_RAWNET is not defined!
#endif /* #ifdef HAVE_RAWNET */

#ifndef VICE_RAWNETARCH_H
#define VICE_RAWNETARCH_H

/* define this only if VICE should write each and every frame received
   and send into the VICE log
   WARNING: The log grows very fast!
*/
/* #define RAWNET_DEBUG_FRAMES */

#include "log.h"
#include "types.h"

extern int rawnet_arch_resources_init(void);
extern int rawnet_arch_cmdline_options_init(void);
extern void rawnet_arch_resources_shutdown(void);

extern int rawnet_arch_init(void);
extern void rawnet_arch_pre_reset(void);
extern void rawnet_arch_post_reset(void);
extern int rawnet_arch_activate(const char *interface_name);
extern void rawnet_arch_deactivate(void);
extern void rawnet_arch_set_mac(const uint8_t mac[6]);
extern void rawnet_arch_set_hashfilter(const uint32_t hash_mask[2]);

extern void rawnet_arch_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash);

extern void rawnet_arch_line_ctl(int bEnableTransmitter, int bEnableReceiver);

extern void rawnet_arch_transmit(int force, int onecoll, int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe);

extern int rawnet_arch_receive(uint8_t *pbuffer, int *plen, int *phashed, int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast, int *pcrc_error);

extern int rawnet_arch_enumadapter_open(void);
extern int rawnet_arch_enumadapter(char **ppname, char **ppdescription);
extern int rawnet_arch_enumadapter_close(void);
extern char *rawnet_arch_get_standard_interface(void);

extern int rawnet_arch_enumdriver_open(void);
extern int rawnet_arch_enumdriver(char **ppname, char **ppdescription);
extern int rawnet_arch_enumdriver_close(void);
extern char *rawnet_arch_get_standard_driver(void);

#ifdef UNIX_COMPILE

/** #define RAWNET_DEBUG_ARCH 1 **/
/** #define RAWNET_DEBUG_PKTDUMP 1 **/

#ifdef RAWNET_DEBUG_PKTDUMP
extern void rawnet_arch_debug_output(const char *text, uint8_t *what, int count);
#endif

/* Logging device to be used by rawnet drivers */
extern log_t rawnet_arch_log;

/* Under Unix we can have various rawnet drivers, and each exposes its
 * interface by instantiating a rawnet_driver_t structure */
typedef struct rawnet_arch_driver_s {
    const char *name;
    void (*pre_reset)(void);
    void (*post_reset)(void);
    int (*activate)(const char *interface_name);
    void (*deactivate)(void);
    void (*set_mac)(const uint8_t mac[6]);
    void (*set_hashfilter)(const uint32_t hash_mask[2]);

    void (*recv_ctl)(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash);

    void (*line_ctl)(int bEnableTransmitter, int bEnableReceiver);

    void (*transmit)(int force, int onecoll, int inhibit_crc, int tx_pad_dis, int txlength, uint8_t *txframe);

    int (*receive)(uint8_t *pbuffer, int *plen, int *phashed, int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast, int *pcrc_error);

    int (*enumadapter_open)(void);
    int (*enumadapter)(char **ppname, char **ppdescription);
    int (*enumadapter_close)(void);

    char *(*get_standard_interface)(void);
} rawnet_arch_driver_t;

#ifdef HAVE_PCAP
extern rawnet_arch_driver_t rawnet_arch_driver_pcap;
#endif
#ifdef HAVE_TUNTAP
extern rawnet_arch_driver_t rawnet_arch_driver_tuntap;
#endif

#endif /* ifdef UNIX_COMPILE */

#endif
