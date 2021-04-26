/*  This file is part of MD.emu.

	MD.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MD.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MD.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuInput.hh>
#include "internal.hh"

enum
{
	CFGKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_SMS_FM = 279,
	CFGKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283,
	CFGKEY_MD_REGION = 284, CFGKEY_VIDEO_SYSTEM = 285,
	CFGKEY_INPUT_PORT_1 = 286, CFGKEY_INPUT_PORT_2 = 287,
	CFGKEY_MULTITAP = 288
};

const char *EmuSystem::configFilename = "MdEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const unsigned EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionBigEndianSram{CFGKEY_BIG_ENDIAN_SRAM, 0};
Byte1Option optionSmsFM{CFGKEY_SMS_FM, 1};
Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
Byte1Option optionMultiTap{CFGKEY_MULTITAP, 0};
SByte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, -1, false, optionIsValidWithMinMax<-1, 4>};
SByte1Option optionInputPort2{CFGKEY_INPUT_PORT_2, -1, false, optionIsValidWithMinMax<-1, 4>};
Byte1Option optionRegion{CFGKEY_MD_REGION, 0, false, optionIsValidWithMax<4>};
#ifndef NO_SCD
FS::PathString cdBiosUSAPath{}, cdBiosJpnPath{}, cdBiosEurPath{};
PathOption optionCDBiosUsaPath{CFGKEY_MD_CD_BIOS_USA_PATH, cdBiosUSAPath, ""};
PathOption optionCDBiosJpnPath{CFGKEY_MD_CD_BIOS_JPN_PATH, cdBiosJpnPath, ""};
PathOption optionCDBiosEurPath{CFGKEY_MD_CD_BIOS_EUR_PATH, cdBiosEurPath, ""};
#endif
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<2>};

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonSpacing(100);
}

EmuSystem::Error EmuSystem::onOptionsLoaded(Base::ApplicationContext)
{
	config_ym2413_enabled = optionSmsFM;
	return {};
}

void EmuSystem::onSessionOptionsLoaded(EmuApp &app)
{
	config.region_detect = optionRegion;
	mdInputPortDev[0] = optionInputPort1;
	mdInputPortDev[1] = optionInputPort2;
	setupMDInput(app);
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	option6BtnPad.reset();
	optionRegion.reset();
	optionVideoSystem.reset();
	optionInputPort1.reset();
	optionInputPort2.reset();
	optionMultiTap.reset();
	onSessionOptionsLoaded(app);
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_6_BTN_PAD: option6BtnPad.readFromIO(io, readSize);
		bcase CFGKEY_MD_REGION: optionRegion.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_INPUT_PORT_1: optionInputPort1.readFromIO(io, readSize);
		bcase CFGKEY_INPUT_PORT_2: optionInputPort2.readFromIO(io, readSize);
		bcase CFGKEY_MULTITAP: optionMultiTap.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	option6BtnPad.writeWithKeyIfNotDefault(io);
	optionRegion.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	optionInputPort1.writeWithKeyIfNotDefault(io);
	optionInputPort2.writeWithKeyIfNotDefault(io);
	optionMultiTap.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, unsigned key, unsigned readSize)
{
	switch(key)
	{
		bcase CFGKEY_BIG_ENDIAN_SRAM: optionBigEndianSram.readFromIO(io, readSize);
		bcase CFGKEY_SMS_FM: optionSmsFM.readFromIO(io, readSize);
		#ifndef NO_SCD
		bcase CFGKEY_MD_CD_BIOS_USA_PATH: optionCDBiosUsaPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_JPN_PATH: optionCDBiosJpnPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_EUR_PATH: optionCDBiosEurPath.readFromIO(io, readSize);
		#endif
		bdefault: return 0;
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionBigEndianSram.writeWithKeyIfNotDefault(io);
	optionSmsFM.writeWithKeyIfNotDefault(io);
	#ifndef NO_SCD
	optionCDBiosUsaPath.writeToIO(io);
	optionCDBiosJpnPath.writeToIO(io);
	optionCDBiosEurPath.writeToIO(io);
	#endif

}
