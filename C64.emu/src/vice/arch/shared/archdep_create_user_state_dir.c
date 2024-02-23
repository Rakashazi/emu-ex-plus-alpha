/** \file   archdep_create_user_state_dir.c
 * \brief   Create XDG user state dir
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
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
#include <errno.h>
#include <string.h>

#include "archdep_defs.h"
#include "archdep_exit.h"
#include "archdep_mkdir.h"
#include "archdep_user_state_path.h"
#include "log.h"

#include "archdep_create_user_state_dir.h"


/** \brief  Create XDG user config dir
 *
 * Create the XDG Base Directory Specification <tt>~/.local/state/vice</tt> directory.
 */
void archdep_create_user_state_dir(void)
{
    const char *state = archdep_user_state_path();

    if (archdep_mkdir_recursive(state, 0755) == 0) {
        return;     /* we created the dir */
    } else if (errno != EEXIST) {
        log_error(LOG_ERR, "failed to create user state dir '%s': %d: %s.",
                  state, errno, strerror(errno));
        archdep_vice_exit(1);
    }
}
