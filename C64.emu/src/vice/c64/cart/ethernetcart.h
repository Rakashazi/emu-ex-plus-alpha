/*
 * ethernetcart.h - Generic CS8900 based ethernet cartridge emulation.
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

#ifdef HAVE_PCAP
#else
  #error ETHERNETCART.H should not be included if HAVE_PCAP is not defined!
#endif /* #ifdef HAVE_PCAP */

#ifndef CARTRIDGE_INCLUDE_PRIVATE_API
#ifndef CARTRIDGE_INCLUDE_PUBLIC_API
#error "do not include this header directly, use c64cart.h instead."
#endif
#endif

#ifndef VICE_ETHERNETCART_H
#define VICE_ETHERNETCART_H

#define ETHERNETCART_MODE_TFE     0
#define ETHERNETCART_MODE_RRNET   1

struct snapshot_s;
extern int ethernetcart_snapshot_read_module(struct snapshot_s *s);
extern int ethernetcart_snapshot_write_module(struct snapshot_s *s);

extern int ethernetcart_cart_enabled(void);

extern void ethernetcart_init(void);
extern int ethernetcart_resources_init(void);
extern void ethernetcart_resources_shutdown(void);
extern int ethernetcart_cmdline_options_init(void);

extern void ethernetcart_reset(void);
extern void ethernetcart_detach(void);
extern int ethernetcart_enable(void);

#endif
