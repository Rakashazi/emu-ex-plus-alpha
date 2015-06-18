
/*
 * features.c - List of compile time selectable features
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

/* FIXME: define "UNIX" for all supported unixish OS */
#if !defined(__OS2__) && !defined(BEOS_COMPILE) && !defined(__MSDOS__) && !defined(AMIGA_SUPPORT) && !defined(WIN32)
#define UNIX
#endif

static feature_list_t featurelist[] = {
#ifdef UNIX /* unix */
    { "BSD_JOYSTICK", "Enable support for BSD style joysticks.",
#ifndef BSD_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
/* all */
    { "DEBUG", "Enable debugging code",
#ifndef DEBUG
        0 },
#else
        1 },
#endif
#ifdef UNIX /* unix */
    { "ENABLE_NLS", "Define if NLS support is enabled.",
#ifndef ENABLE_NLS
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "FEATURE_CPUMEMHISTORY", "Use the memmap feature.",
#ifndef FEATURE_CPUMEMHISTORY
        0 },
#else
        1 },
#endif
#ifdef UNIX /* (unix) */
    { "HAS_DIGITAL_JOYSTICK", "Enable emulation for digital joysticks.",
#ifndef HAS_DIGITAL_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#ifdef MACOSX_SUPPORT /* (osx) */
    { "HAS_HIDMGR", "Enable Mac IOHIDManager Joystick driver.",
#ifndef HAS_HIDMGR
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAS_JOYSTICK", "Enable joystick emulation.",
#ifndef HAS_JOYSTICK
        0 },
#else
        1 },
#endif
#ifdef UNIX /* (unix) */
    { "HAS_USB_JOYSTICK", "Enable emulation for USB joysticks.",
#ifndef HAS_USB_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#ifdef __MSDOS__ /* (dos) */
    { "HAVE_ALLEGRO_H", "Define to 1 if you have the <allegro.h> header file.",
#ifndef HAVE_ALLEGRO_H
        0 },
#else
        1 },
#endif
#endif
#if defined(MACOSX_SUPPORT) /* (osx ???) */
    { "HAVE_AUDIO_UNIT", "Enable AudioUnit support.",
#ifndef HAVE_AUDIO_UNIT
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_CAIRO", "Enable Cairo rendering support",
#ifndef HAVE_CAIRO
        0 },
#else
        1 },
#endif
#endif
#if defined(AMIGA_SUPPORT) || defined(__MSDOS__) || defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (amiga/dos/sdl/unix/windows) */
    { "HAVE_CATWEASELMKIII", "Support for Catweasel MKIII.",
#ifndef HAVE_CATWEASELMKIII
        0 },
#else
        1 },
#endif
#endif
#ifdef WIN32  /* (windows) */
        { "HAVE_D3D9_H", "Support for DirectX9.",
#ifndef HAVE_D3D9_H
        0 },
#else
        1 },
#endif
#endif
#ifdef AMIGA_SUPPORT  /* (amiga) */
    { "HAVE_DEVICES_AHI_H", "Define to 1 if you have the <devices/ahi.h> header file.",
#ifndef HAVE_DEVICES_AHI_H
        0 },
#else
        1 },
#endif
#endif
#ifdef WIN32 /* (windows) */
    { "HAVE_DINPUT", "Use DirectInput joystick driver",
#ifndef HAVE_DINPUT
        0 },
#else
        1 },
#endif
#endif
#if defined(UNIX) || defined(MACOSX_SUPPORT) || defined(WIN32) /* (unix/osx/windows) */
    { "HAVE_DYNLIB_SUPPORT", "Support for dynamic library loading.",
#ifndef HAVE_DYNLIB_SUPPORT
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "HAVE_FFMPEG", "Have FFMPEG av* libs available",
#ifndef HAVE_FFMPEG
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "HAVE_FFMPEG_HEADER_SUBDIRS", "FFMPEG uses subdirs for headers",
#ifndef HAVE_FFMPEG_HEADER_SUBDIRS
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "HAVE_FFMPEG_SWSCALE", "Have FFMPEG swscale lib available",
#ifndef HAVE_FFMPEG_SWSCALE
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "SHARED_FFMPEG", "FFMPEG libraries are shared",
#ifndef SHARED_FFMPEG
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "STATIC_FFMPEG", "FFMPEG libraries are static",
#ifndef STATIC_FFMPEG
        0 },
#else
        1 },
#endif
#endif
 /* (all) */
    { "HAVE_FULLSCREEN", "Enable Fullscreen support.",
#ifndef HAVE_FULLSCREEN
        0 },
#else
        1 },
#endif
 /* (all) */
    { "HAVE_GIF", "Can we use the GIF or UNGIF library?",
#ifndef HAVE_GIF
        0 },
#else
        1 },
#endif
#if defined(AMIGA_SUPPORT) || defined(__MSDOS__) || defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (amiga/dos/sdl/unix/windows) */
    { "HAVE_HARDSID", "Support for HardSID.",
#ifndef HAVE_HARDSID
        0 },
#else
        1 },
#endif
#endif
#if defined(AMIGA_SUPPORT) || defined(USE_SDLUI) || defined(UNIX) || defined(MACOSX_SUPPORT) || defined(WIN32) /* (amiga/sdl/unix/osx/windows) */
    { "HAVE_HWSCALE", "Enable arbitrary window scaling",
#ifndef HAVE_HWSCALE
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_IPV6", "Define if ipv6 can be used",
#ifndef HAVE_IPV6
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_JPEG", "Can we use the JPEG library?",
#ifndef HAVE_JPEG
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_LIBIEEE1284", "Define to 1 if you have the `ieee1284' library", /* (-lieee1284) */
#ifndef HAVE_LIBIEEE1284
        0 },
#else
        1 },
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_LIBXPM", "Is libXpm available?",
#ifndef HAVE_LIBXPM
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) || defined(MACOSX_SUPPORT) || defined(WIN32) /* (sdl/unix/osx/windows) */
    { "HAVE_MIDI", "Enable MIDI emulation.",
#ifndef HAVE_MIDI
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_MOUSE", "Enable 1351 mouse support",
#ifndef HAVE_MOUSE
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
#if defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (sdl/unix/windows) */
    { "HAVE_OPENCBM", "Support for OpenCBM", /* (former CBM4Linux). */
#ifndef HAVE_OPENCBM
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "HAVE_OPENGL_SYNC", "Enable openGL synchronization",
#ifndef HAVE_OPENGL_SYNC
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_PANGO", "Enable support for Pango",
#ifndef HAVE_PANGO
        0 },
#else
        1 },
#endif
#endif
#if defined(__MSDOS__) || defined(USE_SDLUI) || defined(UNIX) || defined(WIN32) /* (dos/sdl/unix/windows) */
    { "HAVE_PARSID", "Support for ParSID.",
#ifndef HAVE_PARSID
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_PNG", "Can we use the PNG library?",
#ifndef HAVE_PNG
        0 },
#else
        1 },
#endif
#ifdef AMIGA_SUPPORT /* (amiga) */
    { "HAVE_PROTO_CYBERGRAPHICS_H", "Define to 1 if you have the <proto/cybergraphics.h> header file.",
#ifndef HAVE_PROTO_CYBERGRAPHICS_H
        0 },
#else
        1 },
#endif
#endif
#ifdef AMIGA_SUPPORT /* (amiga) */
    { "HAVE_PROTO_OPENPCI_H", "Define to 1 if you have the <proto/openpci.h> header file.",
#ifndef HAVE_PROTO_OPENPCI_H
        0 },
#else
        1 },
#endif
#endif
#ifdef AMIGA_SUPPORT /* (amiga) */
    { "HAVE_PROTO_PICASSO96API_H", "Define to 1 if you have the <proto/Picasso96API.h> header file.",
#ifndef HAVE_PROTO_PICASSO96API_H
        0 },
#else
        1 },
#endif
#endif
#ifdef AMIGA_SUPPORT /* (amiga) */
    { "HAVE_PROTO_PICASSO96_H", "Define to 1 if you have the <proto/Picasso96.h> header file.",
#ifndef HAVE_PROTO_PICASSO96_H
        0 },
#else
        1 },
#endif
#endif
#if defined(MACOSX_SUPPORT) || defined(WIN32) /* (osx/windows) */
    { "HAVE_QUICKTIME", "Enable QuickTime support.",
#ifndef HAVE_QUICKTIME
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_RAWDRIVE", "Support for block device disk image access.",
#ifndef HAVE_RAWDRIVE
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_RESID", "This version provides ReSID support.",
#ifndef HAVE_RESID
        0 },
#else
        1 },
#endif
/* (all) */
    { "HAVE_RESID_DTV", "This version provides ReSID-DTV support.",
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
#ifdef USE_SDLUI /* (sdl) */
    { "HAVE_SDL_NUMJOYSTICKS", "Define to 1 if you have the `SDL_NumJoysticks' function.",
#ifndef HAVE_SDL_NUMJOYSTICKS
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "HAVE_SYS_AUDIO_H", "Define to 1 if you have the <sys/audio.h> header file.",
#ifndef HAVE_SYS_AUDIO_H
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "HAVE_SYS_AUDIOIO_H", "Define to 1 if you have the <sys/audioio.h> header file.",
#ifndef HAVE_SYS_AUDIOIO_H
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_TFE", "Support for The Final Ethernet",
#ifndef HAVE_TFE
        0 },
#else
        1 },
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_VTE", "VTE support",
#ifndef HAVE_VTE
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_XRANDR", "Enable XRandR extension.",
#ifndef HAVE_XRANDR
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "HAVE_XVIDEO", "Enable XVideo support.",
#ifndef HAVE_XVIDEO
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "HAVE_ZLIB", "Can we use the ZLIB compression library?",
#ifndef HAVE_ZLIB
        0 },
#else
        1 },
#endif
#ifdef UNIX /* (unix) */
    { "LINUX_JOYSTICK", "Enable support for Linux style joysticks.",
#ifndef LINUX_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#ifdef MACOSX_SUPPORT /* (osx) */
    { "MAC_JOYSTICK", "Enable Mac Joystick support.",
#ifndef MAC_JOYSTICK
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "USE_AIX_AUDIO", "Enable aix sound support.",
#ifndef USE_AIX_AUDIO
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "USE_ALSA", "Enable alsa support.",
#ifndef USE_ALSA
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "USE_ARTS", "Enable aRts support.",
#ifndef USE_ARTS
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(MACOSX_SUPPORT) /* (sdl/osx) */
    { "USE_COREAUDIO", "Enable CoreAudio support.",
#ifndef USE_COREAUDIO
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "USE_DMEDIA", "Enable sgi sound support.",
#ifndef USE_DMEDIA
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(WIN32) /* (sdl/windows) */
    { "USE_DXSOUND", "Enable directx sound support.",
#ifndef USE_DXSOUND
        0 },
#else
        1 },
#endif
#endif
/* (all) */
    { "USE_EMBEDDED", "Use embedded data files.",
#ifndef USE_EMBEDDED
        0 },
#else
        1 },
#endif
/* (all) */
    { "USE_LAMEMP3", "Enable lamemp3 support.",
#ifndef USE_LAMEMP3
        0 },
#else
        1 },
#endif
#ifdef __MSDOS__ /* (dos) */
    { "USE_MIDAS_SOUND", "Use MIDAS Sound System instead of the Allegro library.",
#ifndef USE_MIDAS_SOUND
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "USE_MITSHM", "Enable MITSHM extensions.",
#ifndef USE_MITSHM
        0 },
#else
        1 },
#endif
#endif
#if defined(USE_SDLUI) || defined(UNIX) /* (sdl/unix) */
    { "USE_OSS", "Enable oss support.",
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
#ifdef UNIX /* (unix) */
    { "USE_UI_THREADS", "Enable multithreaded UI.",
#ifndef USE_UI_THREADS
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "USE_XAW3D", "Enable Xaw3d.",
#ifndef USE_XAW3D
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "USE_XF86_EXTENSIONS", "Enable XF86 extensions.",
#ifndef USE_XF86_EXTENSIONS
        0 },
#else
        1 },
#endif
#endif
#ifdef UNIX /* (unix) */
    { "USE_XF86_VIDMODE_EXT", "Enable XF86 VidMode extensions.",
#ifndef USE_XF86_VIDMODE_EXT
        0 },
#else
        1 },
#endif
#endif
    { NULL, NULL, 0 }
};

feature_list_t *vice_get_feature_list(void)
{
    return &featurelist[0];
}
