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
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#include <fceu/sound.h>
#include <fceu/fceu.h>

namespace EmuEx
{

const char *EmuSystem::configFilename = "NesEmu.config";

std::span<const AspectRatioInfo> NesSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void NesSystem::onOptionsLoaded()
{
	FCEUI_SetSoundQuality(optionSoundQuality);
	FCEUI_DisableSpriteLimitation(!optionSpriteLimit);
	setDefaultPalette(appContext(), defaultPalettePath);
}

void NesSystem::onSessionOptionsLoaded(EmuApp &app)
{
	updateVideoPixmap(app.video, optionHorizontalVideoCrop, optionVisibleVideoLines);
}

bool NesSystem::resetSessionOptions(EmuApp &app)
{
	optionFourScore.reset();
	setupNESFourScore();
	optionVideoSystem.reset();
	inputPort1.reset();
	inputPort2.reset();
	replaceP2StartWithMicrophone = false;
	setupNESInputPorts();
	optionCompatibleFrameskip.reset();
	optionStartVideoLine = optionDefaultStartVideoLine;
	optionVisibleVideoLines = optionDefaultVisibleVideoLines;
	optionHorizontalVideoCrop.reset();
	updateVideoPixmap(app.video, optionHorizontalVideoCrop, optionVisibleVideoLines);
	overclock_enabled = 0;
	postrenderscanlines = 0;
	vblankscanlines = 0;
	return true;
}

bool NesSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_FDS_BIOS_PATH:
				return readStringOptionValue(io, fdsBiosPath);
			case CFGKEY_SPRITE_LIMIT: return readOptionValue(io, optionSpriteLimit);
			case CFGKEY_SOUND_QUALITY: return readOptionValue(io, optionSoundQuality);
			case CFGKEY_DEFAULT_VIDEO_SYSTEM: return readOptionValue(io, optionDefaultVideoSystem);
			case CFGKEY_DEFAULT_PALETTE_PATH:
				return readStringOptionValue(io, defaultPalettePath);
			case CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER:
				return readOptionValue<bool>(io, [](auto val){FCEUI_SetLowPass(val);});
			case CFGKEY_SWAP_DUTY_CYCLES: return readOptionValue(io, swapDuty);
			case CFGKEY_START_VIDEO_LINE: return readOptionValue(io, optionDefaultStartVideoLine);
			case CFGKEY_VISIBLE_VIDEO_LINES: return readOptionValue(io, optionDefaultVisibleVideoLines);
			case CFGKEY_CORRECT_LINE_ASPECT: return readOptionValue(io, optionCorrectLineAspect);
			case CFGKEY_FF_DURING_FDS_ACCESS: return readOptionValue(io, fastForwardDuringFdsAccess);
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, cheatsDir);
			case CFGKEY_PATCHES_PATH: return readStringOptionValue(io, patchesDir);
			case CFGKEY_PALETTE_PATH: return readStringOptionValue(io, palettesDir);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_FOUR_SCORE: return readOptionValue(io, optionFourScore);
			case CFGKEY_VIDEO_SYSTEM: return readOptionValue(io, optionVideoSystem);
			case CFGKEY_INPUT_PORT_1: return readOptionValue(io, inputPort1);
			case CFGKEY_INPUT_PORT_2: return readOptionValue(io, inputPort2);
			case CFGKEY_COMPATIBLE_FRAMESKIP: return readOptionValue(io, optionCompatibleFrameskip);
			case CFGKEY_START_VIDEO_LINE: return readOptionValue(io, optionStartVideoLine);
			case CFGKEY_VISIBLE_VIDEO_LINES: return readOptionValue(io, optionVisibleVideoLines);
			case CFGKEY_HORIZONTAL_VIDEO_CROP: return readOptionValue(io, optionHorizontalVideoCrop);
			case CFGKEY_OVERCLOCKING: return readOptionValue<bool>(io, [&](auto on){overclock_enabled = on;});
			case CFGKEY_OVERCLOCK_EXTRA_LINES: return readOptionValue<int16_t>(io,
				[&](auto v){if(v >= 0 && v <= maxExtraLinesPerFrame) postrenderscanlines = v;});
			case CFGKEY_OVERCLOCK_VBLANK_MULTIPLIER: return readOptionValue<int8_t>(io,
				[&](auto v){if(v >= 0 && v <= maxVBlankMultiplier) vblankscanlines = v;});
			case CFGKEY_P2_START_AS_FC_MIC: return readOptionValue(io, replaceP2StartWithMicrophone);
		}
	}
	return false;
}

void NesSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, optionSpriteLimit);
		writeOptionValueIfNotDefault(io, optionSoundQuality);
		writeStringOptionValue(io, CFGKEY_FDS_BIOS_PATH, fdsBiosPath);
		writeOptionValueIfNotDefault(io, optionDefaultVideoSystem);
		writeStringOptionValue(io, CFGKEY_DEFAULT_PALETTE_PATH, defaultPalettePath);
		if(swapDuty)
			writeOptionValue(io, CFGKEY_SWAP_DUTY_CYCLES, swapDuty);
		if(FSettings.lowpass)
			writeOptionValue(io, CFGKEY_DEFAULT_SOUND_LOW_PASS_FILTER, (bool)FSettings.lowpass);
		writeOptionValueIfNotDefault(io, optionDefaultStartVideoLine);
		writeOptionValueIfNotDefault(io, optionDefaultVisibleVideoLines);
		writeOptionValueIfNotDefault(io, optionCorrectLineAspect);
		writeOptionValueIfNotDefault(io, fastForwardDuringFdsAccess);
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
		writeStringOptionValue(io, CFGKEY_PATCHES_PATH, patchesDir);
		writeStringOptionValue(io, CFGKEY_PALETTE_PATH, palettesDir);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, optionFourScore);
		writeOptionValueIfNotDefault(io, optionVideoSystem);
		writeOptionValueIfNotDefault(io, inputPort1);
		writeOptionValueIfNotDefault(io, inputPort2);
		writeOptionValueIfNotDefault(io, optionCompatibleFrameskip);
		writeOptionValueIfNotDefault(io, optionStartVideoLine);
		writeOptionValueIfNotDefault(io, optionVisibleVideoLines);
		writeOptionValueIfNotDefault(io, optionHorizontalVideoCrop);
		writeOptionValueIfNotDefault(io, CFGKEY_OVERCLOCKING, bool(overclock_enabled), 0);
		writeOptionValueIfNotDefault(io, CFGKEY_OVERCLOCK_EXTRA_LINES, int16_t(postrenderscanlines), 0);
		writeOptionValueIfNotDefault(io, CFGKEY_OVERCLOCK_VBLANK_MULTIPLIER, int8_t(vblankscanlines), 0);
		writeOptionValueIfNotDefault(io, CFGKEY_P2_START_AS_FC_MIC, replaceP2StartWithMicrophone, false);
	}
}

}
