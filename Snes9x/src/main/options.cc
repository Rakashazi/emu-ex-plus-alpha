#include <emuframework/EmuApp.hh>
#include <emuframework/Option.hh>
#include "MainSystem.hh"
#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#include <fxemu.h>
#endif

namespace EmuEx
{

#ifdef SNES9X_VERSION_1_4
const char *EmuSystem::configFilename = "Snes9x.config";
#else
bool EmuSystem::hasBundledGames = true;
const char *EmuSystem::configFilename = "Snes9xP.config";
#endif

std::span<const AspectRatioInfo> Snes9xSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

#ifndef SNES9X_VERSION_1_4
void setSuperFXSpeedMultiplier(unsigned val)
{
	Settings.SuperFXClockMultiplier = val;
	S9xSetSuperFXTiming(val);
}
#endif

void Snes9xSystem::applyInputPortOption(int portVal, VController &vCtrl)
{
	snesInputPort = portVal;
	if(hasContent())
	{
		setupSNESInput(vCtrl);
	}
}

void Snes9xSystem::onOptionsLoaded()
{
	#ifndef SNES9X_VERSION_1_4
	SNES::dsp.spc_dsp.interpolation = optionAudioDSPInterpolation;
	#endif
}

bool Snes9xSystem::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			#ifndef SNES9X_VERSION_1_4
			case CFGKEY_AUDIO_DSP_INTERPOLATON: return readOptionValue(io, optionAudioDSPInterpolation);
			#endif
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, cheatsDir);
			case CFGKEY_PATCHES_PATH: return readStringOptionValue(io, patchesDir);
			case CFGKEY_SATELLAVIEW_PATH: return readStringOptionValue(io, satDir);
			case CFGKEY_SUFAMI_BIOS_PATH: return readStringOptionValue(io, sufamiBiosPath);
			case CFGKEY_BSX_BIOS_PATH: return readStringOptionValue(io, bsxBiosPath);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_INPUT_PORT: return readOptionValue(io, optionInputPort);
			case CFGKEY_MULTITAP: return readOptionValue(io, optionMultitap);
			case CFGKEY_VIDEO_SYSTEM: return readOptionValue(io, optionVideoSystem);
			case CFGKEY_ALLOW_EXTENDED_VIDEO_LINES: return readOptionValue(io, optionAllowExtendedVideoLines);
			case CFGKEY_DEINTERLACE_MODE: return readOptionValue(io, deinterlaceMode);
			#ifndef SNES9X_VERSION_1_4
			case CFGKEY_BLOCK_INVALID_VRAM_ACCESS: return readOptionValue(io, optionBlockInvalidVRAMAccess);
			case CFGKEY_SEPARATE_ECHO_BUFFER: return readOptionValue(io, optionSeparateEchoBuffer);
			case CFGKEY_SUPERFX_CLOCK_MULTIPLIER: return readOptionValue(io, optionSuperFXClockMultiplier);
			#endif
		}
	}
	return false;
}

void Snes9xSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		#ifndef SNES9X_VERSION_1_4
		writeOptionValueIfNotDefault(io, optionAudioDSPInterpolation);
		#endif
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
		writeStringOptionValue(io, CFGKEY_PATCHES_PATH, patchesDir);
		writeStringOptionValueIfNotDefault(io, CFGKEY_SATELLAVIEW_PATH, satDir, optionUserPathContentToken);
		writeStringOptionValue(io, CFGKEY_SUFAMI_BIOS_PATH, sufamiBiosPath);
		writeStringOptionValue(io, CFGKEY_BSX_BIOS_PATH, bsxBiosPath);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, optionInputPort);
		writeOptionValueIfNotDefault(io, optionMultitap);
		writeOptionValueIfNotDefault(io, optionVideoSystem);
		writeOptionValueIfNotDefault(io, optionAllowExtendedVideoLines);
		writeOptionValueIfNotDefault(io, CFGKEY_DEINTERLACE_MODE, deinterlaceMode, DeinterlaceMode::Bob);
		#ifndef SNES9X_VERSION_1_4
		writeOptionValueIfNotDefault(io, optionBlockInvalidVRAMAccess);
		writeOptionValueIfNotDefault(io, optionSeparateEchoBuffer);
		writeOptionValueIfNotDefault(io, optionSuperFXClockMultiplier);
		#endif
	}
}

void Snes9xSystem::onSessionOptionsLoaded(EmuApp &app)
{
	applyInputPortOption(optionInputPort, app.defaultVController());
	#ifndef SNES9X_VERSION_1_4
	PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess;
	SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer;
	setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier);
	#endif
}

bool Snes9xSystem::resetSessionOptions(EmuApp &app)
{
	applyInputPortOption(optionInputPort.reset(), app.defaultVController());
	optionMultitap.reset();
	optionVideoSystem.reset();
	optionAllowExtendedVideoLines.reset();
	deinterlaceMode = DeinterlaceMode::Bob;
	#ifndef SNES9X_VERSION_1_4
	// reset emulations hacks
	PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess.reset();
	SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer.reset();
	setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier.reset());
	#endif
	return true;
}

}
