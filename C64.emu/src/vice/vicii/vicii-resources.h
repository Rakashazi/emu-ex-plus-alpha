/*
 * vicii-resources.h - Resources for the MOS 6569 (VIC-II) emulation.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#ifndef VICE_VICII_RESOURCES_H
#define VICE_VICII_RESOURCES_H

/* VIC-II resources.  */
struct vicii_resources_s {
    /* VIC-II border mode, 0..3 */
    int border_mode;

    /* Flag: Do we emulate the sprite-sprite collision register and IRQ?  */
    int sprite_sprite_collisions_enabled;

    /* Flag: Do we emulate the sprite-background collision register and
       IRQ?  */
    int sprite_background_collisions_enabled;

    /* TODO: VIC-II model */
    int model;

    /* on DTV this controls the hardware fix of the luma DAC */
    int new_luminances;
};
typedef struct vicii_resources_s vicii_resources_t;

extern vicii_resources_t vicii_resources;

extern int vicii_resources_init(void);

#endif
