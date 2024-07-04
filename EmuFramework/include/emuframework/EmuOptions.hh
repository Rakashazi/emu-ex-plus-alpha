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

#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/Property.hh>
#include <imagine/util/enum.hh>
#include <cstdint>

namespace EmuEx
{

// TODO: recycle obsolete enums
enum { CFGKEY_SOUND = 0, CFGKEY_TOUCH_CONTROL_DISPLAY = 1,
	CFGKEY_AUTOSAVE_TIMER_MINS = 2, CFGKEY_LAST_DIR = 3, CFGKEY_TOUCH_CONTROL_VIRBRATE = 4,
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
	CFGKEY_RECENT_CONTENT = 26, CFGKEY_GL_SYNC_HACK = 27, CFGKEY_PAUSE_UNFOCUSED = 28,
	CFGKEY_CONTENT_SCALE = 29, CFGKEY_TOUCH_CONTROL_IMG_PIXELS = 30, CFGKEY_SOUND_RATE = 31,
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
	CFGKEY_SOUND_UNDERRUN_CHECK = 53, CFGKEY_AUTOSAVE_LAUNCH_MODE_OLD = 54,
	CFGKEY_SURFACE_TEXTURE = 55, CFGKEY_PROCESS_PRIORITY = 56,
	CFGKEY_SAVE_PATH = 57, CFGKEY_WINDOW_PIXEL_FORMAT = 58,
	CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES = 59,
	CFGKEY_INPUT_KEY_CONFIGS = 60, CFGKEY_INPUT_DEVICE_CONFIGS = 61,
	CFGKEY_CONFIRM_OVERWRITE_STATE = 62, CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE = 63,
	CFGKEY_AUDIO_SOLO_MIX = 64, CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH = 65,
	CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES = 66, CFGKEY_MENU_SCALE = 67,
	CFGKEY_VCONTROLLER_LAYOUT_POS = 68, CFGKEY_MOGA_INPUT_SYSTEM = 69,
	CFGKEY_FAST_MODE_SPEED = 70, CFGKEY_SHOW_BUNDLED_GAMES = 71,
	CFGKEY_IMAGE_EFFECT = 72, CFGKEY_SHOW_ON_2ND_SCREEN = 73,
	CFGKEY_CHECK_SAVE_PATH_WRITE_ACCESS = 74, CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT = 75,
	CFGKEY_SKIP_LATE_FRAMES = 76, CFGKEY_FRAME_RATE = 77,
	CFGKEY_FRAME_RATE_PAL = 78, CFGKEY_TIME_FRAMES_WITH_SCREEN_REFRESH = 79,
	CFGKEY_SUSTAINED_PERFORMANCE_MODE = 80, CFGKEY_SHOW_BLUETOOTH_SCAN = 81,
	CFGKEY_ADD_SOUND_BUFFERS_ON_UNDERRUN = 82, CFGKEY_VIDEO_IMAGE_BUFFERS = 83,
	CFGKEY_AUDIO_API = 84, CFGKEY_SOUND_VOLUME = 85,
	CFGKEY_CONSUME_UNBOUND_GAMEPAD_KEYS = 86, CFGKEY_VIDEO_COLOR_SPACE = 87,
	CFGKEY_RENDER_PIXEL_FORMAT = 88, CFGKEY_RUN_FRAMES_IN_THREAD = 89,
	CFGKEY_SHOW_HIDDEN_FILES = 90, CFGKEY_RENDERER_PRESENTATION_TIME = 91,
	CFGKEY_LAYOUT_BEHIND_SYSTEM_UI = 92, CFGKEY_VCONTROLLER_ALLOW_PAST_CONTENT_BOUNDS = 93,
	CFGKEY_CONTENT_ROTATION = 94, CFGKEY_OVERRIDE_SCREEN_FRAME_RATE = 95,
	CFGKEY_VIDEO_BRIGHTNESS = 96, CFGKEY_SCREENSHOTS_PATH = 97,
	CFGKEY_AUTOSAVE_LAUNCH_MODE = 98, CFGKEY_VCONTROLLER_DPAD = 99,
	CFGKEY_VCONTROLLER_DEVICE_BUTTONS = 100, CFGKEY_VCONTROLLER_UI_BUTTONS = 101,
	CFGKEY_VIDEO_LANDSCAPE_OFFSET = 102, CFGKEY_VIDEO_PORTRAIT_OFFSET = 103,
	CFGKEY_AUTOSAVE_CONTENT = 104, CFGKEY_SLOW_MODE_SPEED = 105,
	CFGKEY_VIDEO_LANDSCAPE_ASPECT_RATIO = 106, CFGKEY_VIDEO_PORTRAIT_ASPECT_RATIO = 107,
	CFGKEY_CPU_AFFINITY_MASK = 108, CFGKEY_CPU_AFFINITY_MODE = 109,
	CFGKEY_RENDERER_PRESENT_MODE = 110, CFGKEY_BLANK_FRAME_INSERTION = 111,
	CFGKEY_VCONTROLLER_DEVICE_BUTTONS_V2 = 112, CFGKEY_VCONTROLLER_UI_BUTTONS_V2 = 113,
	CFGKEY_INPUT_KEY_CONFIGS_V2 = 114, CFGKEY_VCONTROLLER_HIGHLIGHT_PUSHED_BUTTONS = 115,
	CFGKEY_RECENT_CONTENT_V2 = 116, CFGKEY_MAX_RECENT_CONTENT = 117,
	CFGKEY_REWIND_STATES = 118, CFGKEY_REWIND_TIMER_SECS = 119,
	CFGKEY_FRAME_CLOCK = 120, CFGKEY_INPUT_DEVICE_CONTENT_CONFIGS = 121,
	// 256+ is reserved
};

enum class AutoTristate : uint8_t
{
	Auto, Off, On
};

enum class InEmuTristate : uint8_t
{
	Off, InEmu, On
};

constexpr unsigned optionContentScaleIntegerOnly = 255, optionContentScaleIntegerOnlyY = 254;

constexpr const char *optionSavePathDefaultToken = ":DEFAULT:";

constexpr double minRunSpeed = .05;
constexpr double maxRunSpeed = 20.;

constexpr bool isValidFastSpeed(const auto &v) { return v <= int(maxRunSpeed * 100.) && v > 100; }
constexpr bool isValidSlowSpeed(const auto &v) { return v >= int(minRunSpeed * 100.) && v < 100; }

bool isValidAspectRatio(float val);

constexpr bool isValidFontSize(const auto &v)
{
	return v >= 2000 && v <= 10000;
}

constexpr bool optionContentScaleIsValid(const auto &v)
{
	return v == optionContentScaleIntegerOnly || v == optionContentScaleIntegerOnlyY
		|| (v >= 10 && v <= 200);
}

constexpr bool optionMenuScaleIsValid(const auto &v)
{
	return v >= 50 && v <= 100;
}

constexpr bool isValidFrameInterval(const auto &v)
{
	return v >= 0 && v <= 4;
}

constexpr bool imageEffectPixelFormatIsValid(const auto &v)
{
	switch(v)
	{
		case IG::PixelFmtUnset:
		case IG::PixelFmtRGB565:
		case IG::PixelFmtRGBA8888:
			return true;
		default:
			return false;
	}
}

template<auto max>
constexpr bool isValidWithMax(const auto &v)
{
	return v <= max;
}

template<auto min, auto max>
constexpr bool isValidWithMinMax(const auto &v)
{
	return v >= min && v <= max;
}

constexpr bool windowPixelFormatIsValid(const IG::PixelFormat &v)
{
	switch(v)
	{
		case IG::PixelFmtUnset:
		case IG::PixelFmtRGB565:
		case IG::PixelFmtRGBA8888:
			return true;
		default: return false;
	}
}

constexpr bool renderPixelFormatIsValid(const auto &v)
{
	return windowPixelFormatIsValid(v);
}

}

namespace IG
{

template<>
constexpr bool isValidProperty(const EmuEx::AutoTristate &v) { return unsigned(v) <= 2; }

template<>
constexpr bool isValidProperty(const EmuEx::InEmuTristate &v) { return unsigned(v) <= 2; }

}
