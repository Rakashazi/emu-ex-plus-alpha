/** \file   archdep_rawnet_capability.c
 * \brief   Determine if ethernet support (libpcap) will actually work
 *
 * \author  groepaz <groepaz@gmx.net>
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

/* #define DEBUG_ARCHDEP_RAWNET_CAPABILITY */

#include "vice.h"
#include "archdep_defs.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef UNIX_COMPILE
# include <unistd.h>
# include <sys/types.h>
# ifdef HAVE_CAPABILITIES
#  include <sys/capability.h>
# endif
#elif defined(WINDOWS_COMPILE)
# include <windows.h>
#endif

#include "archdep_rawnet_capability.h"

#ifdef DEBUG_ARCHDEP_RAWNET_CAPABILITY
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#ifdef UNIX_COMPILE
# ifdef HAVE_PCAP
#  ifdef HAVE_CAPABILITIES
static bool cap_check(cap_value_t cap_flag)
{
    cap_t cap;
    cap = cap_get_proc();

    if (cap == NULL) {
        return false;
    }

    if (cap_set_flag(cap, CAP_EFFECTIVE, 1, &cap_flag, CAP_SET) == -1) {
        DBG(("CAP_NET_RAW not enabled\n"));
        cap_free(cap);
        return false;
    }
    if (cap_set_proc(cap) != 0) {
        DBG(("cannot set CAP_NET_RAW\n"));
        cap_free(cap);
        return false;
    }
    return true;
}
#  endif
# endif
#endif

/** \brief  Determine if the current process has the permissions to use rawnet
 *
 * \return  bool
 */
bool archdep_rawnet_capability(void)
{
#ifdef UNIX_COMPILE
# ifdef HAVE_PCAP
    /* if we are root then rawnet works, regardless of capabilities */
    if (geteuid() == 0) {
        return true;
    }
#  ifdef HAVE_CAPABILITIES
    /* check if the CAP_NET_RAW capability is supported */
    if (CAP_IS_SUPPORTED(CAP_NET_RAW)) {
        /* check if CAP_NET_RAW is effective */
        if (cap_check(CAP_NET_RAW)) {
            return true;
        }
    }
#  endif
# endif
#elif defined WINDOWS_COMPILE
    /* on windows always return true for the time being, perhaps we should also
       check permissions somehow? */
    return true;
#endif
    return false;
}

