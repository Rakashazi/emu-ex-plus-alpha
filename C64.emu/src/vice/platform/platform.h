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

#include "platform_x86_runtime_cpu.h"

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

extern char *platform_get_sunos_runtime_os(void);
extern char *platform_get_sunos_runtime_cpu(void);

extern char *platform_get_amix_runtime_os(void);
extern char *platform_get_amix_runtime_cpu(void);

extern char *platform_get_solaris_runtime_os(void);
extern char *platform_get_solaris_runtime_cpu(void);

extern char *platform_get_darwin_runtime_os(void);
extern char *platform_get_darwin_runtime_cpu(void);

extern char *platform_get_nextopenstep_runtime_os(void);
extern char *platform_get_nextopenstep_runtime_cpu(void);

extern char *platform_get_rhapsody_runtime_os(void);
extern char *platform_get_rhapsody_runtime_cpu(void);

extern char *platform_get_syllable_runtime_os(void);
extern char *platform_get_syllable_runtime_cpu(void);

extern char *platform_get_linux_runtime_os(void);
extern char *platform_get_linux_runtime_cpu(void);

extern char *platform_get_netbsd_runtime_os(void);
extern char *platform_get_netbsd_runtime_cpu(void);

extern char *platform_get_freebsd_runtime_os(void);
extern char *platform_get_freebsd_runtime_cpu(void);

extern char *platform_get_dragonfly_runtime_os(void);
extern char *platform_get_dragonfly_runtime_cpu(void);

extern char *platform_get_openbsd_runtime_os(void);
extern char *platform_get_openbsd_runtime_cpu(void);

extern char *platform_get_ultrix_runtime_os(void);
extern char *platform_get_ultrix_runtime_cpu(void);

extern char *platform_get_interix_runtime_os(void);

extern char *platform_get_cygwin_runtime_os(void);
extern char *platform_get_cygwin_runtime_cpu(void);

extern char *platform_get_dos_runtime_os(void);

extern char *platform_get_sco_runtime_os(void);
extern char *platform_get_sco_runtime_cpu(void);

extern char *platform_get_skyos_runtime_os(void);
extern char *platform_get_skyos_runtime_cpu(void);

extern char *platform_get_minix_runtime_os(void);
extern char *platform_get_minix_runtime_cpu(void);

extern char *platform_get_hurd_runtime_os(void);

/* Set the runtime os call for known platforms */

/* Windows on cygwin */
#if defined(__CYGWIN32__) || defined(__CYGWIN__)
#define RUNTIME_OS_CALL platform_get_cygwin_runtime_os
#endif

/* MacOSX */
#if defined(MACOSX_COCOA)
#define RUNTIME_OS_CALL platform_get_macosx_runtime_os
#endif

/* AMIX */
#ifdef __AMIX__
#define RUNTIME_OS_CALL platform_get_amix_runtime_os
#endif

/* SunOS */
#if (defined(sun) || defined(__sun)) && !(defined(__SVR4) || defined(__svr4__))
#define RUNTIME_OS_CALL platform_get_sunos_runtime_os
#endif

/* Solaris */
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#define RUNTIME_OS_CALL platform_get_solaris_runtime_os
#endif

/* Minix */
#ifdef __minix
#define RUNTIME_OS_CALL platform_get_minix_runtime_os
#endif

/* GNU Hurd */
#if defined(__GNU__) && !defined(NEXTSTEP_COMPILE) && !defined(OPENSTEP_COMPILE)
#define RUNTIME_OS_CALL platform_get_hurd_runtime_os
#endif

/* Syllable */
#ifdef __SYLLABLE__
#define RUNTIME_OS_CALL platform_get_syllable_runtime_os
#endif

/* Linux */
#if defined(__linux) && !defined(__ANDROID__) && !defined(AMIGA_AROS)
#define RUNTIME_OS_CALL platform_get_linux_runtime_os
#endif

/* NetBSD */
#ifdef __NetBSD__
#define RUNTIME_OS_CALL platform_get_netbsd_runtime_os
#endif

/* FreeBSD */
#if defined(__FreeBSD__) && !defined(__DragonFly__)
#define RUNTIME_OS_CALL platform_get_freebsd_runtime_os
#endif

/* DragonFly BSD */
#ifdef __DragonFly__
#define RUNTIME_OS_CALL platform_get_dragonfly_runtime_os
#endif

/* OpenBSD */
#ifdef __OpenBSD__
#define RUNTIME_OS_CALL platform_get_openbsd_runtime_os
#endif

/* Ultrix */
#if defined(ultrix) || defined(__ultrix) || defined(__ultrix__)
#define RUNTIME_OS_CALL platform_get_ultrix_runtime_os
#endif

/* Interix */
#ifdef __INTERIX
#define RUNTIME_OS_CALL platform_get_interix_runtime_os
#endif

/* SkyOS */
#ifdef __SKYOS__
#define RUNTIME_OS_CALL platform_get_skyos_runtime_os
#endif

/* SCO Unix/Openserver/Unixware */
#if defined(SCO4UNIX_COMPILE) || defined(OPENSERVER5_COMPILE) || defined(OPENSERVER6_COMPILE) || defined(UNIXWARE_COMPILE)
#define RUNTIME_OS_CALL platform_get_sco_runtime_os
#endif

/* Darwin */
#ifdef DARWIN_COMPILE
#define RUNTIME_OS_CALL platform_get_darwin_runtime_os
#endif

/* NextStep/OpenStep */
#if defined(NEXTSTEP_COMPILE) || defined(OPENSTEP_COMPILE)
#define RUNTIME_OS_CALL platform_get_nextopenstep_runtime_os
#endif

/* Rhapsody */
#ifdef RHAPSODY_COMPILE
#define RUNTIME_OS_CALL platform_get_rhapsody_runtime_os
#endif


/* Set the runtime cpu call for known platforms */

/* Windows on cygwin */
#if defined(__CYGWIN32__) || defined(__CYGWIN__)
#define RUNTIME_CPU_CALL platform_get_cygwin_runtime_cpu
#endif

/* SCO Unix 4.x */
#ifdef SCO4UNIX_COMPILE
#define RUNTIME_CPU_CALL platform_get_sco_runtime_cpu
#endif

/* Openserver 5.x */
#ifdef OPENSERVER5_COMPILE
#define RUNTIME_CPU_CALL platform_get_sco_runtime_cpu
#endif

/* Openserver 6.x */
#ifdef OPENSERVER6_COMPILE
#define RUNTIME_CPU_CALL platform_get_sco_runtime_cpu
#endif

/* Unixware 7.x */
#ifdef UNIXWARE_COMPILE
#define RUNTIME_CPU_CALL platform_get_sco_runtime_cpu
#endif

/* AMIX */
#ifdef __AMIX__
#define RUNTIME_CPU_CALL platform_get_amix_runtime_cpu
#endif

/* SunOS */
#if (defined(sun) || defined(__sun)) && !(defined(__SVR4) || defined(__svr4__))
#define RUNTIME_CPU_CALL platform_get_sunos_runtime_cpu
#endif

/* Solaris */
#if (defined(sun) || defined(__sun)) && (defined(__SVR4) || defined(__svr4__))
#  if defined(__sparc64__) || defined(sparc64) || defined(__sparc__) || defined(sparc)
#    define RUNTIME_CPU_CALL platform_get_solaris_runtime_cpu
#  endif
#endif

/* Minix with ack */
#if defined(__minix) && defined(__ACK__)
#define RUNTIME_CPU_CALL platform_get_minix_runtime_cpu
#endif

/* Darwin */
#ifdef DARWIN_COMPILE
#define RUNTIME_CPU_CALL platform_get_darwin_runtime_cpu
#endif

/* NextStep/OpenStep */
#if defined(NEXTSTEP_COMPILE) || defined(OPENSTEP_COMPILE)
#define RUNTIME_CPU_CALL platform_get_nextopenstep_runtime_cpu
#endif

/* Rhapsody */
#ifdef RHAPSODY_COMPILE
#define RUNTIME_CPU_CALL platform_get_rhapsody_runtime_cpu
#endif

/* MacOSX */
#if defined(MACOSX_COCOA)
#define RUNTIME_CPU_CALL platform_get_macosx_runtime_cpu
#endif

/* SkyOS */
#ifdef __SKYOS__
#define RUNTIME_CPU_CALL platform_get_skyos_runtime_cpu
#endif

/* Syllable */
#ifdef __SYLLABLE__
#define RUNTIME_CPU_CALL platform_get_syllable_runtime_cpu
#endif

/* Linux */
#if defined(__linux) && !defined(__ANDROID__)
#define RUNTIME_CPU_CALL platform_get_linux_runtime_cpu
#endif

/* NetBSD */
#ifdef __NetBSD__
#define RUNTIME_CPU_CALL platform_get_netbsd_runtime_cpu
#endif

/* FreeBSD */
#if defined(__FreeBSD__) && !defined(__DragonFly__)
#define RUNTIME_CPU_CALL platform_get_freebsd_runtime_cpu
#endif

/* FreeBSD */
#ifdef __DragonFly__
#define RUNTIME_CPU_CALL platform_get_dragonfly_runtime_cpu
#endif

/* OpenBSD */
#ifdef __OpenBSD__
#define RUNTIME_CPU_CALL platform_get_openbsd_runtime_cpu
#endif

/* Ultrix */
#if defined(ultrix) || defined(__ultrix) || defined(__ultrix__)
#define RUNTIME_CPU_CALL platform_get_ultrix_runtime_cpu
#endif

/* x86/amd64/x86_64 */
#if !defined(PLATFORM_NO_X86_ASM) && !defined(RUNTIME_CPU_CALL)
#define RUNTIME_CPU_CALL platform_get_x86_runtime_cpu
#endif

#endif
