/*
 * vicii-sprites.h - Sprites for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_VICII_SPRITES_H
#define VICE_VICII_SPRITES_H

/* This defines the stolen sprite cycles for all the values of `dma_msk'.  */
/* The table derives from what Christian Bauer <bauec002@physik.uni-mainz.de>
   says in both the "VIC-Article" and Frodo's `VIC_SC.cpp' source file.  */

struct vicii_sprites_fetch_s {
    int cycle, num;
    unsigned int first, last;
};
typedef struct vicii_sprites_fetch_s vicii_sprites_fetch_t;

extern const vicii_sprites_fetch_t vicii_sprites_fetch_table[256][4];
extern const int vicii_sprites_crunch_table[64];

extern void vicii_sprites_init(void);
extern void vicii_sprites_shutdown(void);
extern void vicii_sprites_set_x_position(unsigned int num, int new_x, int raster_x);
extern void vicii_sprites_reset_sprline(void);
extern void vicii_sprites_init_sprline(void);
extern void vicii_sprites_reset_xshift(void);
extern int vicii_sprite_offset(void);

#endif
