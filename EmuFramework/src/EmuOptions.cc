/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include "EmuOptions.hh"
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/VideoImageEffect.hh>
#include <emuframework/VideoImageOverlay.hh>
#include <emuframework/VController.hh>
#include "private.hh"
#include "privateInput.hh"
#include <imagine/base/Base.hh>
#include <imagine/base/platformExtras.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/util/bits.h>

template<class T>
bool optionFrameTimeIsValid(T val)
{
	return !val || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_NATIVE_NTSC, IG::FloatSeconds(val));
}

template<class T>
bool optionFrameTimePALIsValid(T val)
{
	return !val || EmuSystem::frameTimeIsValid(EmuSystem::VIDSYS_PAL, IG::FloatSeconds(val));
}

bool optionOrientationIsValid(uint32_t val)
{
	return val == Base::VIEW_ROTATE_AUTO ||
			val == Base::VIEW_ROTATE_0 ||
			val == Base::VIEW_ROTATE_90 ||
			val == Base::VIEW_ROTATE_180 ||
			val == Base::VIEW_ROTATE_270;
}

bool optionAspectRatioIsValid(double val)
{
	return val == 0. || (val >= 0.1 && val <= 10.);
}

Byte1Option optionAutoSaveState(CFGKEY_AUTO_SAVE_STATE, 1);
Byte1Option optionConfirmAutoLoadState(CFGKEY_CONFIRM_AUTO_LOAD_STATE, 1);

constexpr uint8_t OPTION_SOUND_ENABLED_FLAG = IG::bit(0);
constexpr uint8_t OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG = IG::bit(1);
constexpr uint8_t OPTION_SOUND_DEFAULT_FLAGS = OPTION_SOUND_ENABLED_FLAG | OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG;

Byte1Option optionSound(CFGKEY_SOUND, OPTION_SOUND_DEFAULT_FLAGS);
Byte1Option optionSoundVolume(CFGKEY_SOUND_VOLUME,
	100, false, optionIsValidWithMinMax<0, 100, uint8_t>);

Byte1Option optionSoundBuffers(CFGKEY_SOUND_BUFFERS,
	4, 0, optionIsValidWithMinMax<2, 8, uint8_t>);
Byte1Option optionAddSoundBuffersOnUnderrun(CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN, 1, 0);

#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
OptionAudioSoloMix optionAudioSoloMix(CFGKEY_AUDIO_SOLO_MIX, 1);
#endif

#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
Byte1Option optionAudioAPI(CFGKEY_AUDIO_API, 0);
#endif

Byte4Option optionSoundRate(CFGKEY_SOUND_RATE, 48000, false, optionIsValidWithMax<48000>);

// Store in micro-meters
Byte2Option optionFontSize(CFGKEY_FONT_Y_SIZE,
	Config::MACHINE_IS_PANDORA ? 6500 :
	(Config::envIsIOS || Config::envIsAndroid) ? 3000 :
	8000,
	0, optionIsValidWithMinMax<2000, 10000, uint16_t>);

Byte1Option optionVibrateOnPush(CFGKEY_TOUCH_CONTROL_VIRBRATE, 0, !Config::BASE_SUPPORTS_VIBRATOR);

Byte1Option optionPauseUnfocused(CFGKEY_PAUSE_UNFOCUSED, 1,
	!(Config::envIsLinux || Config::envIsAndroid));

Byte1Option optionNotificationIcon(CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid);
Byte1Option optionTitleBar(CFGKEY_TITLE_BAR, 1, Config::envIsIOS);

OptionBackNavigation
	optionBackNavigation(CFGKEY_BACK_NAVIGATION, View::needsBackControlDefault, View::needsBackControlIsConst);

Byte1Option optionSystemActionsIsDefaultMenu(CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU, 1, 0);
Byte1Option optionLowProfileOSNav(CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid);
Byte1Option optionHideOSNav(CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid);
Byte1Option optionIdleDisplayPowerSave(CFGKEY_IDLE_DISPLAY_POWER_SAVE, 0, !Config::envIsAndroid && !Config::envIsIOS);
Byte1Option optionHideStatusBar(CFGKEY_HIDE_STATUS_BAR, 1, !Config::envIsAndroid && !Config::envIsIOS);
OptionSwappedGamepadConfirm optionSwappedGamepadConfirm(CFGKEY_SWAPPED_GAMEPAD_CONFIM, Input::SWAPPED_GAMEPAD_CONFIRM_DEFAULT);
Byte1Option optionConsumeUnboundGamepadKeys(CFGKEY_CONSUME_UNBOUND_GAMEPAD_KEYS, 0, 0);
Byte1Option optionConfirmOverwriteState(CFGKEY_CONFIRM_OVERWRITE_STATE, 1, 0);
Byte1Option optionFastForwardSpeed(CFGKEY_FAST_FORWARD_SPEED, 4, 0, optionIsValidWithMinMax<2, 7>);
#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
Byte1Option optionNotifyInputDeviceChange(CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE, Config::Input::DEVICE_HOTSWAP, !Config::Input::DEVICE_HOTSWAP);
#endif
#ifdef CONFIG_INPUT_ANDROID_MOGA
Byte1Option optionMOGAInputSystem(CFGKEY_MOGA_INPUT_SYSTEM, 0, 0);
#endif

#ifdef CONFIG_BLUETOOTH
Byte1Option optionKeepBluetoothActive(CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0, !Config::BASE_CAN_BACKGROUND_APP);
Byte1Option optionShowBluetoothScan(CFGKEY_SHOW_BLUETOOTH_SCAN, 1);
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	OptionBlueToothScanCache optionBlueToothScanCache(CFGKEY_BLUETOOTH_SCAN_CACHE, 1);
	#endif
#endif

DoubleOption optionAspectRatio{CFGKEY_GAME_ASPECT_RATIO, (double)EmuSystem::aspectRatioInfo[0], 0, optionAspectRatioIsValid};

Byte1Option optionImgFilter(CFGKEY_GAME_IMG_FILTER, 1, 0);
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
Byte1Option optionImgEffect(CFGKEY_IMAGE_EFFECT, 0, 0, optionIsValidWithMax<VideoImageEffect::LAST_EFFECT_VAL-1>);
#endif
Byte1Option optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL>);
Byte1Option optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25, 0, optionIsValidWithMax<100>);

bool imageEffectPixelFormatIsValid(uint8_t val)
{
	switch(val)
	{
		case IG::PIXEL_NONE:
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
Byte1Option optionImageEffectPixelFormat(CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT, IG::PIXEL_NONE, 0, imageEffectPixelFormatIsValid);
#endif

Byte1Option optionVideoImageBuffers{CFGKEY_VIDEO_IMAGE_BUFFERS, 0, 0,
	optionIsValidWithMax<2>};

#if 0
Byte4Option optionRelPointerDecel(CFGKEY_REL_POINTER_DECEL, optionRelPointerDecelMed,
		!Config::envIsAndroid, optionIsValidWithMax<optionRelPointerDecelHigh>);
#endif

Byte4s1Option optionGameOrientation(CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS) ? Base::VIEW_ROTATE_AUTO : Base::VIEW_ROTATE_0,
		false, optionOrientationIsValid);

Byte4s1Option optionMenuOrientation(CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS) ? Base::VIEW_ROTATE_AUTO : Base::VIEW_ROTATE_0,
		false, optionOrientationIsValid);

Byte1Option optionTouchCtrl(CFGKEY_TOUCH_CONTROL_DISPLAY,
		Config::envIsLinux ? 0 : 2,
		false, optionIsValidWithMax<2>);

Byte1Option optionTouchCtrlAlpha(CFGKEY_TOUCH_CONTROL_ALPHA,
		255 * .5,
		false);

Byte4s2Option optionTouchCtrlSize
		(CFGKEY_TOUCH_CONTROL_SIZE,
		850,
		false, optionIsValidWithMinMax<300, 1500>);
Byte4s2Option optionTouchDpadDeadzone
		(CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE,
		135,
		false, optionIsValidWithMax<160>);
Byte4s2Option optionTouchDpadDiagonalSensitivity
		(CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY,
		1750,
		false, optionIsValidWithMinMax<1000,2500>);
Byte4s2Option optionTouchCtrlBtnSpace
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE,
		200,
		false, optionIsValidWithMax<400>);
Byte4s2Option optionTouchCtrlBtnStagger
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER,
		1,
		false, optionIsValidWithMax<5>);
Byte1Option optionTouchCtrlTriggerBtnPos
		(CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS,
		0, false);
Byte4s2Option optionTouchCtrlExtraXBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE,
		200, false, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE,
		1000, false, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSizeMultiRow
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW,
		200, false, optionIsValidWithMax<1000>);

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

Byte1Option optionTouchCtrlBoundingBoxes(CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES, 0);
Byte1Option optionTouchCtrlShowOnTouch(CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH, 1);
OptionTouchCtrlScaledCoordinates optionTouchCtrlScaledCoordinates(CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES, 1, !Config::envIsAndroid);

OptionVControllerLayoutPosition optionVControllerLayoutPos;

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
Byte1Option optionFrameInterval
	{CFGKEY_FRAME_INTERVAL,	1, !Config::envIsIOS, optionIsValidWithMinMax<1, 4>};
#endif
Byte1Option optionSkipLateFrames{CFGKEY_SKIP_LATE_FRAMES, 1, 0};
DoubleOption optionFrameRate{CFGKEY_FRAME_RATE, 0, 0, optionFrameTimeIsValid};
DoubleOption optionFrameRatePAL{CFGKEY_FRAME_RATE_PAL, 1./50., !EmuSystem::hasPALVideoSystem, optionFrameTimePALIsValid};

bool optionImageZoomIsValid(uint8_t val)
{
	return val == optionImageZoomIntegerOnly || val == optionImageZoomIntegerOnlyY
		|| (val >= 10 && val <= 100);
}
Byte1Option optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid);
Byte1Option optionViewportZoom(CFGKEY_VIEWPORT_ZOOM, 100, 0, optionIsValidWithMinMax<50, 100>);
Byte1Option optionShowOnSecondScreen{CFGKEY_SHOW_ON_2ND_SCREEN, 1, 0};

OptionRecentGames optionRecentGames;

Byte1Option optionTextureBufferMode{CFGKEY_TEXTURE_BUFFER_MODE, 0};
#ifdef __ANDROID__
Byte1Option optionSustainedPerformanceMode{CFGKEY_SUSTAINED_PERFORMANCE_MODE, 0};
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
bool windowPixelFormatIsValid(uint8_t val)
{
	switch(val)
	{
		case IG::PIXEL_NONE:
		case IG::PIXEL_RGB565:
		case IG::PIXEL_RGB888:
		case IG::PIXEL_RGBX8888:
		case IG::PIXEL_RGBA8888:
			return true;
	}
	return false;
}

Byte1Option optionWindowPixelFormat(CFGKEY_WINDOW_PIXEL_FORMAT, IG::PIXEL_NONE, 0, windowPixelFormatIsValid);
#endif

PathOption optionSavePath(CFGKEY_SAVE_PATH, EmuSystem::savePath_, "");
PathOption optionLastLoadPath(CFGKEY_LAST_DIR, lastLoadPath, "");
Byte1Option optionCheckSavePathWriteAccess{CFGKEY_CHECK_SAVE_PATH_WRITE_ACCESS, 1};

Byte1Option optionShowBundledGames(CFGKEY_SHOW_BUNDLED_GAMES, 1);

[[gnu::weak]] PathOption optionFirmwarePath(0, nullptr, 0, nullptr);

void initOptions()
{
	if(!strlen(lastLoadPath.data()))
	{
		lastLoadPath = Base::sharedStoragePath();
	}

	optionSoundRate.initDefault(IG::AudioManager::nativeRate());

	#ifdef CONFIG_BASE_IOS
	if(Base::deviceIsIPad())
		optionFontSize.initDefault(5000);
	#endif

	#ifdef CONFIG_INPUT_ANDROID_MOGA
	if(Base::packageIsInstalled("com.bda.pivot.mogapgp"))
	{
		logMsg("MOGA Pivot app is present");
		optionMOGAInputSystem.initDefault(1);
	}
	#endif

	#ifdef CONFIG_BASE_ANDROID
	if(Base::hasHardwareNavButtons())
	{
		optionLowProfileOSNav.isConst = 1;
		optionHideOSNav.isConst = 1;
	}
	else
	{
		optionBackNavigation.initDefault(1);
		if(Base::androidSDK() >= 19)
			optionHideOSNav.initDefault(1);
	}
	if(Base::androidSDK() >= 11)
	{
		optionNotificationIcon.initDefault(false);
		if(Base::androidSDK() >= 17)
			optionNotificationIcon.isConst = true;
	}
	if(Base::androidSDK() < 12)
	{
		#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
		optionNotifyInputDeviceChange.isConst = 1;
		#endif
	}
	if(!Base::hasVibrator())
	{
		optionVibrateOnPush.isConst = 1;
	}
	if(Base::androidSDK() < 17)
	{
		optionShowOnSecondScreen.isConst = true;
	}
	else if(FS::exists(FS::makePathStringPrintf("%s/emuex_disable_presentation_displays", Base::sharedStoragePath().data())))
	{
		logMsg("force-disabling presentation display support");
		optionShowOnSecondScreen.initDefault(false);
		optionShowOnSecondScreen.isConst = true;
	}
	else
	{
		#ifdef CONFIG_BLUETOOTH
		optionShowBluetoothScan.initDefault(0);
		#endif
	}
	{
		auto type = Base::sustainedPerformanceModeType();
		if(type == Base::SustainedPerformanceType::NONE)
		{
			optionSustainedPerformanceMode.initDefault(0);
			optionSustainedPerformanceMode.isConst = true;
		}
	}
	if(Base::androidSDK() < 11)
	{
		// never run app in onPaused state on Android 2.3
		optionPauseUnfocused.isConst = true;
	}
	#endif

	if(!Base::Screen::screen(0)->frameRateIsReliable())
	{
		optionFrameRate.initDefault(60);
	}

	if(!EmuApp::autoSaveStateDefault)
	{
		optionAutoSaveState.initDefault(false);
	}

	if(!EmuApp::hasIcon)
	{
		optionNotificationIcon.initDefault(false);
		optionNotificationIcon.isConst = true;
	}

	if(EmuSystem::forcedSoundRate)
	{
		optionSoundRate.initDefault(EmuSystem::forcedSoundRate);
		optionSoundRate.isConst = true;
	}

	if(!EmuSystem::hasSound)
	{
		optionSound.initDefault(0);
	}

	if(EmuSystem::constFrameRate)
	{
		optionFrameRate.isConst = true;
	}

	EmuSystem::initOptions();

	bool defaultToLargeControls = false;
	#ifdef CONFIG_BASE_IOS
	if(Base::deviceIsIPad())
		defaultToLargeControls = true;
	#endif
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(defaultToLargeControls)
		optionTouchCtrlSize.initDefault(1400);
	#endif
}

bool OptionVControllerLayoutPosition::isDefault() const
{
	return !vController->layoutPositionChanged();
}

bool OptionVControllerLayoutPosition::writeToIO(IO &io)
{
	logMsg("writing vcontroller positions");
	io.write(key);
	for(auto &posArr : vController->layoutPosition())
	{
		for(auto &e : posArr)
		{
			io.write((uint8_t)e.origin);
			io.write((uint8_t)e.state);
			io.write((int32_t)e.pos.x);
			io.write((int32_t)e.pos.y);
		}
	}
	return 1;
}

static uint sizeofVControllerLayoutPositionEntry()
{
	return 1 + 1 + 4 + 4;
}

bool OptionVControllerLayoutPosition::readFromIO(IO &io, uint readSize_)
{
	int readSize = readSize_;

	for(auto &posArr : vController->layoutPosition())
	{
		for(auto &e : posArr)
		{
			if(readSize < (int)sizeofVControllerLayoutPositionEntry())
			{
				logMsg("expected position data but only %d bytes left", readSize);
				break;
			}

			_2DOrigin origin = _2DOrigin{(uint8_t)io.get<int8_t>()};
			if(!origin.isValid())
			{
				logWarn("invalid v-controller origin from config file");
			}
			else
				e.origin = origin;
			uint state = io.get<int8_t>();
			if(state > 2)
			{
				logWarn("invalid v-controller state from config file");
			}
			else
				e.state = state;
			e.pos.x = io.get<int32_t>();
			e.pos.y = io.get<int32_t>();
			vController->setLayoutPositionChanged();
			readSize -= sizeofVControllerLayoutPositionEntry();
		}
	}

	if(readSize)
	{
		logMsg("skipping excess %d bytes", readSize);
	}

	return 1;
}

uint OptionVControllerLayoutPosition::ioSize() const
{
	uint positions = std::size(vController->layoutPosition()[0]) * std::size(vController->layoutPosition());
	return sizeof(key) + positions * sizeofVControllerLayoutPositionEntry();
}

void OptionVControllerLayoutPosition::setVController(VController &v)
{
	vController = &v;
}

bool vControllerUseScaledCoordinates()
{
	#ifdef CONFIG_BASE_ANDROID
	return defaultVController().usesScaledCoordinates();
	#else
	return false;
	#endif
}

void setVControllerUseScaledCoordinates(bool on)
{
	#ifdef CONFIG_BASE_ANDROID
	defaultVController().setUsesScaledCoordinates(on);
	#endif
}

void setupFont(Gfx::Renderer &r, Base::Window &win)
{
	float size = optionFontSize / 1000.;
	logMsg("setting up font size %f", (double)size);
	View::defaultFace.setFontSettings(r, IG::FontSettings(win.heightSMMInPixels(size)));
	View::defaultBoldFace.setFontSettings(r, IG::FontSettings(win.heightSMMInPixels(size)));
}

bool OptionRecentGames::isDefault() const
{
	return recentGameList.size() == 0;
}

bool OptionRecentGames::writeToIO(IO &io)
{
	logMsg("writing recent list");
	io.write(key);
	for(auto &e : recentGameList)
	{
		uint len = strlen(e.path.data());
		io.write((uint16_t)len);
		io.write(e.path.data(), len);
	}
	return true;
}

bool OptionRecentGames::readFromIO(IO &io, uint readSize_)
{
	int readSize = readSize_;
	while(readSize && !recentGameList.isFull())
	{
		if(readSize < 2)
		{
			logMsg("expected string length but only %d bytes left", readSize);
			break;
		}

		auto len = io.get<uint16_t>();
		readSize -= 2;

		if(len > readSize)
		{
			logMsg("string length %d longer than %d bytes left", len, readSize);
			break;
		}

		RecentGameInfo info;
		auto bytesRead = io.read(info.path.data(), len);
		if(bytesRead == -1)
		{
			logErr("error reading string option");
			return true;
		}
		if(!bytesRead)
			continue; // don't add empty paths
		info.path[bytesRead] = 0;
		readSize -= len;
		info.name = EmuSystem::fullGameNameForPath(info.path.data());
		//logMsg("adding game to recent list: %s, name: %s", info.path, info.name);
		recentGameList.push_back(info);
	}

	if(readSize)
	{
		logMsg("skipping excess %d bytes", readSize);
	}

	return true;
}

uint OptionRecentGames::ioSize() const
{
	uint strSizes = 0;
	for(auto &e : recentGameList)
	{
		strSizes += 2;
		strSizes += strlen(e.path.data());
	}
	return sizeof(key) + strSizes;
}

bool PathOption::writeToIO(IO &io)
{
	uint len = strlen(val);
	if(len > strSize-1)
	{
		logErr("option string too long to write");
		return 0;
	}
	else if(!len)
	{
		logMsg("skipping 0 length option string");
		return 0;
	}
	io.write((uint16_t)(2 + len));
	io.write((uint16_t)KEY);
	io.write(val, len);
	return true;
}

bool PathOption::readFromIO(IO &io, uint readSize)
{
	if(readSize > strSize-1)
	{
		logMsg("skipping %d byte string option value, max is %d", readSize, strSize-1);
		return 0;
	}

	auto bytesRead = io.read(val, readSize);
	if(bytesRead == -1)
	{
		logErr("error reading string option");
		return 0;
	}
	val[bytesRead] = 0;
	logMsg("read path option %s", val);
	return 1;
}

uint PathOption::ioSize() const
{
	return sizeof(KEY) + strlen(val);
}

uint8_t currentFrameInterval()
{
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	return optionFrameInterval;
	#else
	return 1;
	#endif
}

IG::PixelFormatID optionImageEffectPixelFormatValue()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	return (IG::PixelFormatID)optionImageEffectPixelFormat.val;
	#else
	return IG::PIXEL_NONE;
	#endif
}

bool soundIsEnabled()
{
	return optionSound & OPTION_SOUND_ENABLED_FLAG;
}

void setSoundEnabled(bool on)
{
	optionSound = IG::setOrClearBits(optionSound.val, OPTION_SOUND_ENABLED_FLAG, on);
}

bool soundDuringFastForwardIsEnabled()
{
	return optionSound & OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG;
}

void setSoundDuringFastForwardEnabled(bool on)
{
	optionSound = IG::setOrClearBits(optionSound.val, OPTION_SOUND_DURING_FAST_FORWARD_ENABLED_FLAG, on);
}

IG::Audio::Api audioOutputAPI()
{
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	return (IG::Audio::Api)optionAudioAPI.val;
	#else
	return IG::Audio::Api::DEFAULT;
	#endif
}
