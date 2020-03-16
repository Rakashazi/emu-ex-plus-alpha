/*
 * masceruade-stubs.h - C64 expansion port stubs handling for the VIC20 masC=uerade adapter.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *  Bas Wassink <b.wassink@ziggo.nl>
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

#ifndef VICE_MASCUERADE_STUBS_H
#define VICE_MASCUERADE_STUBS_H

int mmc64_cart_enabled(void);
int mmcreplay_cart_enabled(void);
int retroreplay_cart_enabled(void);
int rrnetmk3_cart_enabled(void);
int cartridge_type_enabled(int type);
int cartridge_flush_image(int type);
int cartridge_save_image(int type, const char *filename);

#endif

