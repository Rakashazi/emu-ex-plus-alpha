/** \file   archdep_default_rtc_file_name.c
 * \brief   Determine path to default realtime clock file
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
#include "archdep_defs.h"

#include "archdep_user_config_path.h"
#include "util.h"

#include "archdep_default_rtc_file_name.h"


/** \brief  Generate path to the default RTC file
 *
 * \return  path to default RTC file, must be freed with lib_free()
 */
char *archdep_default_rtc_file_name(void)
{
    return util_join_paths(archdep_user_config_path(),
                           ARCHDEP_VICE_RTC_NAME,
                           NULL);
}
