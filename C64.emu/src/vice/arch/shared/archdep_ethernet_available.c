/** \file   archdep_ethernet_available.c
 * \brief   Determine if ethernet support is available for the current process
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
#include "config.h"
#include "archdep_defs.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef UNIX_COMPILE
# include <unistd.h>
# include <sys/types.h>
#elif defined(WINDOWS_COMPILE)
# include <windows.h>
#endif

#include "archdep_ethernet_available.h"
#include "archdep_rawnet_capability.h"


/** \brief  Determine if ethernet support is available for the current process
 *
 * On Unix, ethernet support is available via TUN/TAP virtual network devices,
 * if TUN/TAP is not available, then we can use libpcap if the user/process has
 * the permission to use rawnet.On Windows it checks for the DLL being loaded.
 * MacOS is currently heaped together with UNIX; a TUN/TAP driver is available,
 * but I don't have a clue how pcap works on MacOS, nor if it is even available.
 *
 * \return  bool
 */
bool archdep_ethernet_available(void)
{
#ifdef UNIX_COMPILE
# ifdef HAVE_TUNTAP
    /* When TUN/TAP is available, ethernet support is available for all users */
    return true;
# elif defined HAVE_PCAP
    /* When PCAP is available, check if we have the permission to use rawnet */
    return archdep_rawnet_capability();
# else
    return false;
# endif
#elif defined WINDOWS_COMPILE
    /* check if the wpcap .dll is loaded */
    return GetModuleHandleA("WPCAP.DLL") != NULL;
#else
    return false;
#endif
}

