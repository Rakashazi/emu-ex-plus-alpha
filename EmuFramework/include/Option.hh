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

#include <imagine/io/Io.hh>
#include <imagine/bluetooth/sys.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/2DOrigin.h>
#include <VideoImageOverlay.hh>
#include <EmuSystem.hh>
#include <Recent.hh>
#include <imagine/audio/Audio.hh>
#include <imagine/util/strings.h>
#include <imagine/util/Rational.hh>

struct OptionBase
{
	bool isConst = 0;

	constexpr OptionBase() {}
	constexpr OptionBase(bool isConst): isConst(isConst) {}
	virtual bool isDefault() const = 0;
	virtual uint ioSize() = 0;
	virtual bool writeToIO(Io *io) = 0;
};

template <class T>
bool OptionMethodIsAlwaysValid(T)
{
	return 1;
}

template <class T>
struct OptionMethodBase
{
	constexpr OptionMethodBase(bool (&validator)(T v)): validator(validator) {}
	bool (&validator)(T v);
	bool isValidVal(T v)
	{
		return validator(v);
	}
};

template <class T, T (&GET)(), void (&SET)(T)>
struct OptionMethodFunc : public OptionMethodBase<T>
{
	constexpr OptionMethodFunc(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodFunc(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	T get() const { return GET(); }
	void set(T v) { SET(v); }
};

template <class T, T &val>
struct OptionMethodRef : public OptionMethodBase<T>
{
	constexpr OptionMethodRef(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodRef(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	T get() const { return val; }
	void set(T v) { val = v; }
};

template <class T>
struct OptionMethodVar : public OptionMethodBase<T>
{
	constexpr OptionMethodVar(bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator) {}
	constexpr OptionMethodVar(T init, bool (&validator)(T v) = OptionMethodIsAlwaysValid): OptionMethodBase<T>(validator), val(init) {}
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

	constexpr Option(uint16 key, T defaultVal = 0, bool isConst = 0, bool (&validator)(T v) = OptionMethodIsAlwaysValid):
		OptionBase(isConst), V(defaultVal, validator), KEY(key), defaultVal(defaultVal)
	{}

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
	void reset() { V::set(defaultVal); }

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

	bool readFromIO(Io &io, uint readSize)
	{
		if(isConst || readSize != sizeof(SERIALIZED_T))
		{
			if(isConst)
				logMsg("skipping const option value");
			else
				logMsg("skipping %d byte option value, expected %d", readSize, (int)sizeof(SERIALIZED_T));
			return 0;
		}

		SERIALIZED_T x;
		if(io.readVar(x) != OK)
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

struct PathOption : public OptionBase
{
	char *val;
	uint strSize;
	const char *defaultVal;
	const uint16 KEY;

	constexpr PathOption(uint16 key, char *val, uint size, const char *defaultVal): val(val), strSize(size), defaultVal(defaultVal), KEY(key) {}
	template <size_t S>
	constexpr PathOption(uint16 key, char (&val)[S], const char *defaultVal): PathOption(key, val, S, defaultVal) {}

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

	bool readFromIO(Io &io, uint readSize)
	{
		if(readSize > strSize-1)
		{
			logMsg("skipping %d byte string option value, max is %d", readSize, strSize-1);
			return 0;
		}

		io.read(val, readSize);
		val[readSize] = 0;
		logMsg("read path option %s", val);
		return 1;
	}

	uint ioSize()
	{
		return sizeof(KEY) + strlen(val);
	}
};

template<int MAX, class T>
bool optionIsValidWithMax(T val)
{
	return val <= MAX;
}

template<int MIN, int MAX, class T>
bool optionIsValidWithMinMax(T val)
{
	return val >= MIN && val <= MAX;
}

using SByte1Option = Option<OptionMethodVar<sint8>, sint8>;
using Byte1Option = Option<OptionMethodVar<uint8>, uint8>;
using Byte2Option = Option<OptionMethodVar<uint16>, uint16>;
using Byte4s2Option = Option<OptionMethodVar<uint32>, uint16>;
using Byte4Option = Option<OptionMethodVar<uint32>, uint32>;
using Byte4s1Option = Option<OptionMethodVar<uint32>, uint8>;

using OptionBackNavigation = Option<OptionMethodRef<template_ntype(View::needsBackControl)>, uint8>;
using OptionSwappedGamepadConfirm = Option<OptionMethodRef<bool, Input::swappedGamepadConfirm>, uint8>;

bool vControllerUseScaledCoordinates();
void setVControllerUseScaledCoordinates(bool on);
using OptionTouchCtrlScaledCoordinates = Option<OptionMethodFunc<bool, vControllerUseScaledCoordinates, setVControllerUseScaledCoordinates>, uint8>;

#ifdef CONFIG_AUDIO_OPENSL_ES
using OptionAudioHintStrictUnderrunCheck = Option<OptionMethodFunc<bool, Audio::hintStrictUnderrunCheck, Audio::setHintStrictUnderrunCheck>, uint8>;
#endif
#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
using OptionBlueToothScanCache = Option<OptionMethodFunc<bool, BluetoothAdapter::scanCacheUsage, BluetoothAdapter::setScanCacheUsage>, uint8>;
#endif

// TODO: recycle obsolete enums
enum { CFGKEY_SOUND = 0, CFGKEY_TOUCH_CONTROL_DISPLAY = 1,
	CFGKEY_AUTO_SAVE_STATE = 2, CFGKEY_LAST_DIR = 3, CFGKEY_TOUCH_CONTROL_VIRBRATE = 4,
	CFGKEY_FRAME_SKIP = 5, CFGKEY_FONT_Y_SIZE = 6, CFGKEY_GAME_ORIENTATION = 7,
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
	CFGKEY_SURFACE_TEXTURE = 55, CFGKEY_PROCESS_PRIORITY = 56,
	CFGKEY_SAVE_PATH = 57, CFGKEY_BEST_COLOR_MODE_HINT = 58,
	CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES = 59,
	CFGKEY_INPUT_KEY_CONFIGS = 60, CFGKEY_INPUT_DEVICE_CONFIGS = 61,
	CFGKEY_CONFIRM_OVERWRITE_STATE = 62, CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE = 63,
	CFGKEY_AUDIO_SOLO_MIX = 64, CFGKEY_TOUCH_CONTROL_SHOW_ON_TOUCH = 65,
	CFGKEY_TOUCH_CONTROL_SCALED_COORDINATES = 66, CFGKEY_VIEWPORT_ZOOM = 67,
	CFGKEY_VCONTROLLER_LAYOUT_POS = 68, CFGKEY_MOGA_INPUT_SYSTEM = 69,
	CFGKEY_FAST_FORWARD_SPEED = 70, CFGKEY_SHOW_BUNDLED_GAMES = 71,
	CFGKEY_IMAGE_EFFECT = 72
	// 256+ is reserved
};

struct OptionAspectRatio : public Option<OptionMethodVar<IG::Point2D<uint> > >
{
	constexpr OptionAspectRatio(T defaultVal, bool isConst = 0): Option<OptionMethodVar<IG::Point2D<uint> > >(CFGKEY_GAME_ASPECT_RATIO, defaultVal, isConst) {}

	uint ioSize()
	{
		return 2 + 2;
	}

	bool writeToIO(Io *io)
	{
		io->writeVar((uint16)CFGKEY_GAME_ASPECT_RATIO);
		logMsg("writing aspect ratio config %u:%u", val.x, val.y);
		io->writeVar((uint8)val.x);
		io->writeVar((uint8)val.y);
		return 1;
	}

	bool readFromIO(Io &io, uint readSize)
	{
		if(isConst || readSize != 2)
		{
			logMsg("skipping %d byte option value, expected %d", readSize, 2);
			return 0;
		}

		uint8 x, y;
		io.readVar(x);
		io.readVar(y);
		logMsg("read aspect ratio config %u,%u", x, y);
		if(y == 0)
			y = 1;
		val = Rational::make<uint>(x, y);
		return 1;
	}
};

struct OptionDPI : public Option<OptionMethodVar<uint32> >
{
	constexpr OptionDPI(T defaultVal = 0, bool isConst = 0): Option<OptionMethodVar<uint32> >(CFGKEY_DPI, defaultVal, isConst) {}

	bool writeToIO(Io *io)
	{
		logMsg("writing dpi config %u", val * 100);
		io->writeVar((uint16)CFGKEY_DPI);
		io->writeVar((uint32)(val * 100));
		return 1;
	}

	bool readFromIO(Io &io, uint readSize)
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
	bool isDefault() const { return recentGameList.size() == 0; }
	const uint16 key = CFGKEY_RECENT_GAMES;

	bool writeToIO(Io *io)
	{
		logMsg("writing recent list");
		io->writeVar(key);
		for(auto &e : recentGameList)
		{
			uint len = strlen(e.path);
			io->writeVar((uint16)len);
			io->fwrite(e.path, len, 1);
		}
		return 1;
	}

	bool readFromIO(Io &io, uint readSize_)
	{
		int readSize = readSize_;
		while(readSize && !recentGameList.isFull())
		{
			if(readSize < 2)
			{
				logMsg("expected string length but only %d bytes left", readSize);
				break;
			}

			uint16 len;
			io.readVar(len);
			readSize -= 2;

			if(len > readSize)
			{
				logMsg("string length %d longer than %d bytes left", len, readSize);
				break;
			}

			RecentGameInfo info;
			io.read(info.path, len);
			info.path[len] = 0;
			readSize -= len;
			FsSys::cPath basenameTemp;
			string_copyUpToLastCharInstance(info.name, string_basename(info.path, basenameTemp), '.');
			//logMsg("adding game to recent list: %s, name: %s", info.path, info.name);
			recentGameList.push_back(info);
		}

		if(readSize)
		{
			logMsg("skipping excess %d bytes", readSize);
		}

		return 1;
	}

	uint ioSize()
	{
		uint strSizes = 0;
		for(auto &e : recentGameList)
		{
			strSizes += 2;
			strSizes += strlen(e.path);
		}
		return sizeof(key) + strSizes;
	}
};

struct OptionVControllerLayoutPosition : public OptionBase
{
	const uint16 key = CFGKEY_VCONTROLLER_LAYOUT_POS;

	bool isDefault() const override;
	bool writeToIO(Io *io) override;
	bool readFromIO(Io &io, uint readSize_);
	uint ioSize() override;
};
