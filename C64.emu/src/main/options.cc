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
#include <emuframework/Option.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

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

constexpr SystemLogger log{"C64.emu"};
const char *EmuSystem::configFilename = "C64Emu.config";

std::span<const AspectRatioInfo> C64System::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void C64System::setPaletteResources(const char *palName)
{
	if(palName && strlen(palName))
	{
		log.info("setting external palette:{}", palName);
		setStringResource(paletteFileResStr.data(), palName);
		setIntResource(externalPaletteResStr.data(), 1);
	}
	else
	{
		log.info("setting internal palette");
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
	currSystem = optionViceSystem;
	plugin = loadVicePlugin(currSystem, appContext().libPath().data());
	if(!plugin)
	{
		throw std::runtime_error(std::format("Error loading plugin for system {}: {}",
			VicePlugin::systemName(currSystem), lastOpenSharedLibraryError()));
	}
	plugin.init();
	auto vcStr = videoChipStr();
	externalPaletteResStr = formatArray<sizeof(externalPaletteResStr)>("{}ExternalPalette", vcStr);
	paletteFileResStr = formatArray<sizeof(paletteFileResStr)>("{}PaletteFile", vcStr);
	colorSettingResStr[0] = std::format("{}ColorSaturation", vcStr);
	colorSettingResStr[1] = std::format("{}ColorContrast", vcStr);
	colorSettingResStr[2] = std::format("{}ColorBrightness", vcStr);
	colorSettingResStr[3] = std::format("{}ColorGamma", vcStr);
	colorSettingResStr[4] = std::format("{}ColorTint", vcStr);
	defaultModel = plugin.defaultModelId;
	auto prgDiskPath = FS::pathString(fallbackSaveDirectory(), "AutostartPrgDisk.d64");
	FS::remove(prgDiskPath);
	setStringResource("AutostartPrgDiskImage", prgDiskPath.data());
	setReSidSampling(defaultReSidSampling);
}

void C64System::onSessionOptionsLoaded(EmuApp &)
{
	setJoystickMode(joystickMode);
}

bool C64System::resetSessionOptions(EmuApp &)
{
	initC64(EmuApp::get(appContext()));
	enterCPUTrap();
	setModel(defaultModel);
	setJoystickMode(JoystickMode::Auto);
	setAutostartWarp(true);
	setAutostartTDE(false);
	resetIntResource("AutostartBasicLoad");
	optionAutostartOnLaunch = true;
	if(currSystem == ViceSystem::VIC20)
	{
		setIntResource("RamBlock0", 0);
		setIntResource("RamBlock1", 0);
		setIntResource("RamBlock2", 0);
		setIntResource("RamBlock3", 0);
		setIntResource("RamBlock5", 0);
	}
	if(currSystemIsC64Or128())
	{
		setIntResource("REU", 0);
	}
	setPaletteResources(defaultPaletteName.c_str());
	// default drive setup
	setDriveType(8, defaultIntResource("Drive8Type"));
	setDriveType(9, DRIVE_TYPE_NONE);
	setDriveType(10, DRIVE_TYPE_NONE);
	setDriveType(11, DRIVE_TYPE_NONE);
	setDriveTrueEmulation(defaultDriveTrueEmulation);
	return true;
}

bool C64System::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_VICE_SYSTEM: return readOptionValue(io, optionViceSystem, [](auto v){return int(v) < VicePlugin::SYSTEMS;});
			case CFGKEY_SYSTEM_FILE_PATH:
				return readStringOptionValue<FS::PathString>(io, [&](auto &&path){sysFilePath[0] = IG_forward(path);});
		}
	}
	else if(type == ConfigType::CORE)
	{
		switch(key)
		{
			case CFGKEY_DEFAULT_MODEL: return readOptionValue(io, defaultModel, modelIdIsValid);
			case CFGKEY_CROP_NORMAL_BORDERS: return readOptionValue(io, optionCropNormalBorders);
			case CFGKEY_DEFAULT_DRIVE_TRUE_EMULATION: return readOptionValue(io, defaultDriveTrueEmulation);
			case CFGKEY_SID_ENGINE: return readOptionValue<uint8_t>(io, [&](auto v){ setSidEngine(v); });
			case CFGKEY_BORDER_MODE: return readOptionValue<uint8_t>(io, [&](auto v){ setBorderMode(v); });
			case CFGKEY_RESID_SAMPLING: return readOptionValue<uint8_t>(io, [&](auto v){ setReSidSampling(v); });
			case CFGKEY_DEFAULT_PALETTE_NAME: return readStringOptionValue(io, defaultPaletteName);
			case CFGKEY_COLOR_SATURATION: return readOptionValue<int16_t>(io, [&](auto v){ setColorSetting(ColorSetting::Saturation, v); });
			case CFGKEY_COLOR_CONTRAST: return readOptionValue<int16_t>(io, [&](auto v){ setColorSetting(ColorSetting::Contrast, v); });
			case CFGKEY_COLOR_BRIGHTNESS: return readOptionValue<int16_t>(io, [&](auto v){ setColorSetting(ColorSetting::Brightness, v); });
			case CFGKEY_COLOR_GAMMA: return readOptionValue<int16_t>(io, [&](auto v){ setColorSetting(ColorSetting::Gamma, v); });
			case CFGKEY_COLOR_TINT: return readOptionValue<int16_t>(io, [&](auto v){ setColorSetting(ColorSetting::Tint, v); });
			case CFGKEY_DEFAULT_JOYSTICK_MODE: return readOptionValue(io, defaultJoystickMode);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_MODEL:
				return readOptionValue<bool>(io, [&](auto v){ if(modelIdIsValid(v)) { setModel(v); } });
			case CFGKEY_DRIVE_TRUE_EMULATION: return readOptionValue<bool>(io, [&](auto v){ setDriveTrueEmulation(v); });
			case CFGKEY_AUTOSTART_WARP: return readOptionValue<bool>(io, [&](auto v){ setAutostartWarp(v); });
			case CFGKEY_AUTOSTART_TDE: return readOptionValue<bool>(io, [&](auto v){ setAutostartTDE(v); });
			case CFGKEY_AUTOSTART_BASIC_LOAD: return readOptionValue<bool>(io, [&](auto v){ setIntResource("AutostartBasicLoad", v); });
			case CFGKEY_JOYSTICK_MODE: return readOptionValue(io, joystickMode);
			case CFGKEY_AUTOSTART_ON_LOAD: return readOptionValue(io, optionAutostartOnLaunch);
			case CFGKEY_VIC20_RAM_EXPANSIONS: return readOptionValue<uint8_t>(io, [&](auto blocks)
			{
				enterCPUTrap();
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
			});
			case CFGKEY_C64_RAM_EXPANSION_MODULE: return readOptionValue<int16_t>(io, [&](auto reuSize)
			{
				setReuSize(reuSize);
			});
			case CFGKEY_PALETTE_NAME: return readStringOptionValue<FS::FileString>(io, [&](auto &&name)
			{
				if(name == "internal")
					setPaletteResources({});
				else
					setPaletteResources(name.data());
			});
			case CFGKEY_DRIVE8_TYPE:
				return readOptionValue<uint16_t>(io, [&](auto type){ setDriveType(8, type); });
			case CFGKEY_DRIVE9_TYPE:
				return readOptionValue<uint16_t>(io, [&](auto type){ setDriveType(9, type); });
			case CFGKEY_DRIVE10_TYPE:
				return readOptionValue<uint16_t>(io, [&](auto type){ setDriveType(10, type); });
			case CFGKEY_DRIVE11_TYPE:
				return readOptionValue<uint16_t>(io, [&](auto type){ setDriveType(11, type); });
		}
	}
	return false;
}

void C64System::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_VICE_SYSTEM, optionViceSystem, ViceSystem::C64);
		writeStringOptionValue(io, CFGKEY_SYSTEM_FILE_PATH, sysFilePath[0]);
	}
	else if(type == ConfigType::CORE)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_DEFAULT_MODEL, defaultModel, plugin.defaultModelId);
		writeOptionValueIfNotDefault(io, CFGKEY_DEFAULT_DRIVE_TRUE_EMULATION, defaultDriveTrueEmulation, true);
		writeOptionValueIfNotDefault(io, CFGKEY_BORDER_MODE, uint8_t(borderMode()), VICII_NORMAL_BORDERS);
		writeOptionValueIfNotDefault(io, CFGKEY_CROP_NORMAL_BORDERS, optionCropNormalBorders, true);
		writeOptionValueIfNotDefault(io, CFGKEY_SID_ENGINE, uint8_t(sidEngine()), SID_ENGINE_RESID);
		writeOptionValueIfNotDefault(io, CFGKEY_RESID_SAMPLING, uint8_t(reSidSampling()), defaultReSidSampling);
		writeStringOptionValue(io, CFGKEY_DEFAULT_PALETTE_NAME, defaultPaletteName);
		writeOptionValueIfNotDefault(io, CFGKEY_COLOR_SATURATION, int16_t(colorSetting(ColorSetting::Saturation)), 1250);
		writeOptionValueIfNotDefault(io, CFGKEY_COLOR_CONTRAST, int16_t(colorSetting(ColorSetting::Contrast)), 1250);
		writeOptionValueIfNotDefault(io, CFGKEY_COLOR_BRIGHTNESS, int16_t(colorSetting(ColorSetting::Brightness)), 1000);
		writeOptionValueIfNotDefault(io, CFGKEY_COLOR_GAMMA, int16_t(colorSetting(ColorSetting::Gamma)), 1000);
		writeOptionValueIfNotDefault(io, CFGKEY_COLOR_TINT, int16_t(colorSetting(ColorSetting::Tint)), 1000);
		writeOptionValueIfNotDefault(io, defaultJoystickMode);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, CFGKEY_MODEL, model(), defaultModel);
		writeOptionValueIfNotDefault(io, CFGKEY_AUTOSTART_WARP, autostartWarp(), true);
		writeOptionValueIfNotDefault(io, CFGKEY_AUTOSTART_TDE, autostartTDE(), false);
		writeOptionValueIfNotDefault(io, CFGKEY_AUTOSTART_BASIC_LOAD, intResource("AutostartBasicLoad"), defaultIntResource("AutostartBasicLoad"));
		writeOptionValueIfNotDefault(io, joystickMode);
		writeOptionValueIfNotDefault(io, CFGKEY_AUTOSTART_ON_LOAD, optionAutostartOnLaunch, true);
		if(currSystem == ViceSystem::VIC20) // save RAM expansion settings
		{
			uint8_t blocks = (intResource("RamBlock0") ? BLOCK_0 : 0);
			if(model() != VIC20MODEL_VIC21)
			{
				blocks |= (intResource("RamBlock1") ? BLOCK_1 : 0);
				blocks |= (intResource("RamBlock2") ? BLOCK_2 : 0);
			}
			blocks |= (intResource("RamBlock3") ? BLOCK_3 : 0);
			blocks |= (intResource("RamBlock5") ? BLOCK_5 : 0);
			writeOptionValueIfNotDefault(io, CFGKEY_VIC20_RAM_EXPANSIONS, blocks, 0);
		}
		if(currSystemIsC64Or128() && intResource("REU"))
		{
			writeOptionValueIfNotDefault(io, CFGKEY_C64_RAM_EXPANSION_MODULE, int16_t(intResource("REUsize")), 0);
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
		writeOptionValueIfNotDefault(io, CFGKEY_DRIVE_TRUE_EMULATION, driveTrueEmulation(), defaultDriveTrueEmulation);
	}
}

}
