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

extern BasicByteOption optionAutoSaveState;
extern BasicByteOption optionConfirmAutoLoadState;
extern BasicByteOption optionSound;
#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
extern OptionAudioHintPcmMaxBuffers optionSoundBuffers;
#endif
#ifdef CONFIG_AUDIO_OPENSL_ES
extern OptionAudioHintStrictUnderrunCheck optionSoundUnderrunCheck;
#endif
extern OptionSoundRate optionSoundRate;
extern BasicByteOption optionLargeFonts;
extern BasicByteOption optionVibrateOnPush;
extern BasicByteOption optionPauseUnfocused;
extern BasicByteOption optionNotificationIcon;
extern BasicByteOption optionTitleBar;
extern OptionBackNavigation optionBackNavigation;
extern BasicByteOption optionRememberLastMenu;
extern BasicByteOption optionLowProfileOSNav;
extern BasicByteOption optionHideOSNav;
extern BasicByteOption optionIdleDisplayPowerSave;
extern BasicByteOption optionShowMenuIcon;
extern BasicByteOption optionHideStatusBar;
extern OptionSwappedGamepadConfirm optionSwappedGamepadConfirm;

#ifdef CONFIG_BLUETOOTH
extern BasicByteOption optionKeepBluetoothActive;
extern OptionBlueToothScanCache optionBlueToothScanCache;
#endif

extern OptionMethodImgFilter optionImgFilter;
extern OptionAspectRatio optionAspectRatio;


extern OptionMethodOverlayEffect optionOverlayEffect;
extern OptionMethodOverlayEffectLevel optionOverlayEffectLevel;

extern OptionMethodRelPointerDecel optionRelPointerDecel;


extern OptionMethodOrientation optionGameOrientation;
extern OptionMethodOrientation optionMenuOrientation;

extern OptionMethodTouchCtrl optionTouchCtrl;

extern OptionMethodTouchCtrlAlpha optionTouchCtrlAlpha;

bool isValidOption2DO(_2DOrigin val);
bool isValidOption2DOCenterBtn(_2DOrigin val);

extern Option<OptionMethodVar<uint32, optionIsValidWithMax<1400> >, uint16> optionTouchCtrlSize;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<160> >, uint16> optionTouchDpadDeadzone;
extern Option<OptionMethodVar<uint32, optionIsValidWithMinMax<1000,2500> >, uint16> optionTouchDpadDiagonalSensitivity;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<400> >, uint16> optionTouchCtrlBtnSpace;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<5> >, uint16> optionTouchCtrlBtnStagger;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<3> >, uint16> optionTouchCtrlTriggerBtnPos;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraXBtnSize;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSize;
extern Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSizeMultiRow;

extern Option2DOrigin optionTouchCtrlDpadPos;
extern Option2DOrigin optionTouchCtrlFaceBtnPos;
extern Option2DOriginC optionTouchCtrlCenterBtnPos;
extern Option2DOrigin optionTouchCtrlMenuPos;
extern Option2DOrigin optionTouchCtrlFFPos;

bool optionFrameSkipIsValid(uint32 val);
extern Option<OptionMethodVar<uint32, optionFrameSkipIsValid >, uint8> optionFrameSkip;

bool optionImageZoomIsValid(uint32 val);
extern Option<OptionMethodVar<uint32, optionImageZoomIsValid>, uint8> optionImageZoom;

extern OptionDPI optionDPI;

extern OptionRecentGames optionRecentGames;

extern Option<OptionMethodVar<uint32, optionIsValidWithMax<128> >, uint16> optionTouchCtrlImgRes;

#ifdef CONFIG_BASE_ANDROID
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		extern OptionDirectTexture optionDirectTexture;
	#endif
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		extern Option<OptionMethodFunc<bool, Gfx::useAndroidSurfaceTexture, Gfx::setUseAndroidSurfaceTexture>, uint8> optionSurfaceTexture;
		extern Option<OptionMethodVar<int8, optionIsValidWithMinMax<-17, 0> > > optionProcessPriority;
	#endif
	extern Option<OptionMethodRef<template_ntype(glSyncHackEnabled)>, uint8> optionGLSyncHack;
#endif

#ifdef CONFIG_INPUT_ICADE
	extern Option<OptionMethodFunc<bool, Input::iCadeActive, Input::setICadeActive>, uint8> optionICade;
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	extern Option<OptionMethodFunc<bool, Input::eventsUseOSInputMethod, Input::setEventsUseOSInputMethod>, uint8> optionUseOSInputMethod;
#endif

extern Option<OptionMethodFunc<uint, Gfx::dither, Gfx::setDither>, uint8> optionDitherImage;

void initOptions();
