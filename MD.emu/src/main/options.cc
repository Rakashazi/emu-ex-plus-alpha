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
#include "internal.hh"

enum
{
	CFGKEY_BIG_ENDIAN_SRAM = 278, CFGKEY_SMS_FM = 279,
	CFGKEY_6_BTN_PAD = 280, CFGKEY_MD_CD_BIOS_USA_PATH = 281,
	CFGKEY_MD_CD_BIOS_JPN_PATH = 282, CFGKEY_MD_CD_BIOS_EUR_PATH = 283,
	CFGKEY_MD_REGION = 284, CFGKEY_VIDEO_SYSTEM = 285,
};

const char *EmuSystem::configFilename = "MdEmu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = IG::size(EmuSystem::aspectRatioInfo);
Byte1Option optionBigEndianSram{CFGKEY_BIG_ENDIAN_SRAM, 0};
Byte1Option optionSmsFM{CFGKEY_SMS_FM, 1};
Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};
Byte1Option optionRegion{CFGKEY_MD_REGION, 0};
#ifndef NO_SCD
FS::PathString cdBiosUSAPath{}, cdBiosJpnPath{}, cdBiosEurPath{};
PathOption optionCDBiosUsaPath{CFGKEY_MD_CD_BIOS_USA_PATH, cdBiosUSAPath, ""};
PathOption optionCDBiosJpnPath{CFGKEY_MD_CD_BIOS_JPN_PATH, cdBiosJpnPath, ""};
PathOption optionCDBiosEurPath{CFGKEY_MD_CD_BIOS_EUR_PATH, cdBiosEurPath, ""};
#endif
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0};

void EmuSystem::initOptions()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	optionTouchCtrlSize.initDefault(750);
	optionTouchCtrlBtnSpace.initDefault(100);
	#endif
}

void EmuSystem::onOptionsLoaded()
{
	#ifdef CONFIG_VCONTROLS_GAMEPAD
	vController.gp.activeFaceBtns = option6BtnPad ? 6 : 3;
	#endif
	config_ym2413_enabled = optionSmsFM;
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		bcase CFGKEY_BIG_ENDIAN_SRAM: optionBigEndianSram.readFromIO(io, readSize);
		bcase CFGKEY_SMS_FM: optionSmsFM.readFromIO(io, readSize);
		bcase CFGKEY_6_BTN_PAD: option6BtnPad.readFromIO(io, readSize);
		#ifndef NO_SCD
		bcase CFGKEY_MD_CD_BIOS_USA_PATH: optionCDBiosUsaPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_JPN_PATH: optionCDBiosJpnPath.readFromIO(io, readSize);
		bcase CFGKEY_MD_CD_BIOS_EUR_PATH: optionCDBiosEurPath.readFromIO(io, readSize);
		#endif
		bcase CFGKEY_MD_REGION:
		{
			optionRegion.readFromIO(io, readSize);
			if(optionRegion < 4)
			{
				config.region_detect = optionRegion;
			}
			else
				optionRegion = 0;
		}
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bdefault: return 0;
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionBigEndianSram.writeWithKeyIfNotDefault(io);
	optionSmsFM.writeWithKeyIfNotDefault(io);
	option6BtnPad.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	#ifndef NO_SCD
	optionCDBiosUsaPath.writeToIO(io);
	optionCDBiosJpnPath.writeToIO(io);
	optionCDBiosEurPath.writeToIO(io);
	#endif
	optionRegion.writeWithKeyIfNotDefault(io);
}
