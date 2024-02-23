/** \file   archdep_sound.c
 * \brief   Shared platform sound code
 * \author  David Hogan <david.q.hogan@gmail.com>
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

#include <stdio.h>
#include <stdlib.h>

#include "archdep_sound.h"

#include "sound.h"

#ifdef MACOS_COMPILE

#include <CoreAudio/CoreAudio.h>

static OSStatus on_default_device_changed(AudioObjectID inObjectID,
                                          UInt32 inNumberAddresses,
                                          const AudioObjectPropertyAddress inAddresses[],
                                          void *inClientData)
{
    /*
     * If we are monitoring the default audio device, then we launched without
     * requesting a specific device. Therefore when the default output device
     * changes, reinitialise sound so that our sound targets the new default device
     */

    sound_state_changed = 1;

    return 0;
}

void archdep_sound_enable_default_device_tracking(void)
{
    AudioObjectPropertyAddress outputDeviceAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    AudioObjectAddPropertyListener(kAudioObjectSystemObject,
                                &outputDeviceAddress,
                                on_default_device_changed,
                                nil);
}

#else /* #ifdef MACOS_COMPILE */

/* Not supported on puny Linux and Windows yet */

void archdep_sound_enable_default_device_tracking(void)
{
}

#endif /* #ifdef MACOS_COMPILE */
