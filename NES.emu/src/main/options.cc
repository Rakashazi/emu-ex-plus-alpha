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
	CFGKEY_SOUND_QUALITY = 274, CFGKEY_INPUT_PORT_1 = 275,
	CFGKEY_INPUT_PORT_2 = 276, CFGKEY_DEFAULT_PALETTE_PATH = 277,
	CFGKEY_DEFAULT_VIDEO_SYSTEM = 278, CFGKEY_COMPATIBLE_FRAMESKIP = 279
};

const char *EmuSystem::configFilename = "NesEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
FS::PathString fdsBiosPath{};
PathOption optionFdsBiosPath{CFGKEY_FDS_BIOS_PATH, fdsBiosPath, ""};
Byte1Option optionFourScore{CFGKEY_FOUR_SCORE, 0};
SByte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, -1, false, optionIsValidWithMinMax<-1, 2>};
SByte1Option optionInputPort2{CFGKEY_INPUT_PORT_2, -1, false, optionIsValidWithMinMax<-1, 2>};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
Byte1Option optionDefaultVideoSystem{CFGKEY_DEFAULT_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
Byte1Option optionSpriteLimit{CFGKEY_SPRITE_LIMIT, 1};
Byte1Option optionSoundQuality{CFGKEY_SOUND_QUALITY, 0, false, optionIsValidWithMax<2>};
FS::PathString defaultPalettePath{};
PathOption optionDefaultPalettePath{CFGKEY_DEFAULT_PALETTE_PATH, defaultPalettePath, ""};
Byte1Option optionCompatibleFrameskip{CFGKEY_COMPATIBLE_FRAMESKIP, 0};

EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext ctx)
{
	FCEUI_SetSoundQuality(optionSoundQuality);
	FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
	setDefaultPalette(ctx, defaultPalettePath.data());
	return {};
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &)
{
	nesInputPortDev[0] = (ESI)(int)optionInputPort1;
	nesInputPortDev[1] = (ESI)(int)optionInputPort2;
}

bool EmuSystem::resetSessionOptions(EmuApp &)
{
	optionFourScore.reset();
	setupNESFourScore();
	optionVideoSystem.reset();
	optionInputPort1.reset();
	optionInputPort2.reset();
	nesInputPortDev[0] = (ESI)(int)optionInputPort1;
	nesInputPortDev[1] = (ESI)(int)optionInputPort2;
	setupNESInputPorts();
	optionCompatibleFrameskip.reset();
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FOUR_SCORE: optionFourScore.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_INPUT_PORT_1: optionInputPort1.readFromIO(io, readSize);
		bcase CFGKEY_INPUT_PORT_2: optionInputPort2.readFromIO(io, readSize);
		bcase CFGKEY_COMPATIBLE_FRAMESKIP: optionCompatibleFrameskip.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionFourScore.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	optionInputPort1.writeWithKeyIfNotDefault(io);
	optionInputPort2.writeWithKeyIfNotDefault(io);
	optionCompatibleFrameskip.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FDS_BIOS_PATH: optionFdsBiosPath.readFromIO(io, readSize);
		bcase CFGKEY_SPRITE_LIMIT: optionSpriteLimit.readFromIO(io, readSize);
		bcase CFGKEY_SOUND_QUALITY: optionSoundQuality.readFromIO(io, readSize);
		bcase CFGKEY_DEFAULT_VIDEO_SYSTEM: optionDefaultVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_DEFAULT_PALETTE_PATH: optionDefaultPalettePath.readFromIO(io, readSize);
		logMsg("fds bios path %s", fdsBiosPath.data());
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionSpriteLimit.writeWithKeyIfNotDefault(io);
	optionSoundQuality.writeWithKeyIfNotDefault(io);
	optionFdsBiosPath.writeToIO(io);
	optionDefaultVideoSystem.writeWithKeyIfNotDefault(io);
	optionDefaultPalettePath.writeToIO(io);
}
