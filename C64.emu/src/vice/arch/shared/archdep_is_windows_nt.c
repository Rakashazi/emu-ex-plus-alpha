/** \file   archdep_is_windows_nt.c
 * \brief   Determine if Windows is 'NT', whatever that means
 *
 * \author  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "archdep_is_windows_nt.h"


/** \fn     archdep_is_windows_nt
 * \brief   Check if the current OS is Window NT
 *
 * No idea if this is useful anymore.
 *
 * \return  1 if true, 0 if not
 */

#ifdef WINDOWS_COMPILE
#include <windows.h>

int archdep_is_windows_nt(void)
{
    OSVERSIONINFO os_version_info;

    ZeroMemory(&os_version_info, sizeof(os_version_info));
    os_version_info.dwOSVersionInfoSize = sizeof(os_version_info);

    GetVersionEx(&os_version_info);

    if (os_version_info.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        return 1;
    }
    return 0;
}

#else

int archdep_is_windows_nt(void)
{
    return 0;
}

#endif
