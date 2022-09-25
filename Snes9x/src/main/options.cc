#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#include <fxemu.h>
#endif
#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"
#include <snes9x.h>

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
		{"8:7", {8, 7}},
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

bool Snes9xSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			#ifndef SNES9X_VERSION_1_4
			case CFGKEY_AUDIO_DSP_INTERPOLATON: return optionAudioDSPInterpolation.readFromIO(io, readSize);
			#endif
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, readSize, cheatsDir);
			case CFGKEY_PATCHES_PATH: return readStringOptionValue(io, readSize, patchesDir);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_INPUT_PORT: return optionInputPort.readFromIO(io, readSize);
			case CFGKEY_MULTITAP: return optionMultitap.readFromIO(io, readSize);
			case CFGKEY_VIDEO_SYSTEM: return optionVideoSystem.readFromIO(io, readSize);
			case CFGKEY_ALLOW_EXTENDED_VIDEO_LINES: return optionAllowExtendedVideoLines.readFromIO(io, readSize);
			#ifndef SNES9X_VERSION_1_4
			case CFGKEY_BLOCK_INVALID_VRAM_ACCESS: return optionBlockInvalidVRAMAccess.readFromIO(io, readSize);
			case CFGKEY_SEPARATE_ECHO_BUFFER: return optionSeparateEchoBuffer.readFromIO(io, readSize);
			case CFGKEY_SUPERFX_CLOCK_MULTIPLIER: return optionSuperFXClockMultiplier.readFromIO(io, readSize);
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
		optionAudioDSPInterpolation.writeWithKeyIfNotDefault(io);
		#endif
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
		writeStringOptionValue(io, CFGKEY_PATCHES_PATH, patchesDir);
	}
	else if(type == ConfigType::SESSION)
	{
		optionInputPort.writeWithKeyIfNotDefault(io);
		optionMultitap.writeWithKeyIfNotDefault(io);
		optionVideoSystem.writeWithKeyIfNotDefault(io);
		optionAllowExtendedVideoLines.writeWithKeyIfNotDefault(io);
		#ifndef SNES9X_VERSION_1_4
		optionBlockInvalidVRAMAccess.writeWithKeyIfNotDefault(io);
		optionSeparateEchoBuffer.writeWithKeyIfNotDefault(io);
		optionSuperFXClockMultiplier.writeWithKeyIfNotDefault(io);
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
	#ifndef SNES9X_VERSION_1_4
	// reset emulations hacks
	PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess.reset();
	SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer.reset();
	setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier.reset());
	#endif
	return true;
}

}
