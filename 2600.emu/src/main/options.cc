/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#undef Debugger
#include "MainSystem.hh"
#include <emuframework/Option.hh>

namespace EmuEx
{

constexpr SystemLogger log{"2600.emu"};
const char *EmuSystem::configFilename = "2600emu.config";

std::span<const AspectRatioInfo> A2600System::aspectRatioInfos()
{
	static constexpr AspectRatioInfo aspectRatioInfo[]
	{
			{"4:3 (Original)", {4, 3}},
			EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
	};
	return aspectRatioInfo;
}

bool A2600System::resetSessionOptions(EmuApp &app)
{
	optionTVPhosphor.reset();
	setRuntimeTVPhosphor(optionTVPhosphor, optionTVPhosphorBlend);
	optionVideoSystem.reset();
	optionInputPort1.reset();
	optionPaddleDigitalSensitivity.reset();
	optionPaddleAnalogRegion.reset();
	if(osystem.hasConsole())
	{
		setControllerType(app, osystem.console(), optionInputPort1);
	}
	return true;
}

bool A2600System::readConfig(ConfigType type, MapIO &io, unsigned key)
{
	if(type == ConfigType::MAIN)
	{
		switch(key)
		{
			case CFGKEY_2600_TV_PHOSPHOR_BLEND: return readOptionValue(io, optionTVPhosphorBlend);
			case CFGKEY_AUDIO_RESAMPLE_QUALITY: return readOptionValue(io, optionAudioResampleQuality);
		}
	}
	else if(type == ConfigType::SESSION)
	{
		switch(key)
		{
			case CFGKEY_2600_TV_PHOSPHOR: return readOptionValue(io, optionTVPhosphor);
			case CFGKEY_VIDEO_SYSTEM: return readOptionValue(io, optionVideoSystem);
			case CFGKEY_INPUT_PORT_1: return readOptionValue(io, optionInputPort1);
			case CFGKEY_PADDLE_DIGITAL_SENSITIVITY: return readOptionValue(io, optionPaddleDigitalSensitivity);
			case CFGKEY_PADDLE_ANALOG_REGION: return readOptionValue(io, optionPaddleAnalogRegion);
		}
	}
	return false;
}

void A2600System::writeConfig(ConfigType type, FileIO &io)
{
	if(type == ConfigType::MAIN)
	{
		writeOptionValueIfNotDefault(io, optionTVPhosphorBlend);
		writeOptionValueIfNotDefault(io, optionAudioResampleQuality);
	}
	else if(type == ConfigType::SESSION)
	{
		writeOptionValueIfNotDefault(io, optionTVPhosphor);
		writeOptionValueIfNotDefault(io, optionVideoSystem);
		writeOptionValueIfNotDefault(io, optionInputPort1);
		writeOptionValueIfNotDefault(io, optionPaddleDigitalSensitivity);
		writeOptionValueIfNotDefault(io, optionPaddleAnalogRegion);
	}
}

const char *optionVideoSystemToStr(uint8_t sysIdx)
{
	switch(sysIdx)
	{
		case 1: return "NTSC";
		case 2: return "PAL";
		case 3: return "SECAM";
		case 4: return "NTSC50";
		case 5: return "PAL60";
		case 6: return "SECAM60";
		default: return "AUTO";
	}
}

void A2600System::setRuntimeTVPhosphor(int val, int blend)
{
	if(!hasContent() || !osystem.hasConsole())
	{
		return;
	}
	// change runtime phosphor value
	bool usePhosphor = false;
	if(val == TV_PHOSPHOR_AUTO)
	{
		usePhosphor = defaultGameProps.get(PropType::Display_Phosphor) == "YES";
	}
	else
	{
		usePhosphor = val;
	}
	log.info("Phosphor effect:{}", usePhosphor ? "on" : "off");
	auto props = osystem.console().properties();
	if(usePhosphor)
	{
		props.set(PropType::Display_Phosphor, "Yes");
	}
	else
	{
		props.set(PropType::Display_Phosphor, "No");
	}
	osystem.console().setProperties(props);
	osystem.frameBuffer().tiaSurface().enablePhosphor(usePhosphor, blend);
}

}
