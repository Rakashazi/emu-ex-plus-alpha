/*  This file is part of NES.emu.

	NES.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	NES.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with NES.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include "internal.hh"

enum
{
	CFGKEY_FDS_BIOS_PATH = 270, CFGKEY_FOUR_SCORE = 271,
	CFGKEY_VIDEO_SYSTEM = 272, CFGKEY_SPRITE_LIMIT = 273,
	CFGKEY_SOUND_QUALITY = 274
};

const char *EmuSystem::configFilename = "NesEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);
FS::PathString fdsBiosPath{};
PathOption optionFdsBiosPath{CFGKEY_FDS_BIOS_PATH, fdsBiosPath, ""};
Byte1Option optionFourScore{CFGKEY_FOUR_SCORE, 0};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
Byte1Option optionSpriteLimit{CFGKEY_SPRITE_LIMIT, 1};
Byte1Option optionSoundQuality{CFGKEY_SOUND_QUALITY, 0, false, optionIsValidWithMax<2>};

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FOUR_SCORE: optionFourScore.readFromIO(io, readSize);
		bcase CFGKEY_FDS_BIOS_PATH: optionFdsBiosPath.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_SPRITE_LIMIT: optionSpriteLimit.readFromIO(io, readSize);
		bcase CFGKEY_SOUND_QUALITY: optionSoundQuality.readFromIO(io, readSize);
		logMsg("fds bios path %s", fdsBiosPath.data());
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionFourScore.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	optionSpriteLimit.writeWithKeyIfNotDefault(io);
	optionSoundQuality.writeWithKeyIfNotDefault(io);
	optionFdsBiosPath.writeToIO(io);
}
