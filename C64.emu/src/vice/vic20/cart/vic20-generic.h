/*
 * vic20-generic.h -- VIC20 generic cartridge emulation.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#ifndef VICE_VIC20_GENERIC_H
#define VICE_VIC20_GENERIC_H

#include <stdio.h>

#include "types.h"

extern BYTE generic_ram123_read(WORD addr);
extern void generic_ram123_store(WORD addr, BYTE value);
extern BYTE generic_blk1_read(WORD addr);
extern void generic_blk1_store(WORD addr, BYTE value);
extern BYTE generic_blk2_read(WORD addr);
extern void generic_blk2_store(WORD addr, BYTE value);
extern BYTE generic_blk3_read(WORD addr);
extern void generic_blk3_store(WORD addr, BYTE value);
extern BYTE generic_blk5_read(WORD addr);
extern void generic_blk5_store(WORD addr, BYTE value);
/* extern BYTE generic_io2_read(WORD addr); */
/* extern void generic_io2_store(WORD addr, BYTE value); */
/* extern BYTE generic_io3_read(WORD addr); */
/* extern void generic_io3_store(WORD addr, BYTE value); */

extern void generic_init(void);
extern void generic_reset(void);

extern void generic_config_setup(BYTE *rawcart);
extern int generic_bin_attach(int type, const char *filename);
/* extern int generic_bin_attach(const char *filename, BYTE *rawcart); */
/* extern int generic_crt_attach(FILE *fd, BYTE *rawcart); */
extern void generic_detach(void);

extern int generic_resources_init(void);
extern void generic_resources_shutdown(void);

struct snapshot_s;

extern int generic_snapshot_write_module(struct snapshot_s *s);
extern int generic_snapshot_read_module(struct snapshot_s *s);

/* special case handling need for multiple file generic carts */
extern int generic_attach_from_resource(int type, const char *filename);
extern void generic_set_default(void);
extern const char *generic_get_file_name(WORD addr);


#endif
