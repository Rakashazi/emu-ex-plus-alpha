/*
 * vic20datasette.c - VIC20 specific CBM cassette implementation.
 *
 * Written by
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

#include "vice.h"

#include "datasette.h"
#include "via.h"
#include "vic20.h"
#include "vic20via.h"


void datasette_trigger_flux_change(unsigned int on)
{
    viacore_signal(machine_context.via1, VIA_SIG_CA1, VIA_SIG_FALL);
}

void datasette_set_tape_sense(int sense)
{
    via2_set_tape_sense(sense);
}
