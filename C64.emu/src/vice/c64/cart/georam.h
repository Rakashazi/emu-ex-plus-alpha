/*
 * georam.h - GEORAM emulation.
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

#ifndef VICE_GEORAM_H
#define VICE_GEORAM_H

#include "types.h"

struct snapshot_s;

extern void georam_init(void);
extern int georam_resources_init(void);
extern void georam_resources_shutdown(void);
extern int georam_cmdline_options_init(void);

extern void georam_reset(void);
extern void georam_detach(void);
extern int georam_enable(void);

extern int georam_read_snapshot_module(struct snapshot_s *s);
extern int georam_write_snapshot_module(struct snapshot_s *s);

extern int georam_cart_enabled(void);
extern void georam_config_setup(BYTE *rawcart);

extern const char *georam_get_file_name(void);
extern int georam_bin_attach(const char *filename, BYTE *rawcart);
extern int georam_bin_save(const char *filename);
extern int georam_flush_image(void);

#endif
