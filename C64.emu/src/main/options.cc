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
#include "MainSystem.hh"
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
}

namespace EmuEx
{

const char *EmuSystem::configFilename = "C64Emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);

void C64System::setPaletteResources(const char *palName)
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

bool C64System::usingExternalPalette() const
{
	return intResource(externalPaletteResStr.data());
}

const char *C64System::externalPaletteName() const
{
	return stringResource(paletteFileResStr.data());
}

const char *C64System::paletteName() const
{
	if(usingExternalPalette())
		return externalPaletteName();
	else
		return "";
}

FS::FileString C64System::configName() const
{
	return FS::FileString{plugin.configName};
}

static bool modelIdIsValid(int8_t id)
{
	auto &plugin = gC64System().plugin;
	return id >= plugin.modelIdBase && id < plugin.modelIdLimit();
}

void C64System::onOptionsLoaded()
{
	currSystem = (ViceSystem)optionViceSystem.val;
	plugin = loadVicePlugin(currSystem, appContext().libPath().data());
	if(!plugin)
	{
		throw std::runtime_error(fmt::format("Error loading plugin for system {}", VicePlugin::systemName(currSystem)));
	}
	IG::formatTo(externalPaletteResStr, "{}ExternalPalette", videoChipStr());
	IG::formatTo(paletteFileResStr, "{}PaletteFile", videoChipStr());
	optionDefaultModel = SByte1Option{CFGKEY_DEFAULT_MODEL, plugin.defaultModelId, false, modelIdIsValid};
	optionModel = SByte1Option{CFGKEY_MODEL, -1, false, modelIdIsValid};
}

void C64System::applySessionOptions()
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
	if(currSystem == ViceSystem::VIC20)
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

void C64System::onSessionOptionsLoaded(EmuApp &)
{
	applySessionOptions();
}

bool C64System::resetSessionOptions(EmuApp &app)
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

bool C64System::readConfig(ConfigType type, IO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_VICE_SYSTEM: return optionViceSystem.readFromIO(io, readSize);
			case CFGKEY_BORDER_MODE: return optionBorderMode.readFromIO(io, readSize);
			case CFGKEY_CROP_NORMAL_BORDERS: return optionCropNormalBorders.readFromIO(io, readSize);
			case CFGKEY_SID_ENGINE: return optionSidEngine.readFromIO(io, readSize);
			case CFGKEY_SYSTEM_FILE_PATH:
				return readStringOptionValue<FS::PathString>(io, readSize, [&](auto &path){setFirmwarePath(path);});
			case CFGKEY_RESID_SAMPLING: return optionReSidSampling.readFromIO(io, readSize);
		}
	}
	else if(type == ConfigType::CORE)
	{
		switch(key)
		{
			case CFGKEY_DEFAULT_MODEL: return optionDefaultModel.readFromIO(io, readSize);
			case CFGKEY_DEFAULT_PALETTE_NAME:
				return readStringOptionValue<FS::FileString>(io, readSize, [&](auto &name){defaultPaletteName = name;});
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_MODEL:
				return optionModel.readFromIO(io, readSize);
			case CFGKEY_DRIVE_TRUE_EMULATION: return optionDriveTrueEmulation.readFromIO(io, readSize);
			case CFGKEY_AUTOSTART_WARP: return optionAutostartWarp.readFromIO(io, readSize);
			case CFGKEY_AUTOSTART_TDE: return optionAutostartTDE.readFromIO(io, readSize);
			case CFGKEY_AUTOSTART_BASIC_LOAD: return optionAutostartBasicLoad.readFromIO(io, readSize);
			case CFGKEY_SWAP_JOYSTICK_PORTS: return optionSwapJoystickPorts.readFromIO(io, readSize);
			case CFGKEY_AUTOSTART_ON_LOAD: return optionAutostartOnLaunch.readFromIO(io, readSize);
			case CFGKEY_VIC20_RAM_EXPANSIONS: return optionVic20RamExpansions.readFromIO(io, readSize);
			case CFGKEY_C64_RAM_EXPANSION_MODULE: return optionC64RamExpansionModule.readFromIO(io, readSize);
			case CFGKEY_PALETTE_NAME:
				return readStringOptionValue<FS::FileString>(io, readSize, [&](auto &name)
				{
					if(name == "internal")
						setPaletteResources({});
					else
						setPaletteResources(name.data());
				});
			case CFGKEY_DRIVE8_TYPE:
				return readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive8Type", type); });
			case CFGKEY_DRIVE9_TYPE:
				return readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive9Type", type); });
			case CFGKEY_DRIVE10_TYPE:
				return readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive10Type", type); });
			case CFGKEY_DRIVE11_TYPE:
				return readOptionValue<uint16_t>(io, readSize, [&](auto &type){ setIntResource("Drive11Type", type); });
		}
	}
	return false;
}

void C64System::writeConfig(ConfigType type, IO &io)
{
	if(type == ConfigType::MAIN)
	{
		optionViceSystem.writeWithKeyIfNotDefault(io);
		optionBorderMode.writeWithKeyIfNotDefault(io);
		optionCropNormalBorders.writeWithKeyIfNotDefault(io);
		optionSidEngine.writeWithKeyIfNotDefault(io);
		optionReSidSampling.writeWithKeyIfNotDefault(io);
		writeStringOptionValue(io, CFGKEY_SYSTEM_FILE_PATH, firmwarePath());
	}
	else if(type == ConfigType::CORE)
	{
		optionDefaultModel.writeWithKeyIfNotDefault(io);
		writeStringOptionValue(io, CFGKEY_DEFAULT_PALETTE_NAME, defaultPaletteName);
	}
	else if(type == ConfigType::SESSION)
	{
		optionModel.writeWithKeyIfNotDefault(io);
		optionDriveTrueEmulation.writeWithKeyIfNotDefault(io);
		optionAutostartWarp.writeWithKeyIfNotDefault(io);
		optionAutostartTDE.writeWithKeyIfNotDefault(io);
		optionAutostartBasicLoad.writeWithKeyIfNotDefault(io);
		optionSwapJoystickPorts.writeWithKeyIfNotDefault(io);
		optionAutostartOnLaunch.writeWithKeyIfNotDefault(io);
		if(currSystem == ViceSystem::VIC20) // save RAM expansion settings
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
}

void C64System::setDefaultModel(int model)
{
	optionDefaultModel = model;
}

}
