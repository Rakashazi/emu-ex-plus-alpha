
/*
 * features.c - List of compile time selectable features
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include <stdlib.h>

#include "debug.h"
#include "vicefeatures.h"


static const feature_list_t featurelist[] = {
/* all */
    { "DEBUG", "Enable debugging code",
#ifndef DEBUG
        0 },
#else
        1 },
#endif
/* all */
    { "HAVE_DEBUG_THREADS", "Enable thread debugging code",
#ifdef HAVE_DEBUG_THREADS
        1 },
#else
        0 },
#endif
/* (all) */
    { "FEATURE_CPUMEMHISTORY", "Enable the memmap/chis feature in the monitor.",
#ifndef FEATURE_CPUMEMHISTORY
        0 },
#else
        1 },
#endif
#ifdef MACOS_COMPILE /* (osx) */
    { "HAS_HIDMGR", "Enable Mac IOHIDManager Joystick driver.",
#ifndef HAS_HIDMGR
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX_COMPILE /* (unix) */
    { "HAS_USB_JOYSTICK", "Enable emulation for USB joysticks. (deprecated)",
#ifndef HAS_USB_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#if defined(MACOS_COMPILE) /* (osx) */
    { "HAVE_AUDIO_UNIT", "Enable AudioUnit support.",
#ifndef HAVE_AUDIO_UNIT
        0 },
#else
        1 },
#endif
#endif
#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE) /* (beos/unix/windows) */
    { "HAVE_CATWEASELMKIII", "Support for Catweasel MKIII.",
#ifndef HAVE_CATWEASELMKIII
        0 },
#else
        1 },
#endif
#endif

#ifdef WINDOWS_COMPILE /* (windows) */
    { "HAVE_DINPUT", "Use the DirectInput joystick driver",
#ifndef HAVE_DINPUT
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX_COMPILE) || defined(MACOS_COMPILE) || defined(WINDOWS_COMPILE) /* (unix/osx/windows) */
    { "HAVE_DYNLIB_SUPPORT", "Support dynamic library loading.",
#ifndef HAVE_DYNLIB_SUPPORT
        0 },
#else
        1 },
#endif
#endif
 /* (all) */
    { "HAVE_EXPERIMENTAL_DEVICES", "Enable experimental devices",
#ifndef HAVE_EXPERIMENTAL_DEVICES
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_FFMPEG", "Have FFMPEG av* libs available (deprecated)",
#ifndef HAVE_FFMPEG
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_FFMPEG_HEADER_SUBDIRS", "FFMPEG uses subdirs for headers (deprecated)",
#ifndef HAVE_FFMPEG_HEADER_SUBDIRS
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_FFMPEG_SWSCALE", "Have FFMPEG swscale lib available (deprecated)",
#ifndef HAVE_FFMPEG_SWSCALE
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_FFMPEG_SWRESAMPLE", "Have FFMPEG swresample lib available (deprecated)",
#ifndef HAVE_FFMPEG_SWRESAMPLE
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_FFMPEG_AVRESAMPLE", "Have FFMPEG avresample lib available (deprecated)",
#ifndef HAVE_FFMPEG_AVRESAMPLE
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_GIF", "Use the GIF or UNGIF library",
#ifndef HAVE_GIF
        0 },
#else
        1 },
#endif
#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE) /* (beos/unix/windows) */
    { "HAVE_HARDSID", "Support for HardSID.",
#ifndef HAVE_HARDSID
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) /* (optional only in sdl1) */
    { "HAVE_HWSCALE", "Enable arbitrary window scaling (deprecated)",
#ifndef HAVE_HWSCALE
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_IPV6", "Support IPv6",
#ifndef HAVE_IPV6
        0 },
#else
        1 },
#endif
/* All? */
    { "HAVE_LIBCURL", "Use the libcurl library",
#ifndef HAVE_LIBCURL
        0 },
#else
        1 },
#endif
#if defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE) /* (unix/windows) */
    { "HAVE_LIBIEEE1284", "Enable IEEE1284 library for parallel port", /* (-lieee1284) */
#ifndef HAVE_LIBIEEE1284
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX_COMPILE) || defined(MACOS_COMPILE) || defined(WINDOWS_COMPILE) /* (unix/osx/windows) */
    { "HAVE_MIDI", "Enable MIDI emulation.",
#ifndef HAVE_MIDI
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_MOUSE", "Enable mouse/paddle support",
#ifndef HAVE_MOUSE
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_LIGHTPEN", "Enable lightpen support",
#if !defined(HAVE_MOUSE) || !defined(HAVE_LIGHTPEN)
        0 },
#else
        1 },
#endif

/* (all) */
    { "HAVE_NANOSLEEP", "Use nanosleep instead of usleep",
#ifndef HAVE_NANOSLEEP
        0 },
#else
        1 },
#endif

/* (all) */
    { "HAVE_NETWORK", "Enable netplay support",
#ifndef HAVE_NETWORK
        0 },
#else
        1 },
#endif

/* FIXME: support for libnet < 1.1 should get removed */
#if defined(UNIX_COMPILE) /* (unix) */
    { "VICE_USE_LIBNET_1_1", "Enable support for libnet 1.1",
#ifndef VICE_USE_LIBNET_1_1
        0 },
#else
        1 },
#endif
#endif

#if defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE) /* (unix/windows) */
    { "HAVE_REALDEVICE", "Support for OpenCBM", /* (former CBM4Linux). */
#ifndef HAVE_REALDEVICE
        0 },
#else
        1 },
#endif
#endif
#if defined(BEOS_COMPILE) || defined(UNIX_COMPILE) || defined(WINDOWS_COMPILE) /* (beos/unix/windows) */
    { "HAVE_PARSID", "Support for ParSID.",
#ifndef HAVE_PARSID
        0 },
#else
        1 },
#endif
#endif

#if defined(UNIX_COMPILE) /* (unix) */
    { "HAVE_PORTSID", "Support for file device based access to ParSID.",
#ifndef HAVE_PORTSID
        0 },
#else
        1 },
#endif
#endif

/* (all) */
    { "HAVE_PNG", "Use the PNG library.",
#ifndef HAVE_PNG
        0 },
#else
        1 },
#endif

/* (all) */
    { "HAVE_FASTSID", "Enable FASTSID support. (deprecated)",
#ifndef HAVE_FASTSID
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_RESID", "Enable ReSID support.",
#ifndef HAVE_RESID
        0 },
#else
        1 },
#endif
#ifdef HAVE_RESID
/* (all) */
    { "HAVE_NEW_8580_FILTER", "Enable new 8580 Filter emulation.",
#ifndef HAVE_NEW_8580_FILTER
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_RESID_DTV", "Enable ReSID-DTV support.",
#ifndef HAVE_RESID_DTV
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_RS232DEV", "Enable RS232 emulation.",
#ifndef HAVE_RS232DEV
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_RS232NET", "Enable RS232 emulation. (via network)",
#ifndef HAVE_RS232NET
        0 },
#else
        1 },
#endif
#if defined(USE_SDLUI) || defined(USE_SDL2UI) /* (sdl) */
    { "HAVE_SDL_NUMJOYSTICKS", "The SDL_NumJoysticks function is available",
#ifndef HAVE_SDL_NUMJOYSTICKS
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX_COMPILE) /* (unix) */
    { "HAVE_SYS_AUDIO_H", "The <sys/audio.h> header file is available.",
#ifndef HAVE_SYS_AUDIO_H
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX_COMPILE) /* (unix) */
    { "HAVE_SYS_AUDIOIO_H", "The <sys/audioio.h> header file is available.",
#ifndef HAVE_SYS_AUDIOIO_H
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_RAWNET", "Enable raw ethernet emulation.",
#ifndef HAVE_RAWNET
        0 },
#else
        1 },
#endif

/* (all) */
    { "HAVE_PCAP", "Use the PCAP library.",
#ifndef HAVE_PCAP
        0 },
#else
        1 },
#endif

#if !defined(WINDOWS_COMPILE) /* not windows */
    { "HAVE_TUNTAP", "Support for TUN/TAP virtual network interface.",
#ifndef HAVE_TUNTAP
        0 },
#else
        1 },
#endif
#endif

#if defined(UNIX_COMPILE) /* (unix) */
    { "HAVE_CAPABILITIES", "Support for POSIX 1003.1e capabilities",
#ifndef HAVE_CAPABILITIES
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_X64_IMAGE", "Support for X64 image files (deprecated)",
#ifndef HAVE_X64_IMAGE
      0 },
#else
      1 },
#endif

/* (all) */
    { "HAVE_ZLIB", "Use the ZLIB compression library.",
#ifndef HAVE_ZLIB
        0 },
#else
        1 },
#endif
#ifdef UNIX_COMPILE /* (unix) */
    { "LINUX_JOYSTICK", "Enable support for Linux style joysticks.",
#ifndef LINUX_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#ifdef MACOS_COMPILE /* (osx) */
    { "MAC_JOYSTICK", "Enable Mac Joystick support.",
#ifndef MAC_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX_COMPILE) /* (unix) */
    { "USE_ALSA", "Enable ALSA support.",
#ifndef USE_ALSA
        0 },
#else
        1 },
#endif
#endif
#if defined(MACOS_COMPILE) /* (osx) */
    { "USE_COREAUDIO", "Enable CoreAudio support.",
#ifndef USE_COREAUDIO
        0 },
#else
        1 },
#endif
#endif
#if defined(WINDOWS_COMPILE) /* (windows) */
    { "USE_DXSOUND", "Enable DirectX sound support.",
#ifndef USE_DXSOUND
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "USE_LAMEMP3", "Enable lamemp3 encoding support.",
#ifndef USE_LAMEMP3
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_PORTAUDIO", "Enable portaudio sound input support.",
#ifndef USE_PORTAUDIO
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_MPG123", "Enable MP3 decoding support.",
#ifndef USE_MPG123
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_FLAC", "Enable FLAC support.",
#ifndef USE_FLAC
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_VORBIS", "Enable ogg/vorbis support.",
#ifndef USE_VORBIS
        0 },
#else
        1 },
#endif
#if defined(UNIX_COMPILE) /* (unix) */
    { "USE_OSS", "Enable OSS support.",
#ifndef USE_OSS
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "USE_PULSE", "Enable pulseaudio support.",
#ifndef USE_PULSE
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_SDL_AUDIO", "Enable SDL sound support.",
#ifndef USE_SDL_AUDIO
        0 },
#else
        1 },
#endif

    { "USE_VICE_THREAD", "UI and emu each on different threads.",
#  ifndef USE_VICE_THREAD
        0 },
#  else
        1 },
#  endif

/*
 * Used in Gtk3 for Unix. Gtk3 can also use fontconfig as a backend on MacOS
 * and Windows.
 */
    { "HAVE_FONTCONFIG", "Support dynamic font loading via Fontconfig.",
#ifndef HAVE_FONTCONFIG
        0 },
#else
        1 },
#endif

/* Gtk3UI debugging support */
    { "HAVE_DEBUG_GTK3UI", "Enable debugging messages in the Gtk3 UI.",
#ifndef HAVE_DEBUG_GTK3UI
        0 },
#else
        1 },
#endif

    { NULL, NULL, 0 }
};

const feature_list_t *vice_get_feature_list(void)
{
    return &featurelist[0];
}

#if 0
/* FIXME: appear in config.h but are not used in code: */

/* Support for direct PCI I/O access Catweasel MKIII. */
#define HAVE_CATWEASELMKIII_IO /**/
/* Enable Fullscreen support. */
/* #undef HAVE_FULLSCREEN */
/* Support for PCI/ISA HardSID. */
#define HAVE_HARDSID_IO /**/
/* Enable support for BSD style joysticks. */
/* #undef BSD_JOYSTICK */
/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */
/* WARNING win32 and osx bindist greps for this in config.h! */
/* External FFMPEG libraries are used */
#define EXTERNAL_FFMPEG /**/
/* WARNING osx bindist greps for this in config.h! */
/* External linking for lame libs */
#define HAVE_EXTERNAL_LAME /**/
/* WARNING: seems to be used in makefiles all over the place */
/* Enable the readline library */
/* #undef HAVE_READLINE */

/* appear in code, but should get removed */

/* FIXME: support for libnet < 1.1 should get removed (unix) */
/* Enable support for libnet 1.1 */
#define VICE_USE_LIBNET_1_1
#endif
