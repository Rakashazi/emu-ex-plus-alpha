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

extern "C"
{
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
	#include "vicii.h"
	#include "sid/sid.h"
	#include "sid/sid-resources.h"
}

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
};

const char *EmuSystem::configFilename = "C64Emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[] =
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionDriveTrueEmulation(CFGKEY_DRIVE_TRUE_EMULATION, 0);
Byte1Option optionVirtualDeviceTraps(CFGKEY_VIRTUAL_DEVICE_TRAPS, 1);
Byte1Option optionCropNormalBorders(CFGKEY_CROP_NORMAL_BORDERS, 1);
Byte1Option optionAutostartWarp(CFGKEY_AUTOSTART_WARP, 1);
Byte1Option optionAutostartTDE(CFGKEY_AUTOSTART_TDE, 0);
Byte1Option optionAutostartBasicLoad(CFGKEY_AUTOSTART_BASIC_LOAD, 0);
Byte1Option optionViceSystem(CFGKEY_VICE_SYSTEM, VICE_SYSTEM_C64, false,
	optionIsValidWithMax<VicePlugin::SYSTEMS-1, uint8_t>);
SByte1Option optionModel(CFGKEY_MODEL, -1, false,
	optionIsValidWithMinMax<-1, 50, int8_t>);
Byte1Option optionC64Model(CFGKEY_C64_MODEL, C64MODEL_C64_NTSC, false,
	optionIsValidWithMax<C64MODEL_NUM-1, uint8_t>);
Byte1Option optionDTVModel(CFGKEY_DTV_MODEL, DTVMODEL_V3_NTSC, false,
	optionIsValidWithMax<DTVMODEL_NUM-1, uint8_t>);
Byte1Option optionC128Model(CFGKEY_C128_MODEL, C128MODEL_C128_NTSC, false,
	optionIsValidWithMax<C128MODEL_NUM-1, uint8_t>);
Byte1Option optionSuperCPUModel(CFGKEY_SUPER_CPU_MODEL, C64MODEL_C64_NTSC, false,
	optionIsValidWithMax<std::size(superCPUModelStr)-1, uint8_t>);
Byte1Option optionCBM2Model(CFGKEY_CBM2_MODEL, CBM2MODEL_610_NTSC, false,
	optionIsValidWithMinMax<CBM2MODEL_610_PAL, CBM2MODEL_720PLUS_NTSC, uint8_t>);
Byte1Option optionCBM5x0Model(CFGKEY_CBM5x0_MODEL, CBM2MODEL_510_NTSC, false,
	optionIsValidWithMinMax<CBM2MODEL_510_PAL, CBM2MODEL_510_NTSC, uint8_t>);
Byte1Option optionPETModel(CFGKEY_PET_MODEL, PETMODEL_8032, false,
	optionIsValidWithMax<PETMODEL_NUM-1, uint8_t>);
Byte1Option optionPlus4Model(CFGKEY_PLUS4_MODEL, PLUS4MODEL_PLUS4_NTSC, false,
	optionIsValidWithMax<PLUS4MODEL_NUM-1, uint8_t>);
Byte1Option optionVIC20Model(CFGKEY_VIC20_MODEL, VIC20MODEL_VIC20_NTSC, false,
	optionIsValidWithMax<VIC20MODEL_NUM-1, uint8_t>);
Byte1Option optionBorderMode(CFGKEY_BORDER_MODE, VICII_NORMAL_BORDERS);
Byte1Option optionSidEngine(CFGKEY_SID_ENGINE, SID_ENGINE_RESID, false,
	optionIsValidWithMax<1, uint8_t>);
Byte1Option optionReSidSampling(CFGKEY_RESID_SAMPLING, SID_RESID_SAMPLING_INTERPOLATION, false,
	optionIsValidWithMax<3, uint8_t>);
Byte1Option optionSwapJoystickPorts(CFGKEY_SWAP_JOYSTICK_PORTS, 0);
PathOption optionFirmwarePath(CFGKEY_SYSTEM_FILE_PATH, firmwareBasePath, "");
Byte1Option optionAutostartOnLaunch(CFGKEY_AUTOSTART_ON_LOAD, 1);

// VIC-20 specific
Byte1Option optionVic20RamExpansions(CFGKEY_VIC20_RAM_EXPANSIONS, 0);

EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext app)
{
	currSystem = (ViceSystem)optionViceSystem.val;
	plugin = loadVicePlugin(currSystem, EmuApp::libPath(app).data());
	if(!plugin)
	{
		return makeError("Error loading plugin for system %s", VicePlugin::systemName(currSystem));
	}
	return {};
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &)
{
	if(optionModel >= plugin.models)
	{
		optionModel.reset();
	}
	applySessionOptions();
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	optionModel.reset();
	optionDriveTrueEmulation.reset();
	optionVirtualDeviceTraps.reset();
	optionAutostartWarp.reset();
	optionAutostartTDE.reset();
	optionAutostartBasicLoad.reset();
	optionSwapJoystickPorts.reset();
	optionAutostartOnLaunch.reset();
	optionVic20RamExpansions.reset();
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
			if(optionModel >= plugin.models)
			{
				optionModel.reset();
			}
		bcase CFGKEY_DRIVE_TRUE_EMULATION: optionDriveTrueEmulation.readFromIO(io, readSize);
		bcase CFGKEY_VIRTUAL_DEVICE_TRAPS: optionVirtualDeviceTraps.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_WARP: optionAutostartWarp.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_TDE: optionAutostartTDE.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_BASIC_LOAD: optionAutostartBasicLoad.readFromIO(io, readSize);
		bcase CFGKEY_SWAP_JOYSTICK_PORTS: optionSwapJoystickPorts.readFromIO(io, readSize);
		bcase CFGKEY_AUTOSTART_ON_LOAD: optionAutostartOnLaunch.readFromIO(io, readSize);
		bcase CFGKEY_VIC20_RAM_EXPANSIONS: optionVic20RamExpansions.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionModel.writeWithKeyIfNotDefault(io);
	optionDriveTrueEmulation.writeWithKeyIfNotDefault(io);
	optionVirtualDeviceTraps.writeWithKeyIfNotDefault(io);
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
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_VICE_SYSTEM: optionViceSystem.readFromIO(io, readSize);
		bcase CFGKEY_C64_MODEL: optionC64Model.readFromIO(io, readSize);
		bcase CFGKEY_DTV_MODEL: optionDTVModel.readFromIO(io, readSize);
		bcase CFGKEY_C128_MODEL: optionC128Model.readFromIO(io, readSize);
		bcase CFGKEY_SUPER_CPU_MODEL: optionSuperCPUModel.readFromIO(io, readSize);
		bcase CFGKEY_CBM2_MODEL: optionCBM2Model.readFromIO(io, readSize);
		bcase CFGKEY_CBM5x0_MODEL: optionCBM5x0Model.readFromIO(io, readSize);
		bcase CFGKEY_PET_MODEL: optionPETModel.readFromIO(io, readSize);
		bcase CFGKEY_PLUS4_MODEL: optionPlus4Model.readFromIO(io, readSize);
		bcase CFGKEY_VIC20_MODEL: optionVIC20Model.readFromIO(io, readSize);
		bcase CFGKEY_BORDER_MODE: optionBorderMode.readFromIO(io, readSize);
		bcase CFGKEY_CROP_NORMAL_BORDERS: optionCropNormalBorders.readFromIO(io, readSize);
		bcase CFGKEY_SID_ENGINE: optionSidEngine.readFromIO(io, readSize);
		bcase CFGKEY_SYSTEM_FILE_PATH: optionFirmwarePath.readFromIO(io, readSize);
		bcase CFGKEY_RESID_SAMPLING: optionReSidSampling.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionViceSystem.writeWithKeyIfNotDefault(io);
	optionC64Model.writeWithKeyIfNotDefault(io);
	optionDTVModel.writeWithKeyIfNotDefault(io);
	optionC128Model.writeWithKeyIfNotDefault(io);
	optionSuperCPUModel.writeWithKeyIfNotDefault(io);
	optionCBM2Model.writeWithKeyIfNotDefault(io);
	optionCBM5x0Model.writeWithKeyIfNotDefault(io);
	optionPETModel.writeWithKeyIfNotDefault(io);
	optionPlus4Model.writeWithKeyIfNotDefault(io);
	optionVIC20Model.writeWithKeyIfNotDefault(io);
	optionBorderMode.writeWithKeyIfNotDefault(io);
	optionCropNormalBorders.writeWithKeyIfNotDefault(io);
	optionSidEngine.writeWithKeyIfNotDefault(io);
	optionReSidSampling.writeWithKeyIfNotDefault(io);
	optionFirmwarePath.writeToIO(io);
}

int optionDefaultModel(ViceSystem system)
{
	switch(system)
	{
		case VICE_SYSTEM_C64: return optionC64Model;
		case VICE_SYSTEM_C64SC: return optionC64Model;
		case VICE_SYSTEM_C64DTV: return optionDTVModel;
		case VICE_SYSTEM_C128: return optionC128Model;
		case VICE_SYSTEM_SUPER_CPU: return optionSuperCPUModel;
		case VICE_SYSTEM_CBM2: return optionCBM2Model;
		case VICE_SYSTEM_CBM5X0: return optionCBM5x0Model;
		case VICE_SYSTEM_PET: return optionPETModel;
		case VICE_SYSTEM_PLUS4: return optionPlus4Model;
		case VICE_SYSTEM_VIC20: return optionVIC20Model;
	}
	return 0;
}
