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

#include <OSystem.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/EmuApp.hh>
#undef Debugger
#include "internal.hh"

static bool optionIsValidControllerType(uint8_t val);

enum
{
	CFGKEY_2600_TV_PHOSPHOR = 270, CFGKEY_VIDEO_SYSTEM = 271,
	CFGKEY_2600_TV_PHOSPHOR_BLEND = 272, CFGKEY_AUDIO_RESAMPLE_QUALITY = 273,
	CFGKEY_INPUT_PORT_1 = 274, CFGKEY_INPUT_PORT_2 = 275,
	CFGKEY_PADDLE_DIGITAL_SENSITIVITY = 276
};

const char *EmuSystem::configFilename = "2600emu.config";
const AspectRatioInfo EmuSystem::aspectRatioInfo[]
{
		{"4:3 (Original)", 4, 3},
		EMU_SYSTEM_DEFAULT_ASPECT_RATIO_INFO_INIT
};
const uint EmuSystem::aspectRatioInfos = std::size(EmuSystem::aspectRatioInfo);
Byte1Option optionTVPhosphor{CFGKEY_2600_TV_PHOSPHOR, TV_PHOSPHOR_AUTO, false, optionIsValidWithMax<2>};
Byte1Option optionTVPhosphorBlend{CFGKEY_2600_TV_PHOSPHOR_BLEND, 80, false, optionIsValidWithMax<100>};
Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<6>};
Byte1Option optionAudioResampleQuality{CFGKEY_AUDIO_RESAMPLE_QUALITY,
	(uint8_t)AudioSettings::DEFAULT_RESAMPLING_QUALITY, false,
	optionIsValidWithMinMax<(uint8_t)AudioSettings::ResamplingQuality::nearestNeightbour, (uint8_t)AudioSettings::ResamplingQuality::lanczos_3>};
Byte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, 0, false, optionIsValidControllerType};
Byte1Option optionPaddleDigitalSensitivity{CFGKEY_PADDLE_DIGITAL_SENSITIVITY, 1, false,
	optionIsValidWithMinMax<1, 20>};

static bool optionIsValidControllerType(uint8_t val)
{
	switch((Controller::Type)val)
	{
		case Controller::Type::Unknown:
		case Controller::Type::Joystick:
		case Controller::Type::Genesis:
			return true;
		default:
			return false;
	}
}

void EmuSystem::initOptions()
{
	EmuApp::setDefaultVControlsButtonStagger(5);
}

bool EmuSystem::resetSessionOptions(EmuApp &app)
{
	optionTVPhosphor.reset();
	setRuntimeTVPhosphor(optionTVPhosphor, optionTVPhosphorBlend);
	optionVideoSystem.reset();
	optionInputPort1.reset();
	optionPaddleDigitalSensitivity.reset();
	if(osystem->hasConsole())
	{
		setControllerType(app, osystem->console(), (Controller::Type)optionInputPort1.val);
	}
	return true;
}

bool EmuSystem::readSessionConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_2600_TV_PHOSPHOR: optionTVPhosphor.readFromIO(io, readSize);
		bcase CFGKEY_VIDEO_SYSTEM: optionVideoSystem.readFromIO(io, readSize);
		bcase CFGKEY_INPUT_PORT_1: optionInputPort1.readFromIO(io, readSize);
		bcase CFGKEY_PADDLE_DIGITAL_SENSITIVITY: optionPaddleDigitalSensitivity.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeSessionConfig(IO &io)
{
	optionTVPhosphor.writeWithKeyIfNotDefault(io);
	optionTVPhosphorBlend.writeWithKeyIfNotDefault(io);
	optionVideoSystem.writeWithKeyIfNotDefault(io);
	optionInputPort1.writeWithKeyIfNotDefault(io);
	optionPaddleDigitalSensitivity.writeWithKeyIfNotDefault(io);
}

bool EmuSystem::readConfig(IO &io, uint key, uint readSize)
{
	switch(key)
	{
		default: return 0;
		bcase CFGKEY_2600_TV_PHOSPHOR_BLEND: optionTVPhosphorBlend.readFromIO(io, readSize);
		bcase CFGKEY_AUDIO_RESAMPLE_QUALITY: optionAudioResampleQuality.readFromIO(io, readSize);
	}
	return 1;
}

void EmuSystem::writeConfig(IO &io)
{
	optionTVPhosphorBlend.writeWithKeyIfNotDefault(io);
	optionAudioResampleQuality.writeWithKeyIfNotDefault(io);
}

const char *optionVideoSystemToStr()
{
	switch((int)optionVideoSystem)
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

void setRuntimeTVPhosphor(int val, int blend)
{
	if(!EmuSystem::gameIsRunning() || !osystem->hasConsole())
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
	logMsg("Phosphor effect %s", usePhosphor ? "on" : "off");
	auto props = osystem->console().properties();
	if(usePhosphor)
	{
		props.set(PropType::Display_Phosphor, "Yes");
	}
	else
	{
		props.set(PropType::Display_Phosphor, "No");
	}
	osystem->console().setProperties(props);
	osystem->frameBuffer().tiaSurface().enablePhosphor(usePhosphor, blend);
}
