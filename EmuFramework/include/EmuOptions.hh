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
#include <bluetooth/BluetoothAdapter.hh>
#include <audio/Audio.hh>

extern Byte1Option optionAutoSaveState;
extern Byte1Option optionConfirmAutoLoadState;
extern Byte1Option optionSound;
#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
extern OptionAudioHintPcmMaxBuffers optionSoundBuffers;
#endif
#ifdef CONFIG_AUDIO_OPENSL_ES
extern OptionAudioHintStrictUnderrunCheck optionSoundUnderrunCheck;
#endif
extern Byte4Option optionSoundRate;
extern Byte1Option optionLargeFonts;
extern Byte1Option optionVibrateOnPush;
extern Byte1Option optionPauseUnfocused;
extern Byte1Option optionNotificationIcon;
extern Byte1Option optionTitleBar;
extern OptionBackNavigation optionBackNavigation;
extern Byte1Option optionRememberLastMenu;
extern Byte1Option optionLowProfileOSNav;
extern Byte1Option optionHideOSNav;
extern Byte1Option optionIdleDisplayPowerSave;
extern Byte1Option optionShowMenuIcon;
extern Byte1Option optionHideStatusBar;
extern OptionSwappedGamepadConfirm optionSwappedGamepadConfirm;
extern Byte1Option optionConfirmOverwriteState;
#ifdef INPUT_HAS_SYSTEM_DEVICE_HOTSWAP
extern Byte1Option optionNotifyInputDeviceChange;
#endif

#ifdef CONFIG_BLUETOOTH
extern Byte1Option optionKeepBluetoothActive;
extern OptionBlueToothScanCache optionBlueToothScanCache;
#endif

extern Byte4s1Option optionImgFilter;
extern OptionAspectRatio optionAspectRatio;


extern Byte1Option optionOverlayEffect;
extern Byte1Option optionOverlayEffectLevel;

static const uint optionRelPointerDecelLow = 500, optionRelPointerDecelMed = 250, optionRelPointerDecelHigh = 125;
extern Byte4Option optionRelPointerDecel;

extern Byte4s1Option optionGameOrientation;
extern Byte4s1Option optionMenuOrientation;

extern Byte1Option optionTouchCtrl;

extern Byte1Option optionTouchCtrlAlpha;

extern Byte4s2Option optionTouchCtrlSize;
extern Byte4s2Option optionTouchDpadDeadzone;
extern Byte4s2Option optionTouchDpadDiagonalSensitivity;
extern Byte4s2Option optionTouchCtrlBtnSpace;
extern Byte4s2Option optionTouchCtrlBtnStagger;
extern Byte4s2Option optionTouchCtrlTriggerBtnPos;
extern Byte4s2Option optionTouchCtrlExtraXBtnSize;
extern Byte4s2Option optionTouchCtrlExtraYBtnSize;
extern Byte4s2Option optionTouchCtrlExtraYBtnSizeMultiRow;
extern Byte1Option optionTouchCtrlBoundingBoxes;

extern Option2DOrigin optionTouchCtrlDpadPos;
extern Option2DOrigin optionTouchCtrlFaceBtnPos;
extern Option2DOrigin optionTouchCtrlCenterBtnPos;
extern Option2DOrigin optionTouchCtrlMenuPos;
extern Option2DOrigin optionTouchCtrlFFPos;

extern Byte1Option optionFrameSkip;

static const uint optionImageZoomIntegerOnly = 255, optionImageZoomIntegerOnlyY = 254;
extern Byte1Option optionImageZoom;

extern OptionDPI optionDPI;

extern OptionRecentGames optionRecentGames;

extern Byte4s2Option optionTouchCtrlImgRes;

#ifdef CONFIG_BASE_ANDROID
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		static const uint8 OPTION_DIRECT_TEXTURE_UNSET = 2;
		extern Byte1Option optionDirectTexture;
	#endif
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		static const uint8 OPTION_SURFACE_TEXTURE_UNSET = 2;
		extern Byte1Option optionSurfaceTexture;
		extern SByte1Option optionProcessPriority;
	#endif
	#if defined CONFIG_INPUT_ANDROID
		extern Option<OptionMethodFunc<bool, Input::eventsUseOSInputMethod, Input::setEventsUseOSInputMethod>, uint8> optionUseOSInputMethod;
	#endif
	extern Option<OptionMethodRef<template_ntype(glSyncHackEnabled)>, uint8> optionGLSyncHack;
#endif

extern Byte1Option optionDitherImage;

#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID) || defined (CONFIG_BASE_IOS)
	#define USE_BEST_COLOR_MODE_OPTION
	extern Byte1Option optionBestColorModeHint;
#endif

extern PathOption optionSavePath;

void initOptions();
