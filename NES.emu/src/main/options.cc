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
#include <fceu/sound.h>
#include <fceu/fceu.h>

namespace EmuEx
{

enum
{
	CFGKEY_FDS_BIOS_PATH = 270, CFGKEY_FOUR_SCORE = 271,
	CFGKEY_VIDEO_SYSTEM = 272, CFGKEY_SPRITE_LIMIT = 273,
	CFGKEY_SOUND_QUALITY = 274, CFGKEY_INPUT_PORT_1 = 275,
	CFGKEY_INPUT_PORT_2 = 276, CFGKEY_DEFAULT_PALETTE_PATH = 277,
	CFGKEY_DEFAULT_VIDEO_SYSTEM = 278, CFGKEY_COMPATIBLE_FRAMESKIP = 279,
	CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER = 280, CFGKEY_SWAP_DUTY_CYCLES = 281,
	CFGKEY_START_VIDEO_LINE = 282, CFGKEY_VISIBLE_VIDEO_LINES = 283,
	CFGKEY_HORIZONTAL_VIDEO_CROP = 284,
};

constexpr bool isSupportedStartingLine(uint8_t line)
{
	switch(line)
	{
		case 0:
		case 8:
			return true;
	}
	return false;
}

constexpr bool isSupportedLineCount(uint8_t lines)
{
	switch(lines)
	{
		case 224:
		case 232:
		case 240:
			return true;
	}
	return false;
}

const char *EmuSystem::configFilename = "NesEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
FS::PathString fdsBiosPath{};
Byte1Option optionFourScore{CFGKEY_FOUR_SCORE, 0};
SByte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, -1, false, optionIsValidWithMinMax<-1, 2>};
SByte1Option optionInputPort2{CFGKEY_INPUT_PORT_2, -1, false, optionIsValidWithMinMax<-1, 2>};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
Byte1Option optionDefaultVideoSystem{CFGKEY_DEFAULT_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
Byte1Option optionSpriteLimit{CFGKEY_SPRITE_LIMIT, 1};
Byte1Option optionSoundQuality{CFGKEY_SOUND_QUALITY, 0, false, optionIsValidWithMax<2>};
FS::PathString defaultPalettePath{};
Byte1Option optionCompatibleFrameskip{CFGKEY_COMPATIBLE_FRAMESKIP, 0};
Byte1Option optionStartVideoLine{CFGKEY_START_VIDEO_LINE, 8, false, isSupportedStartingLine};
Byte1Option optionVisibleVideoLines{CFGKEY_VISIBLE_VIDEO_LINES, 224, false, isSupportedLineCount};
Byte1Option optionHorizontalVideoCrop{CFGKEY_HORIZONTAL_VIDEO_CROP, 0};

void EmuSystem::onOptionsLoaded(IG::ApplicationContext ctx)
{
	FCEUI_SetSoundQuality(optionSoundQuality);
	FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
	setDefaultPalette(ctx, defaultPalettePath);
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &app)
{
	nesInputPortDev[0] = (ESI)(int)optionInputPort1;
	nesInputPortDev[1] = (ESI)(int)optionInputPort2;
	updateVideoPixmap(app.video(), optionHorizontalVideoCrop, optionVisibleVideoLines);
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
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
	optionStartVideoLine.reset();
	optionVisibleVideoLines.reset();
	optionHorizontalVideoCrop.reset();
	updateVideoPixmap(app.video(), optionHorizontalVideoCrop, optionVisibleVideoLines);
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
		bcase CFGKEY_START_VIDEO_LINE: optionStartVideoLine.readFromIO(io, readSize);
		bcase CFGKEY_VISIBLE_VIDEO_LINES: optionVisibleVideoLines.readFromIO(io, readSize);
		bcase CFGKEY_HORIZONTAL_VIDEO_CROP: optionHorizontalVideoCrop.readFromIO(io, readSize);
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
	optionStartVideoLine.writeWithKeyIfNotDefault(io);
	optionVisibleVideoLines.writeWithKeyIfNotDefault(io);
	optionHorizontalVideoCrop.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_FDS_BIOS_PATH:
			readStringOptionValue<FS::PathString>(io, readSize, [](auto &path){fdsBiosPath = path;});
		bcase CFGKEY_SPRITE_LIMIT: optionSpriteLimit.readFromIO(io, readSize);
		bcase CFGKEY_SOUND_QUALITY: optionSoundQuality.readFromIO(io, readSize);
		bcase CFGKEY_DEFAULT_VIDEO_SYSTEM: optionDefaultVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_DEFAULT_PALETTE_PATH:
			readStringOptionValue<FS::PathString>(io, readSize, [](auto &path){defaultPalettePath = path;});
		bcase CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER:
			readOptionValue<bool>(io, readSize, [](auto &val){FCEUI_SetLowPass(val);});
		bcase CFGKEY_SWAP_DUTY_CYCLES:
			readOptionValue<bool>(io, readSize, [](auto &val){swapDuty = val;});
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionSpriteLimit.writeWithKeyIfNotDefault(io);
	optionSoundQuality.writeWithKeyIfNotDefault(io);
	writeStringOptionValue(io, CFGKEY_FDS_BIOS_PATH, fdsBiosPath);
	optionDefaultVideoSystem.writeWithKeyIfNotDefault(io);
	writeStringOptionValue(io, CFGKEY_DEFAULT_PALETTE_PATH, defaultPalettePath);
	if(swapDuty)
		writeOptionValue(io, CFGKEY_SWAP_DUTY_CYCLES, swapDuty);
	if(FSettings.lowpass)
		writeOptionValue(io, CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER, (bool)FSettings.lowpass);
}

}
