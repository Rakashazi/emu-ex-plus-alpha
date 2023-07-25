#pragma once

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

#include <stella/emucore/Props.hxx>
#include <stella/emucore/Control.hxx>
#include <stella/emucore/Paddles.hxx>
#include <OSystem.hxx>
#include <emuframework/EmuSystem.hh>

namespace EmuEx
{

enum class PaddleRegionMode : uint8_t
{
	OFF = 0,
	LEFT = 1,
	RIGHT = 2,
	FULL = 3,
};

enum
{
	CFGKEY_2600_TV_PHOSPHOR = 270, CFGKEY_VIDEO_SYSTEM = 271,
	CFGKEY_2600_TV_PHOSPHOR_BLEND = 272, CFGKEY_AUDIO_RESAMPLE_QUALITY = 273,
	CFGKEY_INPUT_PORT_1 = 274, CFGKEY_INPUT_PORT_2 = 275,
	CFGKEY_PADDLE_DIGITAL_SENSITIVITY = 276, CFGKEY_PADDLE_ANALOG_REGION = 277,
};

static constexpr int TV_PHOSPHOR_AUTO = 2;

inline bool optionIsValidControllerType(uint8_t val)
{
	switch(Controller::Type(val))
	{
		case Controller::Type::Unknown:
		case Controller::Type::Joystick:
		case Controller::Type::Genesis:
		case Controller::Type::BoosterGrip:
		case Controller::Type::Keyboard:
		case Controller::Type::Paddles:
			return true;
		default:
			return false;
	}
}

const char *optionVideoSystemToStr(uint8_t sysIdx);
Controller::Type limitToSupportedControllerTypes(Controller::Type type);
const char *asString(Controller::Type type);

class A2600System final: public EmuSystem
{
public:
	OSystem osystem;
	float configuredInputVideoFrameRate{};
	Properties defaultGameProps{};
	bool p1DiffB = true, p2DiffB = true, vcsColor = true;
	Controller::Type autoDetectedInput1{};
	std::array<Event::Type, 2> jsFireMap{Event::LeftJoystickFire, Event::RightJoystickFire};
	std::array<Event::Type, 2> jsLeftMap{Event::LeftJoystickLeft, Event::RightJoystickLeft};
	std::array<Event::Type, 2> jsRightMap{Event::LeftJoystickRight, Event::RightJoystickRight};
	Byte1Option optionTVPhosphor{CFGKEY_2600_TV_PHOSPHOR, TV_PHOSPHOR_AUTO, false, optionIsValidWithMax<2>};
	Byte1Option optionTVPhosphorBlend{CFGKEY_2600_TV_PHOSPHOR_BLEND, 80, false, optionIsValidWithMax<100>};
	Byte1Option optionVideoSystem{CFGKEY_VIDEO_SYSTEM, 0, false, optionIsValidWithMax<6>};
	Byte1Option optionAudioResampleQuality{CFGKEY_AUDIO_RESAMPLE_QUALITY,
		(uint8_t)AudioSettings::DEFAULT_RESAMPLING_QUALITY, false,
		optionIsValidWithMinMax<(uint8_t)AudioSettings::ResamplingQuality::nearestNeightbour, (uint8_t)AudioSettings::ResamplingQuality::lanczos_3>};
	Byte1Option optionInputPort1{CFGKEY_INPUT_PORT_1, 0, false, optionIsValidControllerType};
	Byte1Option optionPaddleDigitalSensitivity{CFGKEY_PADDLE_DIGITAL_SENSITIVITY, 1, false,
		optionIsValidWithMinMax<1, 20>};
	Byte1Option optionPaddleAnalogRegion{CFGKEY_PADDLE_ANALOG_REGION, 1, false,
		optionIsValidWithMax<3>};

	A2600System(ApplicationContext ctx, EmuApp &app):
		EmuSystem{ctx}, osystem{app}
	{
		Paddles::setDigitalSensitivity(5);
		Paddles::setMouseSensitivity(7);
	}

	void setRuntimeTVPhosphor(int val, int blend);
	void setControllerType(EmuApp &, Console &console, Controller::Type type);
	void updatePaddlesRegionMode(EmuApp &, PaddleRegionMode);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &io, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const;
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	bool onPointerInputStart(const Input::MotionEvent &, Input::DragTrackerState, WindowRect gameRect);
	bool onPointerInputUpdate(const Input::MotionEvent &, Input::DragTrackerState current, Input::DragTrackerState previous, WindowRect gameRect);
	VideoSystem videoSystem() const;
	void renderFramebuffer(EmuVideo &);
	bool onVideoRenderFormatChange(EmuVideo &, PixelFormat);
	bool resetSessionOptions(EmuApp &);

private:
	bool updatePaddle(Input::DragTrackerState dragState);
	void updateSwitchValues();
	void updateJoytickMapping(EmuApp &app, Controller::Type type);
};

using MainSystem = A2600System;

}
