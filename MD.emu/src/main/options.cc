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
#include "MainSystem.hh"

namespace EmuEx
{

const char *EmuSystem::configFilename = "MdEmu.config";

std::span<const AspectRatioInfo> MdSystem::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
		{"4:3 (Original)", {4, 3}},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

void MdSystem::onOptionsLoaded()
{
	config_ym2413_enabled = optionSmsFM;
}

void MdSystem::onSessionOptionsLoaded(EmuApp &app)
{
	config.region_detect = optionRegion;
	mdInputPortDev[0] = optionInputPort1;
	mdInputPortDev[1] = optionInputPort2;
	setupInput(app);
}

bool MdSystem::resetSessionOptions(EmuApp &app)
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

bool MdSystem::readConfig(ConfigType type, MapIO &io, unsigned key, size_t readSize)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_BIG_ENDIAN_SRAM: return optionBigEndianSram.readFromIO(io, readSize);
			case CFGKEY_SMS_FM: return optionSmsFM.readFromIO(io, readSize);
			#ifndef NO_SCD
			case CFGKEY_MD_CD_BIOS_USA_PATH: return readStringOptionValue(io, readSize, cdBiosUSAPath);
			case CFGKEY_MD_CD_BIOS_JPN_PATH: return readStringOptionValue(io, readSize, cdBiosJpnPath);
			case CFGKEY_MD_CD_BIOS_EUR_PATH: return readStringOptionValue(io, readSize, cdBiosEurPath);
			#endif
			case CFGKEY_CHEATS_PATH: return readStringOptionValue(io, readSize, cheatsDir);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_6_BTN_PAD: return option6BtnPad.readFromIO(io, readSize);
			case CFGKEY_MD_REGION: return optionRegion.readFromIO(io, readSize);
			case CFGKEY_VIDEO_SYSTEM: return optionVideoSystem.readFromIO(io, readSize);
			case CFGKEY_INPUT_PORT_1: return optionInputPort1.readFromIO(io, readSize);
			case CFGKEY_INPUT_PORT_2: return optionInputPort2.readFromIO(io, readSize);
			case CFGKEY_MULTITAP: return optionMultiTap.readFromIO(io, readSize);
		}
	}
	return false;
}

void MdSystem::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		optionBigEndianSram.writeWithKeyIfNotDefault(io);
		optionSmsFM.writeWithKeyIfNotDefault(io);
		#ifndef NO_SCD
		writeStringOptionValue(io, CFGKEY_MD_CD_BIOS_USA_PATH, cdBiosUSAPath);
		writeStringOptionValue(io, CFGKEY_MD_CD_BIOS_JPN_PATH, cdBiosJpnPath);
		writeStringOptionValue(io, CFGKEY_MD_CD_BIOS_EUR_PATH, cdBiosEurPath);
		#endif
		writeStringOptionValue(io, CFGKEY_CHEATS_PATH, cheatsDir);
	}
	else if(type == ConfigType::SESSION)
	{
		option6BtnPad.writeWithKeyIfNotDefault(io);
		optionRegion.writeWithKeyIfNotDefault(io);
		optionVideoSystem.writeWithKeyIfNotDefault(io);
		optionInputPort1.writeWithKeyIfNotDefault(io);
		optionInputPort2.writeWithKeyIfNotDefault(io);
		optionMultiTap.writeWithKeyIfNotDefault(io);
	}
}

}
