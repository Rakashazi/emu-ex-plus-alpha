/*
 * petcat-stubs.c - dummies for unneeded/unused functions
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include <vice.h>
#include "archdep_defs.h"

#include "cmdline.h"
#include "vice-event.h"
#include "machine.h"
#include "version.h"

#include <stdbool.h>
#include <stdlib.h>

/*
   FIXME: these really shouldnt be needed here and are a sign of bad modular
          design elsewhere
 */

/* called from resources_set_defaults() */
void cartridge_detach_image(int type)
{
}

/* called from resources_set_defaults() */
void cartridge_unset_default(void)
{
}

/*
 * Stubs needed due to archdep_exit functionality that
 * needs to call back into vice
 */

void main_exit(void)
{
}

bool mainlock_is_vice_thread(void)
{
    return false;
}

void mainlock_initiate_shutdown(void)
{
}

const char machine_name[] = "PETCAT";
int machine_class = VICE_MACHINE_PETCAT;

const char *machine_get_name(void)
{
    return machine_name;
}

int cmdline_register_options(const cmdline_option_t *c)
{
    return 0;
}

int network_connected(void)
{
    return 0;
}

int network_get_mode(void)
{
#if 0
    return NETWORK_IDLE;
#else
    return 0;
#endif
}

void network_event_record(unsigned int type, void *data, unsigned int size)
{
}

void event_record_in_list(event_list_state_t *list, unsigned int type, void *data, unsigned int size)
{
}

void ui_error_string(const char *text) /* win32 needs this */
{
}

void ui_error(const char *format, ...) /* SDL on mingw32 needs this */
{
}

char *kbd_get_menu_keyname(void)
{
    return NULL;
}
