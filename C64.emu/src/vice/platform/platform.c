/*
 * platform.c - port/platform specific compile time discovery.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

/* Operating systems fully supported and tested:
 *
 * platform         | compile present | compile version | runtime version
 * ----------------------------------------------------------------------
 * amigaos3         |       yes       |       n/a       |       yes
 * aros             |       yes       |       n/a       |       yes
 * beos             |       yes       |       n/a       |       yes
 * cygwin           |       yes       |       yes       |       yes
 * darwin           |       yes       |       yes       |       yes
 * interix          |       yes       |       yes       |       yes
 * linux            |       yes       |       yes       |       yes
 * macosx           |       yes       |       yes       |       yes
 * minix 3.x        |       yes       |       yes       |       yes
 * msdos            |       yes       |       n/a       |       yes
 * openserver       |       yes       |       yes       |       yes
 * os/2             |       yes       |       n/a       |       yes
 * solaris          |       yes       |       n/a       |       yes
 * syllable         |       yes       |       n/a       |       yes
 * unixware         |       yes       |       yes       |       yes
 * win32            |       yes       |       yes       |       yes
 * win64            |       yes       |       yes       |       yes
 */

/* Operating systems fully supported and partially tested:
 *
 * platform         | compile present | compile version | runtime version
 * ----------------------------------------------------------------------
 * amigaos4         |       yes       |       n/a       | yes (untested)
 * morphos          |       yes       |       n/a       |     not yet
 */

/* Operating systems compile time supported but untested:
 *
 * platform         | compile present | compile version | runtime version
 * ----------------------------------------------------------------------
 * aix              | yes (untested)  | yes (untested)  |     not yet
 * android          | yes (untested)  |     not yet     |     not yet
 * bsdi             | yes (untested)  |     not yet     |     not yet
 * dragonflybsd     | yes (untested)  |     not yet     |     not yet
 * dynix/ptx        | yes (untested)  |     not yet     |     not yet
 * freebsd          | yes (untested)  | yes (untested)  |     not yet
 * hpux             | yes (untested)  |     not yet     |     not yet
 * hurd             | yes (untested)  |     not yet     |     not yet
 * irix             | yes (untested)  |     not yet     |     not yet
 * lynxos           | yes (untested)  |     not yet     |     not yet
 * netbsd           | yes (untested)  | yes (untested)  |     not yet
 * openbsd          | yes (untested)  | yes (untested)  |     not yet
 * openvms          | yes (untested)  |     not yet     |     not yet
 * qnx              | yes (untested)  | yes (untested)  |     not yet
 * sinux            | yes (untested)  |     not yet     |     not yet
 * sunos            | yes (untested)  |     not yet     |     not yet
 * symbian os       | yes (untested)  |     not yet     |     not yet
 * tru64            | yes (untested)  |     not yet     |     not yet
 * ultrix           | yes (untested)  |     not yet     |     not yet
 * uwin             | yes (untested)  |     not yet     |     not yet
 * vxworks          | yes (untested)  |     not yet     |     not yet
 * xbox             | yes (untested)  |       n/a       |     not yet
 * xbox-360         | yes (untested)  |       n/a       |     not yet
 */

/* Operating systems not yet supported:
 *
 * platform         | compile present | compile version | runtime version
 * ----------------------------------------------------------------------
 * 386bsd           |     not yet     |     not yet     |     not yet
 * 4.3bsd           |     not yet     |     not yet     |     not yet
 * amix             |     not yet     |     not yet     |     not yet
 * amoeba           |     not yet     |     not yet     |     not yet
 * at&t unix        |     not yet     |     not yet     |     not yet
 * atari-mint       |     not yet     |     not yet     |     not yet
 * blackberry       |     not yet     |     not yet     |     not yet
 * caanoo           |     not yet     |     not yet     |     not yet
 * convex-os        |     not yet     |     not yet     |     not yet
 * dell unix        |     not yet     |     not yet     |     not yet
 * desqview-x       |     not yet     |     not yet     |     not yet
 * dgux             |     not yet     |     not yet     |     not yet
 * dingoo           |     not yet     |     not yet     |     not yet
 * dreamcast        |     not yet     |     not yet     |     not yet
 * ews-ux           |     not yet     |     not yet     |     not yet
 * gamecube         |     not yet     |     not yet     |     not yet
 * gp2x             |     not yet     |     not yet     |     not yet
 * interactive unix |     not yet     |     not yet     |     not yet
 * ios              |     not yet     |     not yet     |     not yet
 * mach 2.5/3.0     |     not yet     |     not yet     |     not yet
 * macos (classic)  |     not yet     |     not yet     |     not yet
 * meego            |     not yet     |     not yet     |     not yet
 * menuetos         |     not yet     |     not yet     |     not yet
 * minix386         |     not yet     |     not yet     |     not yet
 * ncr unix         |     not yet     |     not yet     |     not yet
 * nds              |     not yet     |     not yet     |     not yet
 * netware          |     not yet     |     not yet     |     not yet
 * news-os          |     not yet     |     not yet     |     not yet
 * nextstep         |     not yet     |     not yet     |     not yet
 * odt              |     not yet     |     not yet     |     not yet
 * openstep         |     not yet     |     not yet     |     not yet
 * osf1             |     not yet     |     not yet     |     not yet
 * palmos           |     not yet     |     not yet     |     not yet
 * pandora          |     not yet     |     not yet     |     not yet
 * plan9            |     not yet     |     not yet     |     not yet
 * ps2              |     not yet     |     not yet     |     not yet
 * ps3              |     not yet     |     not yet     |     not yet
 * psp              |     not yet     |     not yet     |     not yet
 * rhapsody         |     not yet     |     not yet     |     not yet
 * riscos           |     not yet     |     not yet     |     not yet
 * sailfish         |     not yet     |     not yet     |     not yet
 * sco unix         |     not yet     |     not yet     |     not yet
 * tizen            |     not yet     |     not yet     |     not yet
 * unicos           |     not yet     |     not yet     |     not yet
 * unix svr3        |     not yet     |     not yet     |     not yet
 * unix svr4        |     not yet     |     not yet     |     not yet
 * utek             |     not yet     |     not yet     |     not yet
 * web-os           |     not yet     |     not yet     |     not yet
 * wii              |     not yet     |     not yet     |     not yet
 * wince            |     not yet     |     not yet     |     not yet
 * winphone         |     not yet     |     not yet     |     not yet
 * wiz              |     not yet     |     not yet     |     not yet
 * xenix            |     not yet     |     not yet     |     not yet
 * zodiac           |     not yet     |     not yet     |     not yet
 */

#include "vice.h"

#include "archapi.h"

#include "platform_discovery.h"

char *platform_get_compile_time_os(void)
{
    return PLATFORM_OS;
}

#ifdef PLATFORM_COMPILER_MAJOR_MASK
static char platform_compiler_version[256];
#endif

char *platform_get_compile_time_compiler(void)
{
#ifndef PLATFORM_COMPILER_MAJOR_MASK
    return PLATFORM_COMPILER;
#else
    int major_mask = PLATFORM_COMPILER_MAJOR_MASK;
    int minor_mask = PLATFORM_COMPILER_MINOR_MASK;
#ifdef PLATFORM_COMPILER_PATCHLEVEL_MASK
    int patchlevel_mask = PLATFORM_COMPILER_PATCHLEVEL_MASK;
#endif
    int version = PLATFORM_COMPILER_VERSION;

    int major = (int)(version / major_mask);
    int minor = (int)((version - (major * major_mask)) / minor_mask);
#ifdef PLATFORM_COMPILER_PATCHLEVEL_MASK
    int patchlevel = (int)((version - ((major * major_mask) + (minor * minor_mask)) / patchlevel_mask);

    sprintf(platform_compiler_version, "%s %d.%d.%d", PLATFORM_COMPILER_NAME, major, minor, patchlevel);
#else
    sprintf(platform_compiler_version, "%s %d.%d", PLATFORM_COMPILER_NAME, major, minor);
#endif
    return platform_compiler_version;
#endif
}

char *platform_get_compile_time_cpu(void)
{
    return PLATFORM_CPU;
}

char *platform_get_ui(void)
{
#ifdef USE_SDLUI
    return "SDL";
#elif defined(USE_SDLUI2)
    return "SDL2";
#elif defined(USE_GNOMEUI)
    return "GTK+";
#elif defined(MACOSX_COCOA)
    return "COCOA";
#elif defined(UNIX_COMPILE)
    return "XAW";
#else
    return "NATIVE";
#endif
}

char *platform_get_runtime_os(void)
{
    return archdep_get_runtime_os();
}

char *platform_get_runtime_cpu(void)
{
    return archdep_get_runtime_cpu();
}
