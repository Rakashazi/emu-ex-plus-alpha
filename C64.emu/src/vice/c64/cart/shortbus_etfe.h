/*
 * shortbus_etfe.h - ETFE ("The final ethernet") emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * Based on code by
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

#ifdef HAVE_PCAP
#else
  #error TFE.H should not be included if HAVE_PCAP is not defined!
#endif /* #ifdef HAVE_PCAP */

#ifndef VICE_SHORTBUS_ETFE_H
#define VICE_SHORTBUS_ETFE_H

#include "snapshot.h"

extern int shortbus_etfe_resources_init(void);
extern void shortbus_etfe_resources_shutdown(void);

extern int shortbus_etfe_cmdline_options_init(void);

extern void shortbus_etfe_unregister(void);
extern void shortbus_etfe_register(void);

extern void shortbus_etfe_reset(void);

extern int shortbus_etfe_enabled(void);

extern int shortbus_etfe_write_snapshot_module(snapshot_t *s);
extern int shortbus_etfe_read_snapshot_module(snapshot_t *s);

#endif
