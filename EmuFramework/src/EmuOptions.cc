#include <Option.hh>
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

BasicByteOption optionAutoSaveState(CFGKEY_AUTO_SAVE_STATE, 1);
BasicByteOption optionConfirmAutoLoadState(CFGKEY_CONFIRM_AUTO_LOAD_STATE, 1);
BasicByteOption optionSound(CFGKEY_SOUND, 1);

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
OptionAudioHintPcmMaxBuffers optionSoundBuffers(CFGKEY_SOUND_BUFFERS, Config::envIsAndroid ? 10 : 8);
#endif

#ifdef CONFIG_AUDIO_OPENSL_ES
OptionAudioHintStrictUnderrunCheck optionSoundUnderrunCheck(CFGKEY_SOUND_UNDERRUN_CHECK, 1);
#endif

OptionSoundRate optionSoundRate(CFGKEY_SOUND_RATE,
	(Config::envIsPS3 || Config::envIsLinux) ? 48000 : 44100, Config::envIsPS3);

BasicByteOption optionLargeFonts(CFGKEY_FONT_Y_PIXELS, Config::envIsWebOS3,
	(Config::envIsWebOS && !Config::envIsWebOS3));

BasicByteOption optionVibrateOnPush(CFGKEY_TOUCH_CONTROL_VIRBRATE, 0, !Config::envIsAndroid);

BasicByteOption optionPauseUnfocused(CFGKEY_PAUSE_UNFOCUSED, 1,
	!(Config::envIsPS3 || Config::envIsLinux || Config::envIsAndroid));

BasicByteOption optionNotificationIcon(CFGKEY_NOTIFICATION_ICON, 1, !Config::envIsAndroid);
BasicByteOption optionTitleBar(CFGKEY_TITLE_BAR, 1, Config::envIsIOS || Config::envIsWebOS3);

OptionBackNavigation
	optionBackNavigation(CFGKEY_BACK_NAVIGATION, View::needsBackControlDefault, View::needsBackControlIsConst);

BasicByteOption optionRememberLastMenu(CFGKEY_REMEMBER_LAST_MENU, 1, 0);
BasicByteOption optionLowProfileOSNav(CFGKEY_LOW_PROFILE_OS_NAV, 1, !Config::envIsAndroid);
BasicByteOption optionHideOSNav(CFGKEY_HIDE_OS_NAV, 0, !Config::envIsAndroid);
BasicByteOption optionIdleDisplayPowerSave(CFGKEY_IDLE_DISPLAY_POWER_SAVE, 1, !Config::envIsAndroid && !Config::envIsIOS);
BasicByteOption optionShowMenuIcon(CFGKEY_SHOW_MENU_ICON, Config::envIsIOS || Config::envIsAndroid || Config::envIsWebOS3, Config::envIsPS3);
BasicByteOption optionHideStatusBar(CFGKEY_HIDE_STATUS_BAR, 2, !Config::envIsAndroid && !Config::envIsIOS);

OptionSwappedGamepadConfirm optionSwappedGamepadConfirm(CFGKEY_SWAPPED_GAMEPAD_CONFIM, 0);

#ifdef CONFIG_BLUETOOTH
BasicByteOption optionKeepBluetoothActive(CFGKEY_KEEP_BLUETOOTH_ACTIVE, 0, !Config::BASE_CAN_BACKGROUND_APP);
OptionBlueToothScanCache optionBlueToothScanCache(CFGKEY_BLUETOOTH_SCAN_CACHE, 1);
#endif

OptionAspectRatio optionAspectRatio(0);

OptionMethodImgFilter optionImgFilter(CFGKEY_GAME_IMG_FILTER, GfxBufferImage::linear);

OptionMethodOverlayEffect optionOverlayEffect(CFGKEY_OVERLAY_EFFECT, 0);
OptionMethodOverlayEffectLevel optionOverlayEffectLevel(CFGKEY_OVERLAY_EFFECT_LEVEL, 25);

OptionMethodRelPointerDecel optionRelPointerDecel(CFGKEY_REL_POINTER_DECEL, optionRelPointerDecelMed,
		!Config::envIsAndroid);

OptionMethodOrientation optionGameOrientation(CFGKEY_GAME_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Config::envIsWebOS ? Gfx::VIEW_ROTATE_90 : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3);

OptionMethodOrientation optionMenuOrientation(CFGKEY_MENU_ORIENTATION,
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS3) ? Gfx::VIEW_ROTATE_AUTO : Gfx::VIEW_ROTATE_0,
		Config::envIsPS3);

OptionMethodTouchCtrl optionTouchCtrl(CFGKEY_TOUCH_CONTROL_DISPLAY,
		(Config::envIsLinux || Config::envIsPS3) ? 0 : 2,
		Config::envIsPS3);

OptionMethodTouchCtrlAlpha optionTouchCtrlAlpha(CFGKEY_TOUCH_CONTROL_ALPHA,
		255 * .5,
		Config::envIsPS3);

Option<OptionMethodVar<uint32, optionIsValidWithMax<1400> >, uint16> optionTouchCtrlSize
		(CFGKEY_TOUCH_CONTROL_SIZE,
		(Config::envIsWebOS && !Config::envIsWebOS3) ? 800 : Config::envIsWebOS3 ? 1400 : 850,
		Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<160> >, uint16> optionTouchDpadDeadzone
		(CFGKEY_TOUCH_CONTROL_DPAD_DEADZONE,
		135,
		Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMinMax<1000,2500> >, uint16> optionTouchDpadDiagonalSensitivity
		(CFGKEY_TOUCH_CONTROL_DIAGONAL_SENSITIVITY,
		1750,
		Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<400> >, uint16> optionTouchCtrlBtnSpace
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_SPACE,
		200,
		Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<5> >, uint16> optionTouchCtrlBtnStagger
		(CFGKEY_TOUCH_CONTROL_FACE_BTN_STAGGER,
		1,
		Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<3> >, uint16> optionTouchCtrlTriggerBtnPos
		(CFGKEY_TOUCH_CONTROL_TRIGGER_BTN_POS,
		TRIGGERS_SPLIT, Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraXBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_X_BTN_SIZE,
		200, Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSize
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE,
		1000, Config::envIsPS3);
Option<OptionMethodVar<uint32, optionIsValidWithMax<1000> >, uint16> optionTouchCtrlExtraYBtnSizeMultiRow
		(CFGKEY_TOUCH_CONTROL_EXTRA_Y_BTN_SIZE_MULTI_ROW,
		200, Config::envIsPS3);

bool isValidOption2DO(_2DOrigin val)
{
	return val.isValid() && val != C2DO;
}
bool isValidOption2DOCenterBtn(_2DOrigin val)
{
	return val.isValid() && !val.onYCenter();
}

Option2DOrigin optionTouchCtrlDpadPos(CFGKEY_TOUCH_CONTROL_DPAD_POS, LB2DO);
Option2DOrigin optionTouchCtrlFaceBtnPos(CFGKEY_TOUCH_CONTROL_FACE_BTN_POS, RB2DO);
Option2DOriginC optionTouchCtrlCenterBtnPos(CFGKEY_TOUCH_CONTROL_CENTER_BTN_POS, CB2DO);
Option2DOrigin optionTouchCtrlMenuPos(CFGKEY_TOUCH_CONTROL_MENU_POS,
	#if defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2
		NULL2DO
	#else
		RT2DO
	#endif
	);
Option2DOrigin optionTouchCtrlFFPos(CFGKEY_TOUCH_CONTROL_FF_POS, Config::envIsIOS ? LT2DO : NULL2DO);

bool optionFrameSkipIsValid(uint32 val)
{
	uint limit = (Config::envIsIOS || Config::envIsLinux) ? 4 : 0;
	return val == EmuSystem::optionFrameSkipAuto || val <= limit;
}

Option<OptionMethodVar<uint32, optionFrameSkipIsValid >, uint8> optionFrameSkip
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

bool optionImageZoomIsValid(uint32 val)
{
	return val == optionImageZoomIntegerOnly || val <= 100;
}
Option<OptionMethodVar<uint32, optionImageZoomIsValid>, uint8> optionImageZoom
		(CFGKEY_IMAGE_ZOOM, 100);

OptionDPI optionDPI(0,
#ifdef CONFIG_SUPPORTS_DPI_OVERRIDE
	0
#else
	1
#endif
);

OptionRecentGames optionRecentGames;

Option<OptionMethodVar<uint32, optionIsValidWithMax<128> >, uint16> optionTouchCtrlImgRes
(CFGKEY_TOUCH_CONTROL_IMG_PIXELS,	128, Config::envIsLinux || Config::envIsIOS || Config::envIsWebOS || Config::ENV_ANDROID_MINSDK >= 9);

#ifdef CONFIG_BASE_ANDROID
	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		OptionDirectTexture optionDirectTexture;
	#endif
	#if CONFIG_ENV_ANDROID_MINSDK >= 9
		Option<OptionMethodFunc<bool, Gfx::useAndroidSurfaceTexture, Gfx::setUseAndroidSurfaceTexture>, uint8> optionSurfaceTexture(CFGKEY_SURFACE_TEXTURE, 1);
		Option<OptionMethodVar<int8, optionIsValidWithMinMax<-17, 0> > > optionProcessPriority(CFGKEY_PROCESS_PRIORITY, 0);
	#endif
	Option<OptionMethodRef<template_ntype(glSyncHackEnabled)>, uint8> optionGLSyncHack(CFGKEY_GL_SYNC_HACK, 0);
#endif

#ifdef CONFIG_INPUT_ICADE
	Option<OptionMethodFunc<bool, Input::iCadeActive, Input::setICadeActive>, uint8> optionICade(CFGKEY_ICADE, 0);
#endif

#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	Option<OptionMethodFunc<bool, Input::eventsUseOSInputMethod, Input::setEventsUseOSInputMethod>, uint8> optionUseOSInputMethod(CFGKEY_USE_OS_INPUT_METHOD, 1);
#endif

Option<OptionMethodFunc<uint, Gfx::dither, Gfx::setDither>, uint8> optionDitherImage(CFGKEY_DITHER_IMAGE, 1, !Config::envIsAndroid);

void initOptions()
{
	#ifdef CONFIG_BASE_IOS
		if(Base::runningDeviceType() == Base::DEV_TYPE_IPAD)
			optionLargeFonts.initDefault(1);
	#endif

	#ifdef CONFIG_BASE_ANDROID
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
		#ifdef CONFIG_BASE_ANDROID
			if(!optionTouchCtrlImgRes.isConst)
				optionTouchCtrlImgRes.initDefault((Gfx::viewPixelWidth() * Gfx::viewPixelHeight() > 380000) ? 128 : 64);
		#endif
	#endif

	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		optionDirectTexture.initDefault(Gfx::supportsAndroidDirectTextureWhitelisted());
		Gfx::setUseAndroidDirectTexture(optionDirectTexture);
		optionGLSyncHack.initDefault(glSyncHackBlacklisted);
	#endif
}
