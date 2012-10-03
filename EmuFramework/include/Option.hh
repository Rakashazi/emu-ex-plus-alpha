/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <io/Io.hh>
#include <bluetooth/sys.hh>
#include <gui/View.hh>
#include <util/2DOrigin.h>
#include <VideoImageOverlay.hh>
#include <EmuSystem.hh>
#include <Recent.hh>
#include <audio/Audio.hh>
#include <util/strings.h>

struct OptionBase
{
	bool isConst = 0;
	constexpr OptionBase() { }
	constexpr OptionBase(bool isConst): isConst(isConst) { }

	virtual bool isDefault() const = 0;
	virtual uint ioSize() = 0;
	virtual bool writeToIO(Io *io) = 0;
};

template <class T>
bool OptionMethodIsAlwaysValid(T)
{
	return 1;
}

template <class T, bool (&VALID)(T v)>
struct OptionMethodBase
{
	constexpr OptionMethodBase() { }
	bool isValidVal(T v)
	{
		return VALID(v);
	}
};

template <class T, T (&GET)(), void (&SET)(T), bool (&VALID)(T v) = OptionMethodIsAlwaysValid>
struct OptionMethodFunc : public OptionMethodBase<T, VALID>
{
	constexpr OptionMethodFunc() { }
	constexpr OptionMethodFunc(T init) { }
	T get() const { return GET(); }
	void set(T v) { SET(v); }
};

template <class T, T &val, bool (&VALID)(T v) = OptionMethodIsAlwaysValid>
struct OptionMethodRef : public OptionMethodBase<T, VALID>
{
	constexpr OptionMethodRef() { }
	constexpr OptionMethodRef(T init) { }
	T get() const { return val; }
	void set(T v) { val = v; }
};

template <class T, bool (&VALID)(T v) = OptionMethodIsAlwaysValid>
struct OptionMethodVar : public OptionMethodBase<T, VALID>
{
	constexpr OptionMethodVar() { }
	constexpr OptionMethodVar(T init): val(init) { }
	T val;
	T get() const { return val; }
	void set(T v) { val = v; }
};

template <class V, class SERIALIZED_T = typeof(V().get())>
struct Option : public OptionBase, public V
{
private:
	const uint16 KEY;
public:
	typedef typeof(V().get()) T;
	T defaultVal;

	constexpr Option(uint16 key, T defaultVal = 0, bool isConst = 0): OptionBase(isConst), V(defaultVal), KEY(key), defaultVal(defaultVal) { }

	Option & operator = (T other)
	{
		if(!isConst)
			V::set(other);
		return *this;
	}

	bool operator ==(T const& rhs) const { return V::get() == rhs; }
	bool operator !=(T const& rhs) const { return V::get() != rhs; }

	bool isDefault() const { return V::get() == defaultVal; }
	void initDefault(T val) { defaultVal = val; V::set(val); }

	operator T() const
	{
		return V::get();
	}

	bool writeToIO(Io *io)
	{
		logMsg("writing option key %u after size %u", KEY, ioSize());
		io->writeVar(KEY);
		io->writeVar((SERIALIZED_T)V::get());
		return 1;
	}

	bool writeWithKeyIfNotDefault(Io *io)
	{
		if(!isDefault())
		{
			io->writeVar((uint16)ioSize());
			writeToIO(io);
		}
		return 1;
	}

	bool readFromIO(Io *io, uint readSize)
	{
		if(isConst || readSize != sizeof(SERIALIZED_T))
		{
			logMsg("skipping %d byte option value, expected %d", readSize, (int)sizeof(SERIALIZED_T));
			io->seekRel(readSize);
			return 0;
		}

		SERIALIZED_T x;
		if(io->readVar(x) != OK)
		{
			logErr("error reading option from io");
			return 0;
		}
		if(V::isValidVal(x))
			V::set(x);
		else
			logMsg("skipped invalid option value");
		return 1;
	}

	uint ioSize()
	{
		return sizeof(typeof(KEY)) + sizeof(SERIALIZED_T);
	}
};

typedef Option<OptionMethodVar<uint8>, uint8> BasicByteOption;

template <uint16 KEY>
struct PathOption : public OptionBase
{
	char *val;
	uint strSize;
	const char *defaultVal;

	constexpr PathOption(char *val, uint size, const char *defaultVal): val(val), strSize(size), defaultVal(defaultVal) { }
	template <size_t S>
	constexpr PathOption(char (&val)[S], const char *defaultVal): PathOption(val, S, defaultVal) { }

	bool isDefault() const { return string_equal(val, defaultVal); }

	operator char *() const
	{
		return val;
	}

	bool writeToIO(Io *io)
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
		io->writeVar((uint16)(2 + len));
		io->writeVar((uint16)KEY);
		io->fwrite(val, len, 1);
		return 1;
	}

	bool readFromIO(Io *io, uint readSize)
	{
		if(readSize > strSize-1)
		{
			logMsg("skipping %d byte string option value, max is %d", readSize, strSize-1);
			io->seekRel(readSize);
			return 0;
		}

		io->read(val, readSize);
		val[readSize] = 0;
		logMsg("read path option %s", val);
		return 1;
	}

	uint ioSize()
	{
		return sizeof(KEY) + strlen(val);
	}
};

template<uint MAX, class T>
bool optionIsValidWithMax(T val)
{
	return val <= MAX;
}

template<uint MIN, uint MAX, class T>
bool optionIsValidWithMinMax(T val)
{
	return val >= MIN && val <= MAX;
}

bool optionOrientationIsValid(uint32 val);
using OptionMethodOrientation = Option<OptionMethodVar<uint32, optionOrientationIsValid>, uint8>;

bool isValidOption2DO(_2DOrigin val);
bool isValidOption2DOCenterBtn(_2DOrigin val);
using Option2DOrigin = Option<OptionMethodVar<_2DOrigin, isValidOption2DO>, uint8>;
using Option2DOriginC = Option<OptionMethodVar<_2DOrigin, isValidOption2DOCenterBtn>, uint8>;
static const uint optionRelPointerDecelLow = 500, optionRelPointerDecelMed = 250, optionRelPointerDecelHigh = 125;
using OptionMethodRelPointerDecel = Option<OptionMethodVar<uint32, optionIsValidWithMax<optionRelPointerDecelHigh> > >;
using OptionMethodTouchCtrlAlpha = Option<OptionMethodVar<uint32, optionIsValidWithMax<255> >, uint8>;
using OptionSoundRate = Option<OptionMethodVar<uint32, optionIsValidWithMax<48000> > >;
using OptionBackNavigation = Option<OptionMethodRef<template_ntype(View::needsBackControl)>, uint8>;
using OptionSwappedGamepadConfirm = Option<OptionMethodRef<bool, input_swappedGamepadConfirm>, uint8>;
using OptionMethodImgFilter = Option<OptionMethodVar<uint32, GfxBufferImage::isFilterValid>, uint8>;
using OptionMethodOverlayEffect = Option<OptionMethodVar<uint8, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL> >, uint8>;
using OptionMethodOverlayEffectLevel = Option<OptionMethodVar<uint8, optionIsValidWithMax<100> >, uint8>;
using OptionMethodTouchCtrl = Option<OptionMethodVar<uint32, optionIsValidWithMax<2> >, uint8>;

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
using OptionAudioHintPcmMaxBuffers = Option<OptionMethodFunc<uint, Audio::hintPcmMaxBuffers, Audio::setHintPcmMaxBuffers, optionIsValidWithMinMax<3, 12, uint> >, uint8>;
#endif
#ifdef CONFIG_AUDIO_OPENSL_ES
using OptionAudioHintStrictUnderrunCheck = Option<OptionMethodFunc<bool, Audio::hintStrictUnderrunCheck, Audio::setHintStrictUnderrunCheck>, uint8>;
#endif
#ifdef CONFIG_BLUETOOTH
using OptionBlueToothScanCache = Option<OptionMethodFunc<bool, BluetoothAdapter::scanCacheUsage, BluetoothAdapter::setScanCacheUsage>, uint8>;
#endif

enum { CFGKEY_SOUND = 0, CFGKEY_TOUCH_CONTROL_DISPLAY = 1,
	CFGKEY_AUTO_SAVE_STATE = 2, CFGKEY_LAST_DIR = 3, CFGKEY_TOUCH_CONTROL_VIRBRATE = 4,
	CFGKEY_FRAME_SKIP = 5, CFGKEY_FONT_Y_PIXELS = 6, CFGKEY_GAME_ORIENTATION = 7,
	CFGKEY_GAME_ASPECT_RATIO = 8,
	CFGKEY_TOUCH_CONTROL_ALPHA = 9, CFGKEY_TOUCH_CONTROL_SIZE = 10,
	CFGKEY_TOUCH_CONTROL_DPAD_POS = 11, CFGKEY_TOUCH_CONTROL_FACE_BTN_POS = 12,
	CFGKEY_GAME_IMG_FILTER = 13, CFGKEY_REL_POINTER_DECEL = 14,
	CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE = 15, CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER = 16,
	CFGKEY_DPI = 17, CFGKEY_DIRECT_TEXTURE = 18, CFGKEY_MENU_ORIENTATION = 19,
	CFGKEY_SWAPPED_GAMEPAD_CONFIM = 20, CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE = 21,
	CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS = 22, CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS = 23,
	CFGKEY_TOUCH_CONTROL_MENU_POS = 24, CFGKEY_TOUCH_CONTROL_FF_POS = 25,
	CFGKEY_RECENT_GAMES = 26, CFGKEY_GL_SYNC_HACK = 27, CFGKEY_PAUSE_UNFOCUSED = 28,
	CFGKEY_IMAGE_ZOOM = 29, CFGKEY_TOUCH_CONTROL_IMG_PIXELS = 30, CFGKEY_SOUND_RATE = 31,
	CFGKEY_NOTIFICATION_ICON = 32, CFGKEY_ICADE = 33, CFGKEY_TITLE_BAR = 34,
	CFGKEY_BACK_NAVIGATION = 35, CFGKEY_REMEMBER_LAST_MENU = 36,
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

	CFGKEY_KEY_LOAD_GAME = 100, CFGKEY_KEY_OPEN_MENU = 101,
	CFGKEY_KEY_SAVE_STATE = 102, CFGKEY_KEY_LOAD_STATE = 103,
	CFGKEY_KEY_FAST_FORWARD = 104, CFGKEY_KEY_SCREENSHOT = 105,
	CFGKEY_KEY_EXIT = 106,

	// 256+ is reserved
};

struct OptionAspectRatio : public Option<OptionMethodVar<uint32>, uint8>
{
	constexpr OptionAspectRatio(T defaultVal = 0, bool isConst = 0): Option<OptionMethodVar<uint32>, uint8>(CFGKEY_GAME_ASPECT_RATIO, defaultVal, isConst) { }

	uint ioSize()
	{
		return 2 + 2;
	}

	bool writeToIO(Io *io)
	{
		io->writeVar((uint16)CFGKEY_GAME_ASPECT_RATIO);
		uint x = EmuSystem::aspectRatioX, y = EmuSystem::aspectRatioY;
		if(val == 1)
		{
			x = 1; y = 1;
		}
		else if(val == 2)
		{
			x = 0; y = 0;
		}
		logMsg("writing aspect ratio config %u:%u", x, y);
		io->writeVar((uint8)x);
		io->writeVar((uint8)y);
		return 1;
	}

	bool readFromIO(Io *io, uint readSize)
	{
		if(isConst || readSize != 2)
		{
			logMsg("skipping %d byte option value, expected %d", readSize, 2);
			io->seekRel(readSize);
			return 0;
		}

		uint8 x, y;
		io->readVar(x);
		io->readVar(y);
		logMsg("read aspect ratio config %u,%u", x, y);
		val = 0;
		if(x == 1 && y == 1)
		{
			val = 1;
		}
		else if(x == 0 && y == 0)
		{
			val = 2;
		}
		return 1;
	}
};

static const uint optionImageZoomIntegerOnly = 255;

struct OptionDPI : public Option<OptionMethodVar<uint32> >
{
	constexpr OptionDPI(T defaultVal = 0, bool isConst = 0): Option<OptionMethodVar<uint32> >(CFGKEY_DPI, defaultVal, isConst) { }

	bool writeToIO(Io *io)
	{
		logMsg("writing dpi config %u", val * 100);
		io->writeVar((uint16)CFGKEY_DPI);
		io->writeVar((uint32)(val * 100));
		return 1;
	}

	bool readFromIO(Io *io, uint readSize)
	{
		bool ret = Option<OptionMethodVar<uint32> >::readFromIO(io, readSize);
		if(ret)
		{
			logMsg("read dpi config %u", val);
			val /= 100;
			if(val != 0 && (val < 96 || val > 320))
				val = defaultVal;
		}
		return ret;
	}
};

struct OptionRecentGames : public OptionBase
{
	bool isDefault() const { return recentGameList.size == 0; }
	const uint16 key = CFGKEY_RECENT_GAMES;

	bool writeToIO(Io *io)
	{
		logMsg("writing recent list");
		io->writeVar(key);
		forEachInDLList(&recentGameList, e)
		{
			uint len = strlen(e.path);
			io->writeVar((uint16)len);
			io->fwrite(e.path, len, 1);
		}
		return 1;
	}

	bool readFromIO(Io *io, uint readSize)
	{
		while(readSize && recentGameList.size < 10)
		{
			if(readSize < 2)
			{
				logMsg("expected string length but only %d bytes left", readSize);
				break;
			}

			uint16 len;
			io->readVar(len);
			readSize -= 2;

			if(len > readSize)
			{
				logMsg("string length %d longer than %d bytes left", len, readSize);
				break;
			}

			RecentGameInfo info;
			io->read(info.path, len);
			info.path[len] = 0;
			readSize -= len;
			char *baseName = baseNamePos(info.path);
			string_copyUpToLastCharInstance(info.name, baseName, '.');
			//logMsg("adding game to recent list: %s, name: %s", info.path, info.name);
			recentGameList.addToEnd(info);
		}

		if(readSize)
		{
			logMsg("skipping excess %d bytes", readSize);
			io->seekRel(readSize);
		}

		return 1;
	}

	uint ioSize()
	{
		uint strSizes = 0;
		forEachInDLList(&recentGameList, e)
		{
			strSizes += 2;
			strSizes += strlen(e.path);
		}
		return sizeof(key) + strSizes;
	}
};

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	struct OptionDirectTexture : public Option<OptionMethodVar<uint32>, uint8>
	{
		constexpr OptionDirectTexture(): Option<OptionMethodVar<uint32>, uint8>(CFGKEY_DIRECT_TEXTURE, 0) { }
		bool readFromIO(Io *io, uint readSize)
		{
			if(!Option<OptionMethodVar<uint32>, uint8>::readFromIO(io, readSize))
				return 0;
			logMsg("read direct texture option %d", val);
			if(!Gfx::supportsAndroidDirectTexture())
				val = 0;
			Gfx::setUseAndroidDirectTexture(val);
			return 1;
		}
	};
#endif
