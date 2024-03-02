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
#include <emuframework/EmuOptions.hh>

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

constexpr bool optionIsValidControllerType(const auto &v)
{
	switch(Controller::Type(v))
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
	size_t saveStateSize{};
	float configuredInputVideoFrameRate{};
	Properties defaultGameProps{};
	bool p1DiffB = true, p2DiffB = true, vcsColor = true;
	Controller::Type autoDetectedInput1{};
	std::array<Event::Type, 2> jsFireMap{Event::LeftJoystickFire, Event::RightJoystickFire};
	std::array<Event::Type, 2> jsLeftMap{Event::LeftJoystickLeft, Event::RightJoystickLeft};
	std::array<Event::Type, 2> jsRightMap{Event::LeftJoystickRight, Event::RightJoystickRight};
	Property<uint8_t, CFGKEY_2600_TV_PHOSPHOR,
		PropertyDesc<uint8_t>{.defaultValue = TV_PHOSPHOR_AUTO, .isValid = isValidWithMax<2>}> optionTVPhosphor;
	Property<int8_t, CFGKEY_2600_TV_PHOSPHOR_BLEND,
		PropertyDesc<int8_t>{.defaultValue = 80, .isValid = isValidWithMax<100>}> optionTVPhosphorBlend;
	Property<uint8_t, CFGKEY_VIDEO_SYSTEM,
		PropertyDesc<uint8_t>{.isValid = isValidWithMax<6>}> optionVideoSystem;
	Property<AudioSettings::ResamplingQuality, CFGKEY_AUDIO_RESAMPLE_QUALITY,
		PropertyDesc<AudioSettings::ResamplingQuality>{.defaultValue = AudioSettings::DEFAULT_RESAMPLING_QUALITY,
		.isValid = isValidWithMinMax<AudioSettings::ResamplingQuality::nearestNeightbour, AudioSettings::ResamplingQuality::lanczos_3>}>
		optionAudioResampleQuality;
	Property<Controller::Type, CFGKEY_INPUT_PORT_1,
		PropertyDesc<Controller::Type>{.isValid = optionIsValidControllerType}> optionInputPort1;
	Property<int8_t, CFGKEY_PADDLE_DIGITAL_SENSITIVITY,
		PropertyDesc<int8_t>{.defaultValue = 1, .isValid = isValidWithMinMax<1, 20>}> optionPaddleDigitalSensitivity;
	Property<int8_t, CFGKEY_PADDLE_ANALOG_REGION,
		PropertyDesc<int8_t>{.defaultValue = 1, .isValid = isValidWithMax<3>}> optionPaddleAnalogRegion;

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
	size_t stateSize() { return saveStateSize; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &io, unsigned key);
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
