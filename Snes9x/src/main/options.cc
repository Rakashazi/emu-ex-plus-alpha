#include <emuframework/EmuApp.hh>
#include "internal.hh"
#include <snes9x.h>

enum
{
	CFGKEY_MULTITAP = 276, CFGKEY_BLOCK_INVALID_VRAM_ACCESS = 277,
	CFGKEY_VIDEO_SYSTEM = 278
};

#ifdef SNES9X_VERSION_1_4
const char *EmuSystem::configFilename = "Snes9x.config";
#else
bool EmuSystem::hasBundledGames = true;
const char *EmuSystem::configFilename = "Snes9xP.config";
#endif
Byte1Option optionMultitap{CFGKEY_MULTITAP, 0};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<3>};
#ifndef SNES9X_VERSION_1_4
Byte1Option optionBlockInvalidVRAMAccess{CFGKEY_BLOCK_INVALID_VRAM_ACCESS, 1};
#endif
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		{"8:7", 8, 7},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonSize(700);
	EmuApp::setDefaultVControlsButtonSpacing(100);
	EmuApp::setDefaultVControlsButtonStagger(5); // original SNES layout
}

EmuSystem::Error EmuSystem::onOptionsLoaded()
{
	#ifndef SNES9X_VERSION_1_4
	Settings.BlockInvalidVRAMAccessMaster = optionBlockInvalidVRAMAccess;
	#endif
	return {};
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_MULTITAP: optionMultitap.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		#ifndef SNES9X_VERSION_1_4
		bcase CFGKEY_BLOCK_INVALID_VRAM_ACCESS: optionBlockInvalidVRAMAccess.readFromIO(io, readSize);
		#endif
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionMultitap.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	#ifndef SNES9X_VERSION_1_4
	optionBlockInvalidVRAMAccess.writeWithKeyIfNotDefault(io);
	#endif
}
