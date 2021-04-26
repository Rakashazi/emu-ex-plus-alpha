#ifndef SNES9X_VERSION_1_4
#include <apu/apu.h>
#include <apu/bapu/snes/snes.hpp>
#include <ppu.h>
#include <fxemu.h>
#endif
#include <emuframework/EmuApp.hh>
#include "internal.hh"
#include <snes9x.h>

enum
{
	CFGKEY_MULTITAP = 276, CFGKEY_BLOCK_INVALID_VRAM_ACCESS = 277,
	CFGKEY_VIDEO_SYSTEM = 278, CFGKEY_INPUT_PORT = 279,
	CFGKEY_AUDIO_DSP_INTERPOLATON = 280, CFGKEY_SEPARATE_ECHO_BUFFER = 281,
	CFGKEY_SUPERFX_CLOCK_MULTIPLIER = 282
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
Byte1Option optionSeparateEchoBuffer{CFGKEY_SEPARATE_ECHO_BUFFER, 0};
Byte1Option optionSuperFXClockMultiplier{CFGKEY_SUPERFX_CLOCK_MULTIPLIER, 100, false, optionIsValidWithMinMax<5, 250>};
Byte1Option optionAudioDSPInterpolation{CFGKEY_AUDIO_DSP_INTERPOLATON, DSP_INTERPOLATION_GAUSSIAN, false, optionIsValidWithMax<4>};
#endif
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

#ifndef SNES9X_VERSION_1_4
void setSuperFXSpeedMultiplier(unsigned val)
{
	Settings.SuperFXClockMultiplier = val;
	S9xSetSuperFXTiming(val);
}
#endif

static void applyInputPortOption(int portVal)
{
	snesInputPort = portVal;
	if(EmuSystem::gameIsRunning())
	{
		setupSNESInput();
	}
}

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonSpacing(100);
	EmuApp::setDefaultVControlsButtonStagger(5); // original SNES layout
}

EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext)
{
	#ifndef SNES9X_VERSION_1_4
	SNES::dsp.spc_dsp.interpolation = optionAudioDSPInterpolation;
	#endif
	return {};
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
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

void EmuSystem::onSessionOptionsLoaded(EmuApp &)
{
	applyInputPortOption(optionInputPort);
	#ifndef SNES9X_VERSION_1_4
	PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess;
	SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer;
	setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier);
	#endif
}

bool EmuSystem::resetSessionOptions(EmuApp &)
{
	applyInputPortOption(optionInputPort.reset());
	optionMultitap.reset();
	optionVideoSystem.reset();
	#ifndef SNES9X_VERSION_1_4
	// reset emulations hacks
	PPU.BlockInvalidVRAMAccess = optionBlockInvalidVRAMAccess.reset();
	SNES::dsp.spc_dsp.separateEchoBuffer = optionSeparateEchoBuffer.reset();
	setSuperFXSpeedMultiplier(optionSuperFXClockMultiplier.reset());
	#endif
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_INPUT_PORT: optionInputPort.readFromIO(io, readSize);
		bcase CFGKEY_MULTITAP: optionMultitap.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		#ifndef SNES9X_VERSION_1_4
		bcase CFGKEY_BLOCK_INVALID_VRAM_ACCESS: optionBlockInvalidVRAMAccess.readFromIO(io, readSize);
		bcase CFGKEY_SEPARATE_ECHO_BUFFER: optionSeparateEchoBuffer.readFromIO(io, readSize);
		bcase CFGKEY_SUPERFX_CLOCK_MULTIPLIER: optionSuperFXClockMultiplier.readFromIO(io, readSize);
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
	optionSeparateEchoBuffer.writeWithKeyIfNotDefault(io);
	optionSuperFXClockMultiplier.writeWithKeyIfNotDefault(io);
	#endif
}
