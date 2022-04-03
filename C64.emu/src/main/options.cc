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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include "internal.hh"
#include <imagine/util/format.hh>

extern "C"
{
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
	#include "drive.h"
	#include "vicii.h"
	#include "sid/sid.h"
	#include "sid/sid-resources.h"
}

namespace EmuEx
{

enum
{
	CFGKEY_DRIVE_TRUE_EMULATION = 256, CFGKEY_AUTOSTART_WARP = 257,
	CFGKEY_AUTOSTART_TDE = 258, CFGKEY_C64_MODEL = 259,
	CFGKEY_BORDER_MODE = 260, CFGKEY_SWAP_JOYSTICK_PORTS = 261,
	CFGKEY_SID_ENGINE = 262, CFGKEY_CROP_NORMAL_BORDERS = 263,
	CFGKEY_SYSTEM_FILE_PATH = 264, CFGKEY_DTV_MODEL = 265,
	CFGKEY_C128_MODEL = 266, CFGKEY_SUPER_CPU_MODEL = 267,
	CFGKEY_CBM2_MODEL = 268, CFGKEY_CBM5x0_MODEL = 269,
	CFGKEY_PET_MODEL = 270, CFGKEY_PLUS4_MODEL = 271,
	CFGKEY_VIC20_MODEL = 272, CFGKEY_VICE_SYSTEM = 273,
	CFGKEY_VIRTUAL_DEVICE_TRAPS = 274, CFGKEY_RESID_SAMPLING = 275,
	CFGKEY_MODEL = 276, CFGKEY_AUTOSTART_BASIC_LOAD = 277,
	CFGKEY_VIC20_RAM_EXPANSIONS = 278, CFGKEY_AUTOSTART_ON_LOAD = 279,
	CFGKEY_PALETTE_NAME = 280, CFGKEY_C64_RAM_EXPANSION_MODULE = 281,
	CFGKEY_DEFAULT_MODEL = 282, CFGKEY_DEFAULT_PALETTE_NAME = 283,
	CFGKEY_DRIVE8_TYPE = 284, CFGKEY_DRIVE9_TYPE = 285,
	CFGKEY_DRIVE10_TYPE = 286, CFGKEY_DRIVE11_TYPE = 287,
};

const char *EmuSystem::configFilename = "C64Emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionDriveTrueEmulation(CFGKEY_DRIVE_TRUE_EMULATION, 0);
Byte1Option optionCropNormalBorders(CFGKEY_CROP_NORMAL_BORDERS, 1);
Byte1Option optionAutostartWarp(CFGKEY_AUTOSTART_WARP, 1);
Byte1Option optionAutostartTDE(CFGKEY_AUTOSTART_TDE, 0);
Byte1Option optionAutostartBasicLoad(CFGKEY_AUTOSTART_BASIC_LOAD, 0);
Byte1Option optionViceSystem(CFGKEY_VICE_SYSTEM, VICE_SYSTEM_C64, false,
	optionIsValidWithMax<VicePlugin::SYSTEMS-1, uint8_t>);
SByte1Option optionModel{};
SByte1Option optionDefaultModel{};
Byte1Option optionBorderMode(CFGKEY_BORDER_MODE, VICII_NORMAL_BORDERS);
Byte1Option optionSidEngine(CFGKEY_SID_ENGINE, SID_ENGINE_RESID, false,
	optionIsValidWithMax<1, uint8_t>);
Byte1Option optionReSidSampling(CFGKEY_RESID_SAMPLING, SID_RESID_SAMPLING_INTERPOLATION, false,
	optionIsValidWithMax<3, uint8_t>);
Byte1Option optionSwapJoystickPorts(CFGKEY_SWAP_JOYSTICK_PORTS, JoystickMode::NORMAL, false,
	optionIsValidWithMax<JoystickMode::KEYBOARD>);
Byte1Option optionAutostartOnLaunch(CFGKEY_AUTOSTART_ON_LOAD, 1);

// VIC-20 specific
Byte1Option optionVic20RamExpansions(CFGKEY_VIC20_RAM_EXPANSIONS, 0);

// C64 specific
Byte2Option optionC64RamExpansionModule(CFGKEY_C64_RAM_EXPANSION_MODULE, 0);

std::string defaultPaletteName{};

static std::array<char, 21> externalPaletteResStr{};
static std::array<char, 17> paletteFileResStr{};

void setPaletteResources(const char *palName)
{
	if(palName && strlen(palName))
	{
		logMsg("setting external palette:%s", palName);
		setStringResource(paletteFileResStr.data(), palName);
		setIntResource(externalPaletteResStr.data(), 1);
	}
	else
	{
		logMsg("setting internal palette");
		setIntResource(externalPaletteResStr.data(), 0);
	}
}

bool usingExternalPalette()
{
	return intResource(externalPaletteResStr.data());
}

const char *externalPaletteName()
{
	return stringResource(paletteFileResStr.data());
}

const char *paletteName()
{
	if(usingExternalPalette())
		return externalPaletteName();
	else
		return "";
}

FS::FileString EmuSystem::configName() const
{
	return FS::FileString{plugin.configName};
}

static bool modelIdIsValid(int8_t id)
{
	return id >= plugin.modelIdBase && id < plugin.modelIdLimit();
}

void EmuSystem::onOptionsLoaded()
{
	currSystem = (ViceSystem)optionViceSystem.val;
	plugin = loadVicePlugin(currSystem, appContext().libPath().data());
	if(!plugin)
	{
		throw std::runtime_error(fmt::format("Error loading plugin for system {}", VicePlugin::systemName(currSystem)));
	}
	IG::formatTo(externalPaletteResStr, "{}ExternalPalette", videoChipStr());
	IG::formatTo(paletteFileResStr, "{}PaletteFile", videoChipStr());
	optionDefaultModel = {CFGKEY_DEFAULT_MODEL, plugin.defaultModelId, false, modelIdIsValid};
	optionModel = {CFGKEY_MODEL, -1, false, modelIdIsValid};
}

void applySessionOptions()
{
	if((int)optionModel == -1)
	{
		logMsg("using default model");
		setSysModel(optionDefaultModel);
	}
	else
	{
		setSysModel(optionModel);
	}
	setDriveTrueEmulation(optionDriveTrueEmulation);
	setAutostartWarp(optionAutostartWarp);
	setAutostartTDE(optionAutostartTDE);
	setAutostartBasicLoad(optionAutostartBasicLoad);
	if(currSystem == VICE_SYSTEM_VIC20)
	{
		uint8_t blocks = optionVic20RamExpansions;
		if(blocks & BLOCK_0)
			setIntResource("RamBlock0", 1);
		if(blocks & BLOCK_1)
			setIntResource("RamBlock1", 1);
		if(blocks & BLOCK_2)
			setIntResource("RamBlock2", 1);
		if(blocks & BLOCK_3)
			setIntResource("RamBlock3", 1);
		if(blocks & BLOCK_5)
			setIntResource("RamBlock5", 1);
	}
	if(currSystemIsC64Or128())
	{
		if(optionC64RamExpansionModule)
		{
			setIntResource("REU", 1);
			setIntResource("REUsize", optionC64RamExpansionModule);
		}
	}
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &)
{
	applySessionOptions();
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	optionModel.reset();
	optionDriveTrueEmulation.reset();
	optionAutostartWarp.reset();
	optionAutostartTDE.reset();
	optionAutostartBasicLoad.reset();
	optionSwapJoystickPorts.reset();
	optionAutostartOnLaunch.reset();
	optionVic20RamExpansions.reset();
	optionC64RamExpansionModule.reset();
	setPaletteResources(defaultPaletteName.c_str());
	// default drive setup
	setIntResource("Drive8Type", defaultIntResource("Drive8Type"));
	setIntResource("Drive9Type", DRIVE_TYPE_NONE);
	setIntResource("Drive10Type", DRIVE_TYPE_NONE);
	setIntResource("Drive11Type", DRIVE_TYPE_NONE);
	onSessionOptionsLoaded(app);
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_MODEL:
			optionModel.readFromIO(io, readSize);
		bcase CFGKEY_DRIVE_TRUE_EMULATION: optionDriveTrueEmulation.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_WARP: optionAutostartWarp.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_TDE: optionAutostartTDE.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_BASIC_LOAD: optionAutostartBasicLoad.readFromIO(io, readSize);
		bcase CFGKEY_SWAP_JOYSTICK_PORTS: optionSwapJoystickPorts.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_ON_LOAD: optionAutostartOnLaunch.readFromIO(io, readSize);
		bcase CFGKEY_VIC20_RAM_EXPANSIONS: optionVic20RamExpansions.readFromIO(io, readSize);
		bcase CFGKEY_C64_RAM_EXPANSION_MODULE: optionC64RamExpansionModule.readFromIO(io, readSize);
		bcase CFGKEY_PALETTE_NAME:
			readStringOptionValue<FS::FileString>(io, readSize, [](auto &name)
			{
				if(name == "internal")
					setPaletteResources({});
				else
					setPaletteResources(name.data());
			});
		bcase CFGKEY_DRIVE8_TYPE:
			readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive8Type", type); });
		bcase CFGKEY_DRIVE9_TYPE:
			readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive9Type", type); });
		bcase CFGKEY_DRIVE10_TYPE:
			readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive10Type", type); });
		bcase CFGKEY_DRIVE11_TYPE:
			readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive11Type", type); });
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionModel.writeWithKeyIfNotDefault(io);
	optionDriveTrueEmulation.writeWithKeyIfNotDefault(io);
	optionAutostartWarp.writeWithKeyIfNotDefault(io);
	optionAutostartTDE.writeWithKeyIfNotDefault(io);
	optionAutostartBasicLoad.writeWithKeyIfNotDefault(io);
	optionSwapJoystickPorts.writeWithKeyIfNotDefault(io);
	optionAutostartOnLaunch.writeWithKeyIfNotDefault(io);
	if(currSystem == VICE_SYSTEM_VIC20) // save RAM expansion settings
	{
		uint8_t blocks = (intResource("RamBlock0") ? BLOCK_0 : 0);
		if((int)optionModel != VIC20MODEL_VIC21)
		{
			blocks |= (intResource("RamBlock1") ? BLOCK_1 : 0);
			blocks |= (intResource("RamBlock2") ? BLOCK_2 : 0);
		}
		blocks |= (intResource("RamBlock3") ? BLOCK_3 : 0);
		blocks |= (intResource("RamBlock5") ? BLOCK_5 : 0);
		optionVic20RamExpansions = blocks;
		optionVic20RamExpansions.writeWithKeyIfNotDefault(io);
	}
	if(currSystemIsC64Or128())
	{
		if(intResource("REU"))
		{
			optionC64RamExpansionModule = intResource("REUsize");
			optionC64RamExpansionModule.writeWithKeyIfNotDefault(io);
		}
	}
	auto palName = paletteName();
	if(palName != defaultPaletteName)
	{
		if(!strlen(palName))
			palName = "internal";
		writeStringOptionValue(io, CFGKEY_PALETTE_NAME, palName);
	}
	if(auto driveType = intResource("Drive8Type");
		driveType != defaultIntResource("Drive8Type"))
	{
		writeOptionValue(io, CFGKEY_DRIVE8_TYPE, (uint16_t)driveType);
	}
	if(auto driveType = intResource("Drive9Type");
		driveType != DRIVE_TYPE_NONE)
	{
		writeOptionValue(io, CFGKEY_DRIVE9_TYPE, (uint16_t)driveType);
	}
	if(auto driveType = intResource("Drive10Type");
		driveType != DRIVE_TYPE_NONE)
	{
		writeOptionValue(io, CFGKEY_DRIVE10_TYPE, (uint16_t)driveType);
	}
	if(auto driveType = intResource("Drive11Type");
		driveType != DRIVE_TYPE_NONE)
	{
		writeOptionValue(io, CFGKEY_DRIVE11_TYPE, (uint16_t)driveType);
	}
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_VICE_SYSTEM: optionViceSystem.readFromIO(io, readSize);
		bcase CFGKEY_BORDER_MODE: optionBorderMode.readFromIO(io, readSize);
		bcase CFGKEY_CROP_NORMAL_BORDERS: optionCropNormalBorders.readFromIO(io, readSize);
		bcase CFGKEY_SID_ENGINE: optionSidEngine.readFromIO(io, readSize);
		bcase CFGKEY_SYSTEM_FILE_PATH:
			readStringOptionValue<FS::PathString>(io, readSize, [&](auto &path){setFirmwarePath(path);});
		bcase CFGKEY_RESID_SAMPLING: optionReSidSampling.readFromIO(io, readSize);
	}
	return 1;
}

bool EmuSystem::readCoreConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return false;
		bcase CFGKEY_DEFAULT_MODEL: optionDefaultModel.readFromIO(io, readSize);
		bcase CFGKEY_DEFAULT_PALETTE_NAME:
			readStringOptionValue<FS::FileString>(io, readSize, [&](auto &name){defaultPaletteName = name;});
	}
	return true;
}

void EmuSystem::writeConfig(IO &io)
{
	optionViceSystem.writeWithKeyIfNotDefault(io);
	optionBorderMode.writeWithKeyIfNotDefault(io);
	optionCropNormalBorders.writeWithKeyIfNotDefault(io);
	optionSidEngine.writeWithKeyIfNotDefault(io);
	optionReSidSampling.writeWithKeyIfNotDefault(io);
	writeStringOptionValue(io, CFGKEY_SYSTEM_FILE_PATH, firmwarePath());
}

void EmuSystem::writeCoreConfig(IO &io)
{
	optionDefaultModel.writeWithKeyIfNotDefault(io);
	writeStringOptionValue(io, CFGKEY_DEFAULT_PALETTE_NAME, defaultPaletteName);
}

void setDefaultModel(int model)
{
	optionDefaultModel = model;
}

}
