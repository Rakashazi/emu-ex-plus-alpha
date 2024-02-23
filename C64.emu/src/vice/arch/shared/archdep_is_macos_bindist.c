/** \file   archdep_is_macos_bindist.c
 * \brief   Determine if running from a macOS binary distribution
 *
 * \author  David Hogan <david.q.hogan@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include "archdep_boot_path.h"
#include "archdep_defs.h"

#include "archdep_is_macos_bindist.h"


/** \brief  Determine if we're running a MacOS bindist
 *
 * \return  0 if bindist, -1 otherwise
 */
int archdep_is_macos_bindist(void) {
#ifdef MACOS_COMPILE
    static char *BINDIST_BOOT_PATH = "/VICE.app/Contents/Resources/bin";

    char *bindist_boot_path_ptr = strstr(archdep_boot_path(), BINDIST_BOOT_PATH);

    if (bindist_boot_path_ptr && strlen(bindist_boot_path_ptr) == strlen(BINDIST_BOOT_PATH)) {
        return 1;
    }
#endif
    return 0;
}
