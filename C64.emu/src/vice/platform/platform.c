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
