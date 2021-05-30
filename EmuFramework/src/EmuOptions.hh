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
#include <imagine/audio/defs.hh>
#include <imagine/gui/View.hh>
#include <imagine/gfx/PixmapBufferTexture.hh>
#include <imagine/input/Input.hh>

namespace Base
{
class ApplicationContext;
}

class VController;

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
	CFGKEY_DPI = 17, CFGKEY_TEXTURE_BUFFER_MODE = 18, CFGKEY_MENU_ORIENTATION = 19,
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
	CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN = 82, CFGKEY_VIDEO_IMAGE_BUFFERS = 83,
	CFGKEY_AUDIO_API = 84, CFGKEY_SOUND_VOLUME = 85,
	CFGKEY_CONSUME_UNBOUND_GAMEPAD_KEYS = 86, CFGKEY_VIDEO_COLOR_SPACE = 87,
	CFGKEY_RENDER_PIXEL_FORMAT = 88
	// 256+ is reserved
};

struct OptionRecentGames : public OptionBase
{
	const uint16_t key = CFGKEY_RECENT_GAMES;

	bool isDefault() const override;
	bool writeToIO(IO &io) override;
	bool readFromIO(Base::ApplicationContext, IO &, unsigned readSize_);
	unsigned ioSize() const override;
};

struct OptionVControllerLayoutPosition : public OptionBase
{
	const uint16_t key = CFGKEY_VCONTROLLER_LAYOUT_POS;
	VController *vController{};

	constexpr OptionVControllerLayoutPosition() {}
	bool isDefault() const final;
	bool writeToIO(IO &io) final;
	bool readFromIO(IO &io, unsigned readSize_);
	unsigned ioSize() const final;
	void setVController(VController &);
};

extern Byte1Option optionAutoSaveState;
extern Byte1Option optionConfirmAutoLoadState;
extern Byte1Option optionSound;
extern Byte1Option optionSoundVolume;
extern Byte1Option optionSoundBuffers;
extern Byte1Option optionAddSoundBuffersOnUnderrun;
#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
extern Byte1Option optionAudioAPI;
#endif
extern Byte4Option optionSoundRate;
extern Byte2Option optionFontSize;
extern Byte1Option optionVibrateOnPush;
extern Byte1Option optionPauseUnfocused;
extern Byte1Option optionNotificationIcon;
extern Byte1Option optionTitleBar;
extern Byte1Option optionSystemActionsIsDefaultMenu;
extern Byte1Option optionLowProfileOSNav;
extern Byte1Option optionHideOSNav;
extern Byte1Option optionIdleDisplayPowerSave;
extern Byte1Option optionHideStatusBar;
extern Byte1Option optionConsumeUnboundGamepadKeys;
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
extern DoubleOption optionAspectRatio;
#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
extern Byte1Option optionImgEffect;
extern Byte1Option optionImageEffectPixelFormat;
#endif
extern Byte1Option optionOverlayEffect;
extern Byte1Option optionOverlayEffectLevel;

#if 0
static const unsigned optionRelPointerDecelLow = 500, optionRelPointerDecelMed = 250, optionRelPointerDecelHigh = 125;
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

static const unsigned optionImageZoomIntegerOnly = 255, optionImageZoomIntegerOnlyY = 254;
extern Byte1Option optionImageZoom;
extern Byte1Option optionViewportZoom;
extern Byte1Option optionShowOnSecondScreen;

extern OptionRecentGames optionRecentGames;

extern Byte1Option optionTextureBufferMode;
#ifdef __ANDROID__
extern Byte1Option optionSustainedPerformanceMode;
#endif

extern Byte1Option optionVideoImageBuffers;

static const char *optionSavePathDefaultToken = ":DEFAULT:";
extern PathOption optionSavePath;
extern Byte1Option optionCheckSavePathWriteAccess;

extern Byte1Option optionShowBundledGames;

// Common options handled per-emulator backend
extern PathOption optionFirmwarePath;

void setupFont(ViewManager &manager, Gfx::Renderer &r, Base::Window &win);
bool soundIsEnabled();
void setSoundEnabled(bool on);
bool soundDuringFastForwardIsEnabled();
void setSoundDuringFastForwardEnabled(bool on);
IG::Audio::Api audioOutputAPI();
