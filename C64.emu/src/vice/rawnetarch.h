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

#ifdef HAVE_TFE
#else
  #error RAWNETARCH.H should not be included if HAVE_TFE is not defined!
#endif /* #ifdef HAVE_TFE */

#ifndef VICE_RAWNETARCH_H
#define VICE_RAWNETARCH_H

/* define this only if VICE should write each and every frame received
   and send into the VICE log
   WARNING: The log grows very fast!
*/
/* #define RAWNET_DEBUG_FRAMES */

#include "types.h"

extern int rawnet_arch_init(void);
extern void rawnet_arch_pre_reset(void);
extern void rawnet_arch_post_reset(void);
extern int rawnet_arch_activate(const char *interface_name);
extern void rawnet_arch_deactivate(void);
extern void rawnet_arch_set_mac(const BYTE mac[6]);
extern void rawnet_arch_set_hashfilter(const DWORD hash_mask[2]);

extern void rawnet_arch_recv_ctl(int bBroadcast, int bIA, int bMulticast, int bCorrect, int bPromiscuous, int bIAHash);

extern void rawnet_arch_line_ctl(int bEnableTransmitter, int bEnableReceiver);

extern void rawnet_arch_transmit(int force, int onecoll, int inhibit_crc, int tx_pad_dis, int txlength, BYTE *txframe);

extern int rawnet_arch_receive(BYTE *pbuffer, int *plen, int *phashed, int *phash_index, int *prx_ok, int *pcorrect_mac, int *pbroadcast, int *pcrc_error);

extern int rawnet_arch_enumadapter_open(void);
extern int rawnet_arch_enumadapter(char **ppname, char **ppdescription);
extern int rawnet_arch_enumadapter_close(void);

#endif
