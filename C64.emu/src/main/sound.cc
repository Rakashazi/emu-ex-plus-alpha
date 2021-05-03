/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#define LOGTAG "sound"
#include <emuframework/EmuAudio.hh>
#include "internal.hh"

extern "C"
{
	#include "sound.h"
}

static int soundInit(const char *param, int *speed,
		   int *fragsize, int *fragnr, int *channels)
{
	logMsg("sound init %dHz, %d fragsize, %d fragnr, %d channels", *speed, *fragsize, *fragnr, *channels);
	assert(*channels == 1);
	return 0;
}

static int soundWrite(int16_t *pbuf, size_t nr)
{
	//logMsg("sound write %zd", nr);
	if(audioPtr) [[likely]]
		audioPtr->writeFrames(pbuf, nr);
	return 0;
}

static sound_device_t soundDevice =
{
    "dummy",
    soundInit,
    soundWrite,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    0,
    2
};

CLINK int sound_init_dummy_device()
{
	return plugin.sound_register_device(&soundDevice);
}
