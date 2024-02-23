/*
 * snespad.h
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

#ifndef VICE_SNESPAD_H
#define VICE_SNESPAD_H

#include "types.h"

enum {
    SNESPAD_BUTTON_B = 0,
    SNESPAD_BUTTON_Y,
    SNESPAD_BUTTON_SELECT,
    SNESPAD_BUTTON_START,
    SNESPAD_UP,
    SNESPAD_DOWN,
    SNESPAD_LEFT,
    SNESPAD_RIGHT,
    SNESPAD_BUTTON_A,
    SNESPAD_BUTTON_X,
    SNESPAD_BUMPER_LEFT,
    SNESPAD_BUMPER_RIGHT,
    SNESPAD_BIT_12_1,
    SNESPAD_BIT_13_1,
    SNESPAD_BIT_14_1,
    SNESPAD_BIT_15_1,

    /* This item always needs to be at the end */
    SNESPAD_EOS
};

#endif
