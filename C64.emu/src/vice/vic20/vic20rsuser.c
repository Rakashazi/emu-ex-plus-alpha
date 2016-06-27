/*
 * vic20rsuser.c - VIC20 RS232 userport interface
 *
 * Written by
 *  Andre Fachat <a.fachat@physik.tu-chemnitz.de>
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
/*
 * This is a very crude emulation. It does not check for a lot of things.
 * It simply tries to work with existing programs that work on the real
 * machine and does not try to catch rogue attempts...
 *
 * It calls the stuff provided by the rsuser.c in the main source
 * directory.
 */

#include "vice.h"

#include <stdio.h>

#include "machine.h"
#include "rsuser.h"
#include "types.h"
#include "via.h"
#include "vic20.h"
#include "vic20rsuser.h"


static void vic20_trigger_start(void)
{
    viacore_signal(machine_context.via2, VIA_SIG_CB1, VIA_SIG_FALL);
    viacore_signal(machine_context.via2, VIA_SIG_CB1, VIA_SIG_RISE);
}

void vic20_rsuser_init(void)
{
    /* The 1.0 is the CPU clk ratio to 1 MHz */
    rsuser_init(machine_get_cycles_per_second(), vic20_trigger_start, NULL);
}
