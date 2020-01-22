#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#endif
#include <emuframework/EmuApp.hh>
#include "internal.hh"
#include <snes9x.h>

enum
{
	CFGKEY_MULTITAP = 276, CFGKEY_BLOCK_INVALID_VRAM_ACCESS = 277,
	CFGKEY_VIDEO_SYSTEM = 278, CFGKEY_INPUT_PORT = 279,
	CFGKEY_AUDIO_DSP_INTERPOLATON = 280
};

#ifdef SNES9X_VERSION_1_4
const char *EmuSystem::configFilename = "Snes9x.config";
static constexpr int inputPortMinVal = 0;
#else
bool EmuSystem::hasBundledGames = true;
const char *EmuSystem::configFilename = "Snes9xP.config";
static constexpr int inputPortMinVal = -1;
#endif
Byte1Option optionMultitap{CFGKEY_MULTITAP, 0};
SByte1Option optionInputPort{CFGKEY_INPUT_PORT, inputPortMinVal, false, optionIsValidWithMinMax<inputPortMinVal, 3>};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
#ifndef SNES9X_VERSION_1_4
Byte1Option optionBlockInvalidVRAMAccess{CFGKEY_BLOCK_INVALID_VRAM_ACCESS, 1};
Byte1Option optionAudioDSPInterpolation{CFGKEY_AUDIO_DSP_INTERPOLATON, DSP_INTERPOLATION_GAUSSIAN, false, optionIsValidWithMax<4>};
#endif
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonSpacing(100);
	EmuApp::setDefaultVControlsButtonStagger(5); // original SNES layout
}

EmuSystem::Error EmuSystem::onOptionsLoaded()
{
	#ifndef SNES9X_VERSION_1_4
	SNES::dsp.spc_dsp.interpolation = optionAudioDSPInterpolation;
	#endif
	return {};
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return false;
		#ifndef SNES9X_VERSION_1_4
		bcase CFGKEY_AUDIO_DSP_INTERPOLATON: optionAudioDSPInterpolation.readFromIO(io, readSize);
		#endif
	}
	return true;
}

void EmuSystem::writeConfig(IO &io)
{
	#ifndef SNES9X_VERSION_1_4
	optionAudioDSPInterpolation.writeWithKeyIfNotDefault(io);
	#endif
}

void EmuSystem::onSessionOptionsLoaded()
{
	#ifndef SNES9X_VERSION_1_4
	Settings.BlockInvalidVRAMAccessMaster = optionBlockInvalidVRAMAccess;
	#endif
	snesInputPort = optionInputPort;
	if(gameIsRunning())
	{
		setupSNESInput();
	}
}

bool EmuSystem::resetSessionOptions()
{
	optionInputPort.reset();
	optionMultitap.reset();
	optionVideoSystem.reset();
	#ifndef SNES9X_VERSION_1_4
	optionBlockInvalidVRAMAccess.reset();
	#endif
	onSessionOptionsLoaded();
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_INPUT_PORT: optionInputPort.readFromIO(io, readSize);
		bcase CFGKEY_MULTITAP: optionMultitap.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		#ifndef SNES9X_VERSION_1_4
		bcase CFGKEY_BLOCK_INVALID_VRAM_ACCESS: optionBlockInvalidVRAMAccess.readFromIO(io, readSize);
		#endif
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionInputPort.writeWithKeyIfNotDefault(io);
	optionMultitap.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	#ifndef SNES9X_VERSION_1_4
	optionBlockInvalidVRAMAccess.writeWithKeyIfNotDefault(io);
	#endif
}
