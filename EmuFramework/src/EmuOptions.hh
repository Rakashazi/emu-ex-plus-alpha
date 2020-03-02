#pragma once

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

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/bluetooth/BluetoothAdapter.hh>
#include <imagine/audio/AudioManager.hh>
#include <imagine/gui/View.hh>

using OptionBackNavigation = Option<OptionMethodRef<bool, View::needsBackControl>, uint8_t>;
using OptionSwappedGamepadConfirm = Option<OptionMethodFunc<bool, Input::swappedGamepadConfirm, Input::setSwappedGamepadConfirm>, uint8_t>;

bool vControllerUseScaledCoordinates();
void setVControllerUseScaledCoordinates(bool on);
using OptionTouchCtrlScaledCoordinates = Option<OptionMethodFunc<bool, vControllerUseScaledCoordinates, setVControllerUseScaledCoordinates>, uint8_t>;

#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
using OptionBlueToothScanCache = Option<OptionMethodFunc<bool, BluetoothAdapter::scanCacheUsage, BluetoothAdapter::setScanCacheUsage>, uint8_t>;
#endif

// TODO: recycle obsolete enums
enum { CFGKEY_SOUND = 0, CFGKEY_TOUCH_CONTROL_DISPLAY = 1,
	CFGKEY_AUTO_SAVE_STATE = 2, CFGKEY_LAST_DIR = 3, CFGKEY_TOUCH_CONTROL_VIRBRATE = 4,
	CFGKEY_FRAME_INTERVAL = 5, CFGKEY_FONT_Y_SIZE = 6, CFGKEY_GAME_ORIENTATION = 7,
	CFGKEY_GAME_ASPECT_RATIO = 8,
	CFGKEY_TOUCH_CONTROL_ALPHA = 9, CFGKEY_TOUCH_CONTROL_SIZE = 10,
	CFGKEY_TOUCH_CONTROL_DPAD_POS = 11, CFGKEY_TOUCH_CONTROL_FACE_BTN_POS = 12,
	CFGKEY_GAME_IMG_FILTER = 13, CFGKEY_REL_POINTER_DECEL = 14,
	CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE = 15, CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER = 16,
	CFGKEY_DPI = 17, CFGKEY_ANDROID_TEXTURE_STORAGE = 18, CFGKEY_MENU_ORIENTATION = 19,
	CFGKEY_SWAPPED_GAMEPAD_CONFIM = 20, CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE = 21,
	CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS = 22, CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS = 23,
	CFGKEY_TOUCH_CONTROL_MENU_POS = 24, CFGKEY_TOUCH_CONTROL_FF_POS = 25,
	CFGKEY_RECENT_GAMES = 26, CFGKEY_GL_SYNC_HACK = 27, CFGKEY_PAUSE_UNFOCUSED = 28,
	CFGKEY_IMAGE_ZOOM = 29, CFGKEY_TOUCH_CONTROL_IMG_PIXELS = 30, CFGKEY_SOUND_RATE = 31,
	CFGKEY_NOTIFICATION_ICON = 32, CFGKEY_ICADE = 33, CFGKEY_TITLE_BAR = 34,
	CFGKEY_BACK_NAVIGATION = 35, CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU = 36,
	CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY = 37,
	CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE = 38, CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE = 39,
	CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW = 40,
	CFGKEY_USE_OS_INPUT_METHOD = 41, CFGKEY_DITHER_IMAGE = 42,
	CFGKEY_OVERLAY_EFFECT = 43, CFGKEY_OVERLAY_EFFECT_LEVEL = 44,
	CFGKEY_LOW_PROFILE_OS_NAV = 45, CFGKEY_IDLE_DISPLAY_POWER_SAVE = 46,
	CFGKEY_SHOW_MENU_ICON = 47, CFGKEY_KEEP_BLUETOOTH_ACTIVE = 48,
	CFGKEY_HIDE_OS_NAV = 49, CFGKEY_HIDE_STATUS_BAR = 50,
	CFGKEY_BLUETOOTH_SCAN_CACHE = 51, CFGKEY_SOUND_BUFFERS = 52,
	CFGKEY_SOUND_UNDERRUN_CHECK = 53, CFGKEY_CONFIRM_AUTO_LOAD_STATE = 54,
	CFGKEY_SURFACE_TEXTURE = 55, CFGKEY_PROCESS_PRIORITY = 56,
	CFGKEY_SAVE_PATH = 57, CFGKEY_WINDOW_PIXEL_FORMAT = 58,
	CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES = 59,
	CFGKEY_INPUT_KEY_CONFIGS = 60, CFGKEY_INPUT_DEVICE_CONFIGS = 61,
	CFGKEY_CONFIRM_OVERWRITE_STATE = 62, CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE = 63,
	CFGKEY_AUDIO_SOLO_MIX = 64, CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH = 65,
	CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES = 66, CFGKEY_VIEWPORT_ZOOM = 67,
	CFGKEY_VCONTROLLER_LAYOUT_POS = 68, CFGKEY_MOGA_INPUT_SYSTEM = 69,
	CFGKEY_FAST_FORWARD_SPEED = 70, CFGKEY_SHOW_BUNDLED_GAMES = 71,
	CFGKEY_IMAGE_EFFECT = 72, CFGKEY_SHOW_ON_2ND_SCREEN = 73,
	CFGKEY_CHECK_SAVE_PATH_WRITE_ACCESS = 74, CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT = 75,
	CFGKEY_SKIP_LATE_FRAMES = 76, CFGKEY_FRAME_RATE = 77,
	CFGKEY_FRAME_RATE_PAL = 78, CFGKEY_TIME_FRAMES_WITH_SCREEN_REFRESH = 79,
	CFGKEY_SUSTAINED_PERFORMANCE_MODE = 80, CFGKEY_SHOW_BLUETOOTH_SCAN = 81,
	CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN = 82, CFGKEY_GPU_MULTITHREADING = 83
	// 256+ is reserved
};

struct OptionAspectRatio : public Option<OptionMethodVar<IG::Point2D<uint> > >
{
	constexpr OptionAspectRatio(T defaultVal, bool isConst = 0): Option<OptionMethodVar<IG::Point2D<uint> > >(CFGKEY_GAME_ASPECT_RATIO, defaultVal, isConst) {}

	uint ioSize() const override
	{
		return 2 + 2;
	}

	bool writeToIO(IO &io) override
	{
		std::error_code ec{};
		io.writeVal((uint16_t)CFGKEY_GAME_ASPECT_RATIO, &ec);
		logMsg("writing aspect ratio config %u:%u", val.x, val.y);
		io.writeVal((uint8_t)val.x, &ec);
		io.writeVal((uint8_t)val.y, &ec);
		return true;
	}

	bool readFromIO(IO &io, uint readSize)
	{
		if(isConst || readSize != 2)
		{
			logMsg("skipping %d byte option value, expected %d", readSize, 2);
			return false;
		}

		auto x = io.readVal<uint8_t>();
		auto y = io.readVal<uint8_t>();
		logMsg("read aspect ratio config %u,%u", x, y);
		if(y == 0)
			y = 1;
		val = {x, y};
		return true;
	}
};

struct OptionRecentGames : public OptionBase
{
	const uint16_t key = CFGKEY_RECENT_GAMES;

	bool isDefault() const override;
	bool writeToIO(IO &io) override;
	bool readFromIO(IO &io, uint readSize_);
	uint ioSize() const override;
};

struct OptionVControllerLayoutPosition : public OptionBase
{
	const uint16_t key = CFGKEY_VCONTROLLER_LAYOUT_POS;

	bool isDefault() const final;
	bool writeToIO(IO &io) final;
	bool readFromIO(IO &io, uint readSize_);
	uint ioSize() const final;
};

extern Byte1Option optionAutoSaveState;
extern Byte1Option optionConfirmAutoLoadState;
extern Byte1Option optionSound;
extern Byte1Option optionSoundBuffers;
extern Byte1Option optionAddSoundBuffersOnUnderrun;
#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
using OptionAudioSoloMix = Option<OptionMethodFunc<bool, IG::AudioManager::soloMix, IG::AudioManager::setSoloMix>, uint8_t>;
extern OptionAudioSoloMix optionAudioSoloMix;
#endif
extern Byte4Option optionSoundRate;
extern Byte2Option optionFontSize;
extern Byte1Option optionVibrateOnPush;
extern Byte1Option optionPauseUnfocused;
extern Byte1Option optionNotificationIcon;
extern Byte1Option optionTitleBar;
extern OptionBackNavigation optionBackNavigation;
extern Byte1Option optionSystemActionsIsDefaultMenu;
extern Byte1Option optionLowProfileOSNav;
extern Byte1Option optionHideOSNav;
extern Byte1Option optionIdleDisplayPowerSave;
extern Byte1Option optionHideStatusBar;
extern OptionSwappedGamepadConfirm optionSwappedGamepadConfirm;
extern Byte1Option optionConfirmOverwriteState;
extern Byte1Option optionFastForwardSpeed;
#ifdef CONFIG_INPUT_DEVICE_HOTSWAP
extern Byte1Option optionNotifyInputDeviceChange;
#endif
#ifdef CONFIG_INPUT_ANDROID_MOGA
extern Byte1Option optionMOGAInputSystem;
#endif

#ifdef CONFIG_BLUETOOTH
extern Byte1Option optionKeepBluetoothActive;
extern Byte1Option optionShowBluetoothScan;
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	extern OptionBlueToothScanCache optionBlueToothScanCache;
	#endif
#endif

extern Byte1Option optionImgFilter;
extern OptionAspectRatio optionAspectRatio;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
extern Byte1Option optionImgEffect;
extern Byte1Option optionImageEffectPixelFormat;
#endif
extern Byte1Option optionOverlayEffect;
extern Byte1Option optionOverlayEffectLevel;

#if 0
static const uint optionRelPointerDecelLow = 500, optionRelPointerDecelMed = 250, optionRelPointerDecelHigh = 125;
extern Byte4Option optionRelPointerDecel;
#endif

extern Byte4s1Option optionGameOrientation;
extern Byte4s1Option optionMenuOrientation;

#ifdef CONFIG_VCONTROLS_GAMEPAD
extern Byte1Option optionTouchCtrl;
extern Byte4s2Option optionTouchCtrlSize;
extern Byte4s2Option optionTouchDpadDeadzone;
extern Byte4s2Option optionTouchDpadDiagonalSensitivity;
extern Byte4s2Option optionTouchCtrlBtnSpace;
extern Byte4s2Option optionTouchCtrlBtnStagger;
extern Byte1Option optionTouchCtrlTriggerBtnPos;
extern Byte4s2Option optionTouchCtrlExtraXBtnSize;
extern Byte4s2Option optionTouchCtrlExtraYBtnSize;
extern Byte4s2Option optionTouchCtrlExtraYBtnSizeMultiRow;
extern Byte1Option optionTouchCtrlBoundingBoxes;
extern Byte1Option optionTouchCtrlShowOnTouch;
	#if defined(CONFIG_BASE_ANDROID)
	extern OptionTouchCtrlScaledCoordinates optionTouchCtrlScaledCoordinates;
	#endif
#endif
extern Byte1Option optionTouchCtrlAlpha;
extern OptionVControllerLayoutPosition optionVControllerLayoutPos;

#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
extern Byte1Option optionFrameInterval;
#endif
extern Byte1Option optionSkipLateFrames;
extern DoubleOption optionFrameRate;
extern DoubleOption optionFrameRatePAL;
extern DoubleOption optionRefreshRateOverride;

static const uint optionImageZoomIntegerOnly = 255, optionImageZoomIntegerOnlyY = 254;
extern Byte1Option optionImageZoom;
extern Byte1Option optionViewportZoom;
extern Byte1Option optionShowOnSecondScreen;

extern OptionRecentGames optionRecentGames;

#ifdef __ANDROID__
static constexpr uint8_t OPTION_ANDROID_TEXTURE_STORAGE_AUTO = 0;
static constexpr uint8_t OPTION_ANDROID_TEXTURE_STORAGE_NONE = 1;
static constexpr uint8_t OPTION_ANDROID_TEXTURE_STORAGE_GRAPHIC_BUFFER = 2;
static constexpr uint8_t OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE = 3;
static constexpr uint8_t OPTION_ANDROID_TEXTURE_STORAGE_MAX_VALUE = OPTION_ANDROID_TEXTURE_STORAGE_SURFACE_TEXTURE;
extern Byte1Option optionAndroidTextureStorage;
Gfx::Texture::AndroidStorageImpl makeAndroidStorageImpl(uint8_t val);
extern SByte1Option optionProcessPriority;
extern Byte1Option optionSustainedPerformanceMode;
#endif

#ifdef EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
extern Byte1Option optionWindowPixelFormat;
#endif
extern Byte1Option optionGPUMultiThreading;

static const char *optionSavePathDefaultToken = ":DEFAULT:";
extern PathOption optionSavePath;
extern PathOption optionLastLoadPath;
extern Byte1Option optionCheckSavePathWriteAccess;

extern Byte1Option optionShowBundledGames;

// Common options handled per-emulator backend
extern PathOption optionFirmwarePath;

void initOptions();
void setupFont(Gfx::Renderer &r, Base::Window &win);
