/*
 * inception.h
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

#ifndef VICE_INCEPTION_H
#define VICE_INCEPTION_H

#include "types.h"

enum {
    INCEPTION_STATE_IDLE = 0,
    INCEPTION_STATE_HI_JOY1,
    INCEPTION_STATE_LO_JOY1,
    INCEPTION_STATE_HI_JOY2,
    INCEPTION_STATE_LO_JOY2,
    INCEPTION_STATE_HI_JOY3,
    INCEPTION_STATE_LO_JOY3,
    INCEPTION_STATE_HI_JOY4,
    INCEPTION_STATE_LO_JOY4,
    INCEPTION_STATE_HI_JOY5,
    INCEPTION_STATE_LO_JOY5,
    INCEPTION_STATE_HI_JOY6,
    INCEPTION_STATE_LO_JOY6,
    INCEPTION_STATE_HI_JOY7,
    INCEPTION_STATE_LO_JOY7,
    INCEPTION_STATE_HI_JOY8,
    INCEPTION_STATE_LO_JOY8,
    INCEPTION_STATE_EOS
};

int joyport_inception_resources_init(void);

#endif
