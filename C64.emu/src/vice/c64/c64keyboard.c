/*
 * c64keyboard.c
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

#include "c64cia.h"
#include "c64keyboard.h"
#include "interrupt.h"
#include "joystick.h"
#include "keyboard.h"
#include "machine.h"
#include "maincpu.h"
#include "vicii.h"

int c64keyboard_active = 1;

static unsigned int c64keyboard_int_num;

static void c64keyboard_machine_func(int *keyarr)
{
    cia1_check_lightpen();
}

static void c64joystick_machine_func(void)
{
    cia1_check_lightpen();
}

void c64keyboard_restore_key(int v)
{
    /* ignore key release */
    if (v) {
        /* trigger the NMI */
        maincpu_set_nmi(c64keyboard_int_num, 1);

        /* ack the restore NMI, leaving global_pending_int active */
        maincpu_set_nmi(c64keyboard_int_num, 0);
    }
}

void c64keyboard_init(void)
{
    c64keyboard_int_num = interrupt_cpu_status_int_new(maincpu_int_status, "RestoreKEY");

    switch (machine_class) {
        case VICE_MACHINE_C64:
        case VICE_MACHINE_C64SC:
        case VICE_MACHINE_SCPU64:
        case VICE_MACHINE_C128:
            keyboard_register_machine(c64keyboard_machine_func);
            joystick_register_machine(c64joystick_machine_func);
            break;
        default:
            /* No lightpen in x64dtv */
            break;
    }
}

void c64keyboard_enable(int val)
{
    c64keyboard_active = val ? 1 : 0;
}
