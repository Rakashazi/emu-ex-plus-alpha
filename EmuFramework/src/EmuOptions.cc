#include <EmuOptions.hh>
#include <EmuSystem.hh>
#include "VController.hh"
extern SysVController vController;

bool optionOrientationIsValid(uint32 val)
{
	return val == Gfx::VIEW_ROTATE_AUTO ||
			val == Gfx::VIEW_ROTATE_0 ||
			val == Gfx::VIEW_ROTATE_90 ||
			val == Gfx::VIEW_ROTATE_180 ||
			val == Gfx::VIEW_ROTATE_270;
}

Byte1Option optionAutoSaveState(CFGKEY_AUTO_SAVE_STATE, 1);
Byte1Option optionConfirmAutoLoadState(CFGKEY_CONFIRM_AUTO_LOAD_STATE, 1);
Byte1Option optionSound(CFGKEY_SOUND, 1);

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
OptionAudioHintPcmMaxBuffers optionSoundBuffers(CFGKEY_SOUND_BUFFERS, Config::envIsAndroid ? 10 : 8, 0, optionIsValidWithMinMax<3, 12, uint>);
#endif

#ifdef CONFIG_AUDIO_OPENSL_ES
OptionAudioHintStrictUnderrunCheck optionSoundUnderrunCheck(CFGKEY_SOUND_UNDERRUN_CHECK, 1);
#endif

Byte4Option optionSoundRate(CFGKEY_SOUND_RATE,
	(Config::envIsPS3 || Config::envIsLinux) ? 48000 : 44100, Config::envIsPS3, optionIsValidWithMax<48000>);

Byte1Option optionLargeFonts(CFGKEY_FONT_Y_PIXELS, Config::envIsWebOS3,
	(Config::envIsWebOS && !Config::envIsWebOS3));

Byte1Option optionVibrateOnPush(CFGKEY_TOUCH_CONTROL_VIRBRATE, 0, !Config::envIsAndroid);

Byte1Option optionPauseUnfocused(CFGKEY_PAUSE_UNFOCUSED, 1,
	!(Config::envIsPS3 || Config::envIsLinux || Config::envIsAndroid));

Byte1Option optionNotificationIcon(CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid);
Byte1Option optionTitleBar(CFGKEY_TITLE_BAR, 1, Config::envIsIOS || Config::envIsWebOS3);

OptionBackNavigation
	optionBackNavigation(CFGKEY_BACK_NAVIGATION, View::needsBackControlDefault, View::needsBackControlIsConst);

Byte1Option optionRememberLastMenu(CFGKEY_REMEMBER_LAST_MENU, 1, 0);
Byte1Option optionLowProfileOSNav(CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid);
Byte1Option optionHideOSNav(CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid);
Byte1Option optionIdleDisplayPowerSave(CFGKEY_IDLE_DISPLAY_POWER_SAVE, 1, !Config::envIsAndroid && !Config::envIsIOS);
Byte1Option optionShowMenuIcon(CFGKEY_SHOW_MENU_ICON, Config::envIsIOS || Config::envIsAndroid || Config::envIsWebOS3, Config::envIsPS3);
Byte1Option optionHideStatusBar(CFGKEY_HIDE_STATUS_BAR, 2, !Config::envIsAndroid && !Config::envIsIOS);

OptionSwappedGamepadConfirm optionSwappedGamepadConfirm(CFGKEY_SWAPPED_GAMEPAD_CONFIM, 0);

#ifdef CONFIG_BLUETOOTH
Byte1Option optionKeepBluetoothActive(CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0, !Config::BASE_CAN_BACKGROUND_APP);
OptionBlueToothScanCache optionBlueToothScanCache(CFGKEY_BLUETOOTH_SCAN_CACHE, 1);
#endif

OptionAspectRatio optionAspectRatio(0);

Byte4s1Option optionImgFilter(CFGKEY_GAME_IMG_FILTER, Gfx::BufferImage::linear, 0, Gfx::BufferImage::isFilterValid);

Byte1Option optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0, 0, optionIsValidWithMax<VideoImageOverlay::MAX_EFFECT_VAL>);
Byte1Option optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25, 0, optionIsValidWithMax<100>);

Byte4Option optionRelPointerDecel(CFGKEY_REL_POINTER_DECEL, optionRelPointerDecelMed,
		!Config::envIsAndroid, optionIsValidWithMax<optionRelPointerDecelHigh>);

Byte4s1Option optionGameOrientation(CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Config::envIsWebOS ? Gfx::VIEW_ROTATE_90 : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3, optionOrientationIsValid);

Byte4s1Option optionMenuOrientation(CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3, optionOrientationIsValid);

Byte1Option optionTouchCtrl(CFGKEY_TOUCH_CONTROL_DISPLAY,
		(Config::envIsLinux || Config::envIsPS3) ? 0 : 2,
		Config::envIsPS3, optionIsValidWithMax<2>);

Byte1Option optionTouchCtrlAlpha(CFGKEY_TOUCH_CONTROL_ALPHA,
		255 * .5,
		Config::envIsPS3/*, optionIsValidWithMax<255>*/);

Byte4s2Option optionTouchCtrlSize
		(CFGKEY_TOUCH_CONTROL_SIZE,
		(Config::envIsWebOS && !Config::envIsWebOS3) ? 800 : Config::envIsWebOS3 ? 1400 : 850,
		Config::envIsPS3, optionIsValidWithMax<1400>);
Byte4s2Option optionTouchDpadDeadzone
		(CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE,
		135,
		Config::envIsPS3, optionIsValidWithMax<160>);
Byte4s2Option optionTouchDpadDiagonalSensitivity
		(CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY,
		1750,
		Config::envIsPS3, optionIsValidWithMinMax<1000,2500>);
Byte4s2Option optionTouchCtrlBtnSpace
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE,
		200,
		Config::envIsPS3, optionIsValidWithMax<400>);
Byte4s2Option optionTouchCtrlBtnStagger
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER,
		1,
		Config::envIsPS3, optionIsValidWithMax<5>);
Byte4s2Option optionTouchCtrlTriggerBtnPos
		(CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS,
		TRIGGERS_SPLIT, Config::envIsPS3, optionIsValidWithMax<3>);
Byte4s2Option optionTouchCtrlExtraXBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE,
		200, Config::envIsPS3, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE,
		1000, Config::envIsPS3, optionIsValidWithMax<1000>);
Byte4s2Option optionTouchCtrlExtraYBtnSizeMultiRow
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW,
		200, Config::envIsPS3, optionIsValidWithMax<1000>);

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

Option2DOrigin optionTouchCtrlDpadPos(CFGKEY_TOUCH_CONTROL_DPAD_POS, LB2DO, 0, isValidOption2DO);
Option2DOrigin optionTouchCtrlFaceBtnPos(CFGKEY_TOUCH_CONTROL_FACE_BTN_POS, RB2DO, 0, isValidOption2DO);
Option2DOrigin optionTouchCtrlCenterBtnPos(CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS, CB2DO, 0, isValidOption2DOCenterBtn);
Option2DOrigin optionTouchCtrlMenuPos(CFGKEY_TOUCH_CONTROL_MENU_POS,
	#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		NULL2DO
	#else
		RT2DO
	#endif
	, 0, isValidOption2DO);
Option2DOrigin optionTouchCtrlFFPos(CFGKEY_TOUCH_CONTROL_FF_POS, Config::envIsIOS ? LT2DO : NULL2DO, 0, isValidOption2DO);

Byte1Option optionTouchCtrlBoundingBoxes(CFGKEY_TOUCH_CONTROL_BOUNDING_BOXES, 0);

bool optionFrameSkipIsValid(uint8 val)
{
	uint limit = (Config::envIsIOS || Config::envIsLinux) ? 4 : 0;
	return val == EmuSystem::optionFrameSkipAuto || val <= limit;
}

Byte1Option optionFrameSkip
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
		Config::envIsPS3, optionFrameSkipIsValid);

bool optionImageZoomIsValid(uint8 val)
{
	return val == optionImageZoomIntegerOnly || val <= 100;
}
Byte1Option optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100, 0, optionImageZoomIsValid);

OptionDPI optionDPI(0,
#ifdef CONFIG_SUPPORTS_DPI_OVERRIDE
	0
#else
	1
#endif
);

OptionRecentGames optionRecentGames;

Byte4s2Option optionTouchCtrlImgRes
(CFGKEY_TOUCH_CONTROL_IMG_PIXELS,	128,
		Config::envIsLinux || Config::envIsIOS || Config::envIsWebOS || Config::ENV_ANDROID_MINSDK >= 9,
		optionIsValidWithMax<128>);

#ifdef CONFIG_BASE_ANDROID
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		// Default & current setting isn't known until OpenGL init
		Byte1Option optionDirectTexture(CFGKEY_DIRECT_TEXTURE, OPTION_DIRECT_TEXTURE_UNSET);
	#endif
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		Byte1Option optionSurfaceTexture(CFGKEY_SURFACE_TEXTURE, OPTION_SURFACE_TEXTURE_UNSET);
		SByte1Option optionProcessPriority(CFGKEY_PROCESS_PRIORITY, 0, 0, optionIsValidWithMinMax<-17, 0>);
	#endif
	Option<OptionMethodRef<template_ntype(glSyncHackEnabled)>, uint8> optionGLSyncHack(CFGKEY_GL_SYNC_HACK, 0);
#endif

#ifdef CONFIG_INPUT_ICADE
	Option<OptionMethodFunc<bool, Input::iCadeActive, Input::setICadeActive>, uint8> optionICade(CFGKEY_ICADE, 0);
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	Option<OptionMethodFunc<bool, Input::eventsUseOSInputMethod, Input::setEventsUseOSInputMethod>, uint8> optionUseOSInputMethod(CFGKEY_USE_OS_INPUT_METHOD, 1);
#endif

Byte1Option optionDitherImage(CFGKEY_DITHER_IMAGE, 1, !Config::envIsAndroid);

#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
	Byte1Option optionBestColorModeHint(CFGKEY_BEST_COLOR_MODE_HINT, 1);
#endif

PathOption optionSavePath(CFGKEY_SAVE_PATH, EmuSystem::savePath_, sizeof(EmuSystem::savePath_), "");

void initOptions()
{
	#ifdef CONFIG_BASE_IOS
		if(Base::runningDeviceType() == Base::DEV_TYPE_IPAD)
			optionLargeFonts.initDefault(1);
	#endif

	#ifdef CONFIG_BASE_ANDROID
		optionGLSyncHack.initDefault(glSyncHackBlacklisted);
		if(Base::hasHardwareNavButtons())
		{
			optionLowProfileOSNav.isConst = 1;
			optionHideOSNav.isConst = 1;
			optionShowMenuIcon.initDefault(0);
		}
		else
			optionBackNavigation.initDefault(1);

		if(Base::androidSDK() >= 11)
		{
			optionGLSyncHack.isConst = 1;
			// don't change dither setting on Android 3.0+
			optionDitherImage.isConst = 1;
			if(Base::refreshRate() == 60)
			{
				// TODO: more testing needed with audio sync
				/*logMsg("using default frame-skip 0");
				optionFrameSkip.initDefault(0);*/
			}
		}
		else
		{
			optionDitherImage.initDefault(0);
		}
	#endif

	#ifdef INPUT_SUPPORTS_POINTER
		#ifdef CONFIG_BASE_IOS
			if(Base::runningDeviceType() == Base::DEV_TYPE_IPAD)
				optionTouchCtrlSize.initDefault(1400);
		#endif
		optionTouchCtrlTriggerBtnPos.isConst = vController.hasTriggers() ? 0 : 1;
	#endif

	#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID)
		optionBestColorModeHint.initDefault(Base::windowPixelBestColorHintDefault());
	#endif
}
