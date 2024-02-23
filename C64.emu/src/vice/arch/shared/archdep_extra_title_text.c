/** \file   archdep_extra_title_text.c
 *
 * \brief   Extra text to use in the title bar
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 *
 * Provide extra text for the window title bar indicating which key to press
 * to access the menu.
 *
 * \note    Only used in the SDL port.
 */

/*
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

#include "kbd.h"
#include "lib.h"
#include "util.h"

#include "archdep_extra_title_text.h"


#if defined(USE_SDLUI) || defined(USE_SDL2UI)
/** \brief  Extra text for the window title bar
 *
 * Heap-allocated by archdep_extra_title_text(), needs to be freed on emu exit
 * with archdep_extra_title_text_free() (SDL only)
 */
static char *extra_title_text = NULL;
#endif


/** \brief  Get extra text for title bar
 *
 * \return  'press "\$MENU_KEY" for the menu.'
 *
 * \note    Call archdep_extra_title_text_free() on emulator shutdown to free
 *          memory allocated for the string.
 */
const char *archdep_extra_title_text(void)
{
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
    if (extra_title_text == NULL) {
        char *menu_keyname = kbd_get_menu_keyname();

        extra_title_text = util_concat(", press \"",
                                       menu_keyname,
                                       "\" for the menu.",
                                       NULL);
        lib_free(menu_keyname);
    }
    return extra_title_text;
#else
    return NULL;
#endif
}


/** \brief  Free memory used by the extra title text
 *
 * \note    Call on emulator shutdown.
 */
void archdep_extra_title_text_free(void)
{
#if defined(USE_SDLUI) || defined(USE_SDL2UI)
    if (extra_title_text != NULL) {
        lib_free(extra_title_text);
        extra_title_text = NULL;
    }
#endif
}

