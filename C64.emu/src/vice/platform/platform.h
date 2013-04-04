/*
 * platform.h - port/platform specific discovery.
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

#ifndef VICE_PLATFORM_H
#define VICE_PLATFORM_H

#include "vice.h"

extern char *platform_get_compile_time_os(void);
extern char *platform_get_compile_time_compiler(void);
extern char *platform_get_compile_time_cpu(void);
extern char *platform_get_ui(void);
extern char *platform_get_runtime_os(void);
extern char *platform_get_runtime_cpu(void);

extern char* platform_get_x86_runtime_cpu(void);
extern char *platform_get_windows_runtime_os(void);

extern char *platform_get_macosx_runtime_os(void);
extern char *platform_get_macosx_runtime_cpu(void);

extern char *platform_get_amigaos3_runtime_os(void);
extern char *platform_get_amigaos3_runtime_cpu(void);

extern char *platform_get_amigaos4_runtime_os(void);
extern char *platform_get_amigaos4_runtime_cpu(void);

extern char *platform_get_aros_runtime_os(void);
extern char *platform_get_aros_runtime_cpu(void);

extern char *platform_get_os2_runtime_os(void);

extern int CheckForHaiku(void);
extern int CheckForZeta(void);

extern char *platform_get_haiku_runtime_os(void);
extern char *platform_get_zeta_runtime_os(void);
extern char *platform_get_beos_runtime_os(void);
extern char *platform_get_beosppc_runtime_cpu(void);

extern char *platform_get_solaris_runtime_os(void);

extern char *platform_get_syllable_runtime_os(void);
extern char *platform_get_syllable_runtime_cpu(void);

#endif
