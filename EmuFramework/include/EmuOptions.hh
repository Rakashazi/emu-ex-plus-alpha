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

#include "Option.hh"
#include <config/env.hh>

static BasicByteOption optionAutoSaveState(CFGKEY_AUTO_SAVE_STATE, 1);
BasicByteOption optionSound(CFGKEY_SOUND, 1);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<48000> > > optionSoundRate(CFGKEY_SOUND_RATE,
		(Config::envIsPS3 || Config::envIsLinux) ? 48000 : 44100, Config::envIsPS3);
static BasicByteOption optionLargeFonts(CFGKEY_FONT_Y_PIXELS, Config::envIsWebOS3,
		(Config::envIsWebOS && !Config::envIsWebOS3));
BasicByteOption optionVibrateOnPush(CFGKEY_TOUCH_CONTROL_VIRBRATE, 0,
		!Config::envIsAndroid);
static BasicByteOption optionPauseUnfocused(CFGKEY_PAUSE_UNFOCUSED, 1,
		!(Config::envIsPS3 || Config::envIsLinux || Config::envIsAndroid));
static BasicByteOption optionNotificationIcon(CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid);
static BasicByteOption optionTitleBar(CFGKEY_TITLE_BAR, 1, Config::envIsIOS || Config::envIsWebOS3);
static Option<OptionMethodRef<template_ntype(View::needsBackControl)>, uint8>
	optionBackNavigation(CFGKEY_BACK_NAVIGATION, View::needsBackControlDefault, View::needsBackControlIsConst);
static BasicByteOption optionRememberLastMenu(CFGKEY_REMEMBER_LAST_MENU, 1, 0);
static BasicByteOption optionLowProfileOSNav(CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid);
static BasicByteOption optionHideOSNav(CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid);
static BasicByteOption optionIdleDisplayPowerSave(CFGKEY_IDLE_DISPLAY_POWER_SAVE, 1, !Config::envIsAndroid && !Config::envIsIOS);
static BasicByteOption optionShowMenuIcon(CFGKEY_SHOW_MENU_ICON, Config::envIsIOS || Config::envIsAndroid || Config::envIsWebOS3, Config::envIsPS3);

#ifdef CONFIG_BLUETOOTH
static BasicByteOption optionKeepBluetoothActive(CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0);
#endif

typedef OptionMethodValidatedVar<uint32, GfxBufferImage::isFilterValid> OptionMethodImgFilter;
static Option<OptionMethodImgFilter, uint8> optionImgFilter(CFGKEY_GAME_IMG_FILTER, GfxBufferImage::linear);

static struct OptionAspectRatio : public Option<OptionMethodVar<uint32>, uint8>
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
		io->readVar(&x);
		io->readVar(&y);
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
} optionAspectRatio(0);

typedef OptionMethodValidatedVar<uint8, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL> > OptionMethodOverlayEffect;
static Option<OptionMethodOverlayEffect, uint8> optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0);

typedef OptionMethodValidatedVar<uint8, optionIsValidWithMax<100> > OptionMethodOverlayEffectLevel;
static Option<OptionMethodOverlayEffectLevel, uint8> optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25);

Option<OptionMethodRelPointerDecel> optionRelPointerDecel(CFGKEY_REL_POINTER_DECEL, optionRelPointerDecelMed,
		!Config::envIsAndroid);

bool optionOrientationIsValid(uint32 val)
{
	return val == Gfx::VIEW_ROTATE_AUTO ||
			val == Gfx::VIEW_ROTATE_0 ||
			val == Gfx::VIEW_ROTATE_90 ||
			val == Gfx::VIEW_ROTATE_270;
}
typedef OptionMethodValidatedVar<uint32, optionOrientationIsValid> OptionMethodOrientation;
static Option<OptionMethodOrientation, uint8> optionGameOrientation(CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Config::envIsWebOS ? Gfx::VIEW_ROTATE_90 : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3);

static Option<OptionMethodOrientation, uint8> optionMenuOrientation(CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3);


typedef OptionMethodValidatedVar<uint32, optionIsValidWithMax<2> > OptionMethodTouchCtrl;
static Option<OptionMethodTouchCtrl, uint8> optionTouchCtrl(CFGKEY_TOUCH_CONTROL_DISPLAY,
		(Config::envIsLinux || Config::envIsPS3) ? 0 : 2,
		Config::envIsPS3);

typedef OptionMethodValidatedVar<uint32, optionIsValidWithMax<255> > OptionMethodTouchCtrlAlpha;
static Option<OptionMethodTouchCtrlAlpha, uint8> optionTouchCtrlAlpha(CFGKEY_TOUCH_CONTROL_ALPHA,
		255 * .5,
		Config::envIsPS3);

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<1400> >, uint16> optionTouchCtrlSize
		(CFGKEY_TOUCH_CONTROL_SIZE,
		(Config::envIsWebOS && !Config::envIsWebOS3) ? 800 : Config::envIsWebOS3 ? 1400 : 850,
		Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<160> >, uint16> optionTouchDpadDeadzone
		(CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE,
		135,
		Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMinMax<1000,2500> >, uint16> optionTouchDpadDiagonalSensitivity
		(CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY,
		1750,
		Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<400> >, uint16> optionTouchCtrlBtnSpace
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE,
		200,
		Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<5> >, uint16> optionTouchCtrlBtnStagger
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER,
		1,
		Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<3> >, uint16> optionTouchCtrlTriggerBtnPos
		(CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS,
		0, Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraXBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE,
		200, Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE,
		1000, Config::envIsPS3);
static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSizeMultiRow
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW,
		200, Config::envIsPS3);

Option2DOrigin optionTouchCtrlDpadPos(CFGKEY_TOUCH_CONTROL_DPAD_POS, LB2DO);
Option2DOrigin optionTouchCtrlFaceBtnPos(CFGKEY_TOUCH_CONTROL_FACE_BTN_POS, RB2DO);
static Option2DOriginC optionTouchCtrlCenterBtnPos(CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS, CB2DO);
static Option2DOrigin optionTouchCtrlMenuPos(CFGKEY_TOUCH_CONTROL_MENU_POS,
	#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		NULL2DO
	#else
		RT2DO
	#endif
	);
static Option2DOrigin optionTouchCtrlFFPos(CFGKEY_TOUCH_CONTROL_FF_POS, Config::envIsIOS ? LT2DO : NULL2DO);


bool optionFrameSkipIsValid(uint32 val)
{
	uint limit = (Config::envIsIOS || Config::envIsLinux) ? 4 : 0;
	return val == EmuSystem::optionFrameSkipAuto || val <= limit;
}

static Option<OptionMethodValidatedVar<uint32, optionFrameSkipIsValid >, uint8> optionFrameSkip
		(CFGKEY_FRAME_SKIP,
		#if defined(CONFIG_BASE_IOS)
			#ifdef __ARM_ARCH_6K__
				EmuSystem::optionFrameSkipAuto,
			#else
				0,
			#endif
		#elif defined(CONFIG_BASE_PS3)
			0,
		#else
			EmuSystem::optionFrameSkipAuto,
		#endif
		Config::envIsPS3);

static const uint optionImageZoomIntegerOnly = 255;
bool optionImageZoomIsValid(uint32 val)
{
	return val == optionImageZoomIntegerOnly || val <= 100;
}
static Option<OptionMethodValidatedVar<uint32, optionImageZoomIsValid>, uint8> optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100);

static struct OptionDPI : public Option<OptionMethodVar<uint32> >
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
} optionDPI(0,
#ifdef CONFIG_SUPPORTS_DPI_OVERRIDE
	0
#else
	1
#endif
);

static struct OptionRecentGames : public OptionBase
{
	bool isDefault() const { return recentGameList.size == 0; }
	static const uint16 key = CFGKEY_RECENT_GAMES;

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
			io->readVar(&len);
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
} optionRecentGames;

static Option<OptionMethodValidatedVar<uint32, optionIsValidWithMax<128> >, uint16> optionTouchCtrlImgRes
(CFGKEY_TOUCH_CONTROL_IMG_PIXELS,
	#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
	64,
	#else
	128,
	#endif
	Config::envIsIOS || Config::envIsWebOS3);

#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	static struct OptionDirectTexture : public Option<OptionMethodVar<uint32>, uint8>
	{
		constexpr OptionDirectTexture(): Option<OptionMethodVar<uint32>, uint8>(CFGKEY_DIRECT_TEXTURE, 0) { }
		bool readFromIO(Io *io, uint readSize)
		{
			if(!Option<OptionMethodVar<uint32>, uint8>::readFromIO(io, readSize))
				return 0;
			logMsg("read direct texture option %d", val);
			if(!gfx_androidDirectTextureSupported())
				val = 0;
			gfx_setAndroidDirectTexture(val);
			return 1;
		}
	} optionDirectTexture;

	static Option<OptionMethodRef<template_ntype(glSyncHackEnabled)>, uint8> optionGLSyncHack(CFGKEY_GL_SYNC_HACK, 0);
#endif

#ifdef CONFIG_INPUT_ICADE
	static Option<OptionMethodFunc<fbool, Input::iCadeActive, Input::setICadeActive>, uint8> optionICade(CFGKEY_ICADE, 0);
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	static Option<OptionMethodFunc<fbool, Input::eventsUseOSInputMethod, Input::setEventsUseOSInputMethod>, uint8> optionUseOSInputMethod(CFGKEY_USE_OS_INPUT_METHOD, 1);
#endif

static Option<OptionMethodFunc<uint, Gfx::dither, Gfx::setDither>, uint8> optionDitherImage(CFGKEY_DITHER_IMAGE, 1, !Config::envIsAndroid);
