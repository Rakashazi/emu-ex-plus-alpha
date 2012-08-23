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

#include <gui/View.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <audio/Audio.hh>

static void setupDrawing(bool force = 0);

class ButtonConfigCategoryView : public BaseMenuView
{
	struct ProfileItem
	{
		const char *name[10];
		uint names;
		uint val;
		uint devType;

		void init(uint devType, KeyProfileManager *profileMgr)
		{
			names = profileMgr->defaultProfiles + 1;
			assert(names < sizeofArray(name));
			name[0] = "Unbind All";
			iterateTimes(profileMgr->defaultProfiles, i)
			{
				name[i+1] = profileMgr->defaultProfile[i].name;
			}
			var_selfSet(devType);
		}

		void profileHandler(TextMenuItem &, const InputEvent &e)
		{
			multiChoiceView.init(name, names, !e.isPointer());
			multiChoiceView.onSelectDelegate().bind<ProfileItem, &ProfileItem::setProfileFromChoice>(this);
			multiChoiceView.place(Gfx::viewportRect());
			modalView = &multiChoiceView;
		}

		bool setProfileFromChoice(int val, const InputEvent &e)
		{
			removeModalView();
			this->val = val;
			ynAlertView.init("Really apply new key bindings?", !e.isPointer());
			ynAlertView.onYesDelegate().bind<ProfileItem, &ProfileItem::confirmAlert>(this);
			ynAlertView.place(Gfx::viewportRect());
			modalView = &ynAlertView;
			return 0;
		}

		void confirmAlert(const InputEvent &e)
		{
			keyConfig.loadProfile(devType, val-1);
			keyMapping.build(EmuControls::category);
		}

	} profileItem;
	TextMenuItem profile;

	struct CategoryItem
	{
		KeyCategory *cat;
		uint devType;
		void init(KeyCategory *cat, uint devType)
		{
			var_selfs(cat);
			var_selfs(devType);
		}

		void categoryHandler(TextMenuItem &, const InputEvent &e)
		{
			bcMenu.init(cat, devType, !e.isPointer());
			viewStack.pushAndShow(&bcMenu);
		}
	} catItem[sizeofArrayConst(EmuControls::category)];
	TextMenuItem cat[sizeofArrayConst(EmuControls::category)];

	MenuItem *item[sizeofArrayConst(EmuControls::category) + 1];
public:
	void init(uint devType, bool highlightFirst)
	{
		using namespace EmuControls;
		name_ = InputEvent::devTypeName(devType);
		uint i = 0;
		profile.init("Load Defaults"); item[i++] = &profile;
		profileItem.init(devType, &EmuControls::profileManager(devType));
		profile.selectDelegate().bind<ProfileItem, &ProfileItem::profileHandler>(&profileItem);
		forEachInArray(category, c)
		{
			cat[c_i].init(c->name); item[i++] = &cat[c_i];
			catItem[c_i].init(c, devType);
			cat[c_i].selectDelegate().bind<CategoryItem, &CategoryItem::categoryHandler>(&catItem[c_i]);
		}
		assert(i <= sizeofArray(item));
		BaseMenuView::init(item, i, highlightFirst);
	}
};

static ButtonConfigCategoryView bcatMenu;

static const char *pauseUnfocusedStr = Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused";

class OptionView : public BaseMenuView
{
protected:
	BoolMenuItem snd;

	static void soundHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionSound = item.on;
	}

	#ifdef CONFIG_AUDIO_OPENSL_ES
	BoolMenuItem sndUnderrunCheck;

	static void soundUnderrunCheckHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionSoundUnderrunCheck = item.on;
	}
	#endif

	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	BoolMenuItem useOSInputMethod;

	static void useOSInputMethodHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		Input::setEventsUseOSInputMethod(!item.on);
	}
	#endif

	#ifdef CONFIG_ENV_WEBOS
	BoolMenuItem touchCtrl;

	static void touchCtrlHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionTouchCtrl = item.on;
	}

	void touchCtrlInit()
	{
		touchCtrl.init("On-screen Controls", int(optionTouchCtrl));
		touchCtrl.selectDelegate().bind<&touchCtrlHandler>();
	}
	#else
	MultiChoiceSelectMenuItem touchCtrl;

	static void touchCtrlSet(MultiChoiceMenuItem &, int val)
	{
		optionTouchCtrl = val;
	}

	void touchCtrlInit()
	{
		static const char *str[] =
		{
			"Off", "On", "Auto"
		};
		touchCtrl.init("On-screen Controls", str, int(optionTouchCtrl), sizeofArray(str));
		touchCtrl.valueDelegate().bind<&touchCtrlSet>();
	}
	#endif

	TextMenuItem touchCtrlConfig;

	static void touchCtrlConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		#ifndef CONFIG_BASE_PS3
			logMsg("init touch config menu");
			tcMenu.init(!e.isPointer());
			viewStack.pushAndShow(&tcMenu);
		#endif
	}

	MultiChoiceSelectMenuItem autoSaveState;

	static void autoSaveStateSet(MultiChoiceMenuItem &, int val)
	{
		switch(val)
		{
			bcase 0: optionAutoSaveState.val = 0;
			bcase 1: optionAutoSaveState.val = 1;
			bcase 2: optionAutoSaveState.val = 15;
			bcase 3: optionAutoSaveState.val = 30;
		}
		EmuSystem::setupAutoSaveStateTime(optionAutoSaveState.val);
		logMsg("set auto-savestate %d", optionAutoSaveState.val);
	}

	void autoSaveStateInit()
	{
		static const char *str[] =
		{
			"Off", "On Exit",
			"15mins", "30mins"
		};
		int val = 0;
		switch(optionAutoSaveState.val)
		{
			bcase 1: val = 1;
			bcase 15: val = 2;
			bcase 30: val = 3;
		}
		autoSaveState.init("Auto-save State", str, val, sizeofArray(str));
		autoSaveState.valueDelegate().bind<&autoSaveStateSet>();
	}

	MultiChoiceSelectMenuItem statusBar;

	static void statusBarSet(MultiChoiceMenuItem &, int val)
	{
		optionHideStatusBar = val;
		setupStatusBarInMenu();
	}

	void statusBarInit()
	{
		static const char *str[] =
		{
			"Off", "In Game", "On",
		};
		int val = 2;
		if(optionHideStatusBar < 2)
			val = optionHideStatusBar;
		statusBar.init("Hide Status Bar", str, val, sizeofArray(str));
		statusBar.valueDelegate().bind<&statusBarSet>();
	}

	MultiChoiceSelectMenuItem frameSkip;

	static void frameSkipSet(MultiChoiceMenuItem &, int val)
	{
		if(val == -1)
		{
			optionFrameSkip.val = EmuSystem::optionFrameSkipAuto;
			logMsg("set auto frame skip");
		}
		else
		{
			optionFrameSkip.val = val;
			logMsg("set frame skip: %d", int(optionFrameSkip));
		}
		EmuSystem::configAudioRate();
	}

	void frameSkipInit()
	{
		static const char *str[] =
		{
			"Auto", "0",
			#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_X11)
			"1", "2", "3", "4"
			#endif
		};
		int baseVal = -1;
		int val = int(optionFrameSkip);
		if(optionFrameSkip.val == EmuSystem::optionFrameSkipAuto)
			val = -1;
		frameSkip.init("Frame Skip", str, val, sizeofArray(str), baseVal);
		frameSkip.valueDelegate().bind<&frameSkipSet>();
	}

	MultiChoiceSelectMenuItem audioRate;

	static void audioRateSet(MultiChoiceMenuItem &, int val)
	{
		uint rate = 44100;
		switch(val)
		{
			bcase 0: rate = 22050;
			bcase 1: rate = 32000;
			bcase 3: rate = 48000;
		}
		if(rate != optionSoundRate)
		{
			optionSoundRate = rate;
			EmuSystem::configAudioRate();
		}
	}

	void audioRateInit()
	{
		static const char *str[] =
		{
			"22KHz", "32KHz", "44KHz", "48KHz"
		};
		int rates = 3;
		if(Audio::supportsRateNative(48000))
		{
			logMsg("supports 48KHz");
			rates++;
		}

		int val = 2; // default to 44KHz
		switch(optionSoundRate)
		{
			bcase 22050: val = 0;
			bcase 32000: val = 1;
			bcase 48000: val = 3;
		}

		audioRate.init("Sound Rate", str, val, rates);
		audioRate.valueDelegate().bind<&audioRateSet>();
	}

	#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
	BoolMenuItem directTexture;

	static void directTextureHandler(BoolMenuItem &item, const InputEvent &e)
	{
		if(!item.active)
		{
			popup.postError(Gfx::androidDirectTextureError());
			return;
		}
		item.toggle();
		Gfx::setUseAndroidDirectTexture(item.on);
		optionDirectTexture.val = item.on;
		if(emuView.vidImg.impl)
			emuView.reinitImage();
	}

	BoolMenuItem glSyncHack;

	static void glSyncHackHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		glSyncHackEnabled = item.on;
	}
	#endif

	BoolMenuItem pauseUnfocused;

	static void pauseUnfocusedHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionPauseUnfocused = item.on;
	}

	BoolMenuItem largeFonts;

	static void largeFontsHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionLargeFonts = item.on;
		setupFont();
		Gfx::onViewChange();
	}

	BoolMenuItem notificationIcon;

	static void notificationIconHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionNotificationIcon = item.on;
	}

	BoolMenuItem lowProfileOSNav;

	static void lowProfileOSNavHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionLowProfileOSNav = item.on;
		applyOSNavStyle();
	}

	BoolMenuItem hideOSNav;

	static void hideOSNavHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionHideOSNav = item.on;
		applyOSNavStyle();
	}

	BoolMenuItem idleDisplayPowerSave;

	static void idleDisplayPowerSaveHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionIdleDisplayPowerSave = item.on;
		Base::setIdleDisplayPowerSave(item.on);
	}

	BoolMenuItem altGamepadConfirm;

	static void altGamepadConfirmHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		input_swappedGamepadConfirm = item.on;
	}

	BoolMenuItem dither;

	static void ditherHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		Gfx::setDither(item.on);
	}

	BoolMenuItem navView;

	static void navViewHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionTitleBar = item.on;
		viewStack.setNavView(item.on ? &viewNav : 0);
		Gfx::onViewChange();
	}

	BoolMenuItem backNav;

	static void backNavHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		View::setNeedsBackControl(item.on);
		viewNav.setBackImage(View::needsBackControl ? getArrowAsset() : nullptr);
		Gfx::onViewChange();
	}

	BoolMenuItem rememberLastMenu;

	static void rememberLastMenuHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionRememberLastMenu = item.on;
	}

	TextMenuItem buttonConfig;

	static void buttonConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		#if defined(CONFIG_BASE_PS3)
			bcatMenu.init(InputEvent::DEV_PS3PAD, !e.isPointer());
		#elif defined(INPUT_SUPPORTS_KEYBOARD)
			bcatMenu.init(InputEvent::DEV_KEYBOARD, !e.isPointer());
		#else
			bug_exit("invalid dev type in initButtonConfigMenu");
		#endif
		viewStack.pushAndShow(&bcatMenu);
	}

	#ifdef CONFIG_INPUT_ICADE
	BoolMenuItem iCade;

	static void iCadeHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		Input::setICadeActive(item.on);
	}

	TextMenuItem iCadeButtonConfig;

	static void iCadeButtonConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		bcatMenu.init(InputEvent::DEV_ICADE, !e.isPointer());
		viewStack.pushAndShow(&bcatMenu);
	}
	#endif

	#ifdef CONFIG_BLUETOOTH
	TextMenuItem wiiButtonConfig;

	static void wiiButtonConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		bcatMenu.init(InputEvent::DEV_WIIMOTE, !e.isPointer());
		viewStack.pushAndShow(&bcatMenu);
	}

	TextMenuItem iCPButtonConfig;

	static void iCPButtonConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		bcatMenu.init(InputEvent::DEV_ICONTROLPAD, !e.isPointer());
		viewStack.pushAndShow(&bcatMenu);
	}

	TextMenuItem zeemoteButtonConfig;

	static void zeemoteButtonConfigHandler(TextMenuItem &, const InputEvent &e)
	{
		bcatMenu.init(InputEvent::DEV_ZEEMOTE, !e.isPointer());
		viewStack.pushAndShow(&bcatMenu);
	}

	MultiChoiceSelectMenuItem btScanSecs;

	static void btScanSecsSet(MultiChoiceMenuItem &, int val)
	{
		switch(val)
		{
			bcase 0: Bluetooth::scanSecs = 2;
			bcase 1: Bluetooth::scanSecs = 4;
			bcase 2: Bluetooth::scanSecs = 6;
			bcase 3: Bluetooth::scanSecs = 8;
			bcase 4: Bluetooth::scanSecs = 10;
		}
		logMsg("set bluetooth scan time %d", Bluetooth::scanSecs);
	}

	void btScanSecsInit()
	{
		static const char *str[] =
		{
			"2secs", "4secs", "6secs", "8secs", "10secs"
		};

		int val = 1;
		switch(Bluetooth::scanSecs)
		{
			bcase 2: val = 0;
			bcase 4: val = 1;
			bcase 6: val = 2;
			bcase 8: val = 3;
			bcase 10: val = 4;
		}
		btScanSecs.init("Bluetooth Scan", str, val, sizeofArray(str));
		btScanSecs.valueDelegate().bind<&btScanSecsSet>();
	}

	BoolMenuItem keepBtActive;

	static void keepBtActiveHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionKeepBluetoothActive = item.on;
	}

	BoolMenuItem btScanCache;

	static void btScanCacheHandler(BoolMenuItem &item, const InputEvent &e)
	{
		item.toggle();
		optionBlueToothScanCache = item.on;
	}
	#endif

	enum { O_AUTO = -1, O_90, O_270, O_0 };

	static int convertOrientationMenuValueToOption(int val)
	{
		if(val == O_AUTO)
			return Gfx::VIEW_ROTATE_AUTO;
		else if(val == O_90)
			return Gfx::VIEW_ROTATE_90;
		else if(val == O_270)
			return Gfx::VIEW_ROTATE_270;
		else if(val == O_0)
			return Gfx::VIEW_ROTATE_0;
		assert(0);
		return 0;
	}

	static void orientationInit(MultiChoiceSelectMenuItem &item, const char *name, uint option)
	{
		static const char *str[] =
		{
			#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
			"Auto",
			#endif

			#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || (defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2)
			"Landscape", "Landscape 2", "Portrait"
			#else
			"90 Left", "90 Right", "Standard"
			#endif
		};
		int baseVal = 0;
		#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
			baseVal = -1;
		#endif
		uint initVal = O_AUTO;
		if(option == Gfx::VIEW_ROTATE_90)
			initVal = O_90;
		else if(option == Gfx::VIEW_ROTATE_270)
			initVal = O_270;
		else if(option == Gfx::VIEW_ROTATE_0)
			initVal = O_0;
		item.init(name, str, initVal, sizeofArray(str), baseVal);
	}

	MultiChoiceSelectMenuItem gameOrientation;

	static void gameOrientationSet(MultiChoiceMenuItem &, int val)
	{
		optionGameOrientation.val = convertOrientationMenuValueToOption(val);
		logMsg("set game orientation: %s", Gfx::orientationName(int(optionGameOrientation)));
	}

	void gameOrientationInit()
	{
		orientationInit(gameOrientation, "Orientation", optionGameOrientation);
		gameOrientation.valueDelegate().bind<&gameOrientationSet>();
	}

	MultiChoiceSelectMenuItem menuOrientation;

	static void menuOrientationSet(MultiChoiceMenuItem &, int val)
	{
		optionMenuOrientation.val = convertOrientationMenuValueToOption(val);
		if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
			Gfx::onViewChange();
		logMsg("set menu orientation: %s", Gfx::orientationName(int(optionMenuOrientation)));
	}

	void menuOrientationInit()
	{
		orientationInit(menuOrientation, "Orientation", optionMenuOrientation);
		menuOrientation.valueDelegate().bind<&menuOrientationSet>();
	}

	MultiChoiceSelectMenuItem aspectRatio;

	static void aspectRatioSet(MultiChoiceMenuItem &, int val)
	{
		optionAspectRatio.val = val;
		logMsg("set aspect ratio: %d", int(optionAspectRatio));
		emuView.placeEmu();
	}

	void aspectRatioInit()
	{
		static const char *str[] = { systemAspectRatioString, "1:1", "Full Screen" };
		aspectRatio.init("Aspect Ratio", str, optionAspectRatio, sizeofArray(str));
		aspectRatio.valueDelegate().bind<&aspectRatioSet>();
	}

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	MultiChoiceSelectMenuItem soundBuffers;

	static void soundBuffersSet(MultiChoiceMenuItem &, int val)
	{
		optionSoundBuffers = val+4;
	}

	void soundBuffersInit()
	{
		static const char *str[] = { "4", "5", "6", "7", "8", "9", "10", "11", "12" };
		soundBuffers.init("Buffer Size In Frames", str, IG::max((int)optionSoundBuffers - 4, 0), sizeofArray(str));
		soundBuffers.valueDelegate().bind<&soundBuffersSet>();
	}
#endif

	MultiChoiceSelectMenuItem zoom;

	static void zoomSet(MultiChoiceMenuItem &, int val)
	{
		switch(val)
		{
			bcase 0: optionImageZoom.val = 100;
			bcase 1: optionImageZoom.val = 90;
			bcase 2: optionImageZoom.val = 80;
			bcase 3: optionImageZoom.val = 70;
			bcase 4: optionImageZoom.val = optionImageZoomIntegerOnly;
		}
		logMsg("set image zoom: %d", int(optionImageZoom));
		emuView.placeEmu();
	}

	void zoomInit()
	{
		static const char *str[] = { "100%", "90%", "80%", "70%", "Integer-only" };
		int val = 0;
		switch(optionImageZoom.val)
		{
			bcase 100: val = 0;
			bcase 90: val = 1;
			bcase 80: val = 2;
			bcase 70: val = 3;
			bcase optionImageZoomIntegerOnly: val = 4;
		}
		zoom.init("Zoom", str, val, sizeofArray(str));
		zoom.valueDelegate().bind<&zoomSet>();
	}

	MultiChoiceSelectMenuItem dpi;

	static void dpiSet(MultiChoiceMenuItem &, int val)
	{
		switch(val)
		{
			bdefault: optionDPI.val = 0;
			bcase 1: optionDPI.val = 96;
			bcase 2: optionDPI.val = 120;
			bcase 3: optionDPI.val = 160;
			bcase 4: optionDPI.val = 220;
			bcase 5: optionDPI.val = 240;
			bcase 6: optionDPI.val = 265;
			bcase 7: optionDPI.val = 320;
		}
		Base::setDPI(optionDPI);
		logMsg("set DPI: %d", (int)optionDPI);
		setupFont();
		Gfx::onViewChange();
	}

	void dpiInit()
	{
		static const char *str[] = { "Auto", "96", "120", "160", "220", "240", "265", "320" };
		uint init = 0;
		switch(optionDPI)
		{
			bcase 96: init = 1;
			bcase 120: init = 2;
			bcase 160: init = 3;
			bcase 220: init = 4;
			bcase 240: init = 5;
			bcase 265: init = 6;
			bcase 320: init = 7;
		}
		assert(init < sizeofArray(str));
		dpi.init("DPI Override", str, init, sizeofArray(str));
		dpi.valueDelegate().bind<&dpiSet>();
	}

	MultiChoiceSelectMenuItem imgFilter;

	static void imgFilterSet(MultiChoiceMenuItem &, int val)
	{
		optionImgFilter.val = val;
		if(emuView.disp.img)
			emuView.vidImg.setFilter(val);
	}

	void imgFilterInit()
	{
		static const char *str[] = { "None", "Linear" };
		imgFilter.init("Image Filter", str, optionImgFilter, sizeofArray(str));
		imgFilter.valueDelegate().bind<&imgFilterSet>();
	}

	MultiChoiceSelectMenuItem overlayEffect;

	static void overlayEffectSet(MultiChoiceMenuItem &, int val)
	{
		uint setVal = 0;
		switch(val)
		{
			bcase 1: setVal = VideoImageOverlay::SCANLINES;
			bcase 2: setVal = VideoImageOverlay::SCANLINES_2;
			bcase 3: setVal = VideoImageOverlay::CRT;
			bcase 4: setVal = VideoImageOverlay::CRT_RGB;
			bcase 5: setVal = VideoImageOverlay::CRT_RGB_2;
		}
		optionOverlayEffect.val = setVal;
		emuView.vidImgOverlay.setEffect(setVal);
		emuView.placeOverlay();
	}

	void overlayEffectInit()
	{
		static const char *str[] = { "Off", "Scanlines", "Scanlines 2x", "CRT Mask", "CRT", "CRT 2x" };
		uint init = 0;
		switch(optionOverlayEffect)
		{
			bcase VideoImageOverlay::SCANLINES: init = 1;
			bcase VideoImageOverlay::SCANLINES_2: init = 2;
			bcase VideoImageOverlay::CRT: init = 3;
			bcase VideoImageOverlay::CRT_RGB: init = 4;
			bcase VideoImageOverlay::CRT_RGB_2: init = 5;
		}
		overlayEffect.init("Overlay Effect", str, init, sizeofArray(str));
		overlayEffect.valueDelegate().bind<&overlayEffectSet>();
	}

	MultiChoiceSelectMenuItem overlayEffectLevel;

	static void overlayEffectLevelSet(MultiChoiceMenuItem &, int val)
	{
		uint setVal = 10;
		switch(val)
		{
			bcase 1: setVal = 25;
			bcase 2: setVal = 33;
			bcase 3: setVal = 50;
			bcase 4: setVal = 66;
			bcase 5: setVal = 75;
			bcase 6: setVal = 100;
		}
		optionOverlayEffectLevel.val = setVal;
		emuView.vidImgOverlay.intensity = setVal/100.;
	}

	void overlayEffectLevelInit()
	{
		static const char *str[] = { "10%", "25%", "33%", "50%", "66%", "75%", "100%" };
		uint init = 0;
		switch(optionOverlayEffectLevel)
		{
			bcase 25: init = 1;
			bcase 33: init = 2;
			bcase 50: init = 3;
			bcase 66: init = 4;
			bcase 75: init = 5;
			bcase 100: init = 6;
		}
		overlayEffectLevel.init("Overlay Effect Level", str, init, sizeofArray(str));
		overlayEffectLevel.valueDelegate().bind<&overlayEffectLevelSet>();
	}

	MultiChoiceSelectMenuItem relativePointerDecel;

	static void relativePointerDecelSet(MultiChoiceMenuItem &, int val)
	{
		#if defined(CONFIG_BASE_ANDROID)
		if(val == 0)
			optionRelPointerDecel.val = optionRelPointerDecelLow;
		else if(val == 1)
			optionRelPointerDecel.val = optionRelPointerDecelMed;
		else if(val == 2)
			optionRelPointerDecel.val = optionRelPointerDecelHigh;
		#endif
	}

	void relativePointerDecelInit()
	{
		static const char *str[] = { "Low", "Med.", "High" };
		int init = 0;
		if(optionRelPointerDecel == optionRelPointerDecelLow)
			init = 0;
		if(optionRelPointerDecel == optionRelPointerDecelMed)
			init = 1;
		if(optionRelPointerDecel == optionRelPointerDecelHigh)
			init = 2;
		relativePointerDecel.init("Trackball Sensitivity", str, init, sizeofArray(str));
		relativePointerDecel.valueDelegate().bind<&relativePointerDecelSet>();
	}

	void loadVideoItems(MenuItem *item[], uint &items)
	{
		name_ = "Video Options";
		if(!optionFrameSkip.isConst) { frameSkipInit(); item[items++] = &frameSkip; }
		if(!optionGameOrientation.isConst) { gameOrientationInit(); item[items++] = &gameOrientation; }
		aspectRatioInit(); item[items++] = &aspectRatio;
		imgFilterInit(); item[items++] = &imgFilter;
		overlayEffectInit(); item[items++] = &overlayEffect;
		overlayEffectLevelInit(); item[items++] = &overlayEffectLevel;
		zoomInit(); item[items++] = &zoom;
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		if(Base::androidSDK() < 14)
		{
			directTexture.init("Direct Texture", optionDirectTexture, Gfx::supportsAndroidDirectTexture()); item[items++] = &directTexture;
			directTexture.selectDelegate().bind<&directTextureHandler>();
		}
		if(!optionGLSyncHack.isConst)
		{
			glSyncHack.init("GPU Sync Hack", optionGLSyncHack); item[items++] = &glSyncHack;
			glSyncHack.selectDelegate().bind<&glSyncHackHandler>();
		}
		#endif
		if(!optionDitherImage.isConst)
		{
			dither.init("Dither Image", Gfx::dither()); item[items++] = &dither;
			dither.selectDelegate().bind<&ditherHandler>();
		}
	}

	void loadAudioItems(MenuItem *item[], uint &items)
	{
		name_ = "Audio Options";
		snd.init("Sound", optionSound); item[items++] = &snd;
		snd.selectDelegate().bind<&soundHandler>();
		if(!optionSoundRate.isConst) { audioRateInit(); item[items++] = &audioRate; }
#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
		soundBuffersInit(); item[items++] = &soundBuffers;
#endif
#ifdef CONFIG_AUDIO_OPENSL_ES
		sndUnderrunCheck.init("Strict Underrun Check", optionSoundUnderrunCheck); item[items++] = &sndUnderrunCheck;
		sndUnderrunCheck.selectDelegate().bind<&soundUnderrunCheckHandler>();
#endif
	}

	void loadInputItems(MenuItem *item[], uint &items)
	{
		name_ = "Input Options";
		if(!optionTouchCtrl.isConst)
		{
			touchCtrlInit(); item[items++] = &touchCtrl;
			touchCtrlConfig.init("On-screen Config"); item[items++] = &touchCtrlConfig;
			touchCtrlConfig.selectDelegate().bind<&touchCtrlConfigHandler>();
		}
		#if !defined(CONFIG_BASE_IOS) && !defined(CONFIG_BASE_PS3)
		buttonConfig.init("Key Config"); item[items++] = &buttonConfig;
		buttonConfig.selectDelegate().bind<&buttonConfigHandler>();
		#endif
		#ifdef CONFIG_INPUT_ICADE
		iCade.init("Use iCade", Input::iCadeActive()); item[items++] = &iCade;
		iCade.selectDelegate().bind<&iCadeHandler>();
		iCadeButtonConfig.init("iCade Key Config"); item[items++] = &iCadeButtonConfig;
		iCadeButtonConfig.selectDelegate().bind<&iCadeButtonConfigHandler>();
		#endif
		#ifdef CONFIG_BLUETOOTH
		wiiButtonConfig.init("Wiimote Key Config"); item[items++] = &wiiButtonConfig;
		wiiButtonConfig.selectDelegate().bind<&wiiButtonConfigHandler>();
		iCPButtonConfig.init("iControlPad Key Config"); item[items++] = &iCPButtonConfig;
		iCPButtonConfig.selectDelegate().bind<&iCPButtonConfigHandler>();
		zeemoteButtonConfig.init("Zeemote JS1 Key Config"); item[items++] = &zeemoteButtonConfig;
		zeemoteButtonConfig.selectDelegate().bind<&zeemoteButtonConfigHandler>();
		btScanSecsInit(); item[items++] = &btScanSecs;
		keepBtActive.init("Background Bluetooth", optionKeepBluetoothActive); item[items++] = &keepBtActive;
		keepBtActive.selectDelegate().bind<&keepBtActiveHandler>();
		btScanCache.init("Bluetooth Scan Cache", optionBlueToothScanCache); item[items++] = &btScanCache;
		btScanCache.selectDelegate().bind<&btScanCacheHandler>();
		#endif
		#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
		useOSInputMethod.init("Skip OS Input Method", !Input::eventsUseOSInputMethod()); item[items++] = &useOSInputMethod;
		useOSInputMethod.selectDelegate().bind<&useOSInputMethodHandler>();
		#endif
		if(!optionRelPointerDecel.isConst) { relativePointerDecelInit(); item[items++] = &relativePointerDecel; }
	}

	void loadSystemItems(MenuItem *item[], uint &items)
	{
		name_ = "System Options";
		autoSaveStateInit(); item[items++] = &autoSaveState;
	}

	void loadGUIItems(MenuItem *item[], uint &items)
	{
		name_ = "GUI Options";
		if(!optionMenuOrientation.isConst) { menuOrientationInit(); item[items++] = &menuOrientation; }
		if(!optionPauseUnfocused.isConst)
		{
			pauseUnfocused.init(pauseUnfocusedStr, optionPauseUnfocused); item[items++] = &pauseUnfocused;
			pauseUnfocused.selectDelegate().bind<&pauseUnfocusedHandler>();
		}
		if(!optionNotificationIcon.isConst)
		{
			notificationIcon.init("Suspended App Icon", optionNotificationIcon); item[items++] = &notificationIcon;
			notificationIcon.selectDelegate().bind<&notificationIconHandler>();
		}
		if(!optionTitleBar.isConst)
		{
			navView.init("Title Bar", optionTitleBar); item[items++] = &navView;
			navView.selectDelegate().bind<&navViewHandler>();
		}
		if(!View::needsBackControlIsConst)
		{
			backNav.init("Title Back Navigation", View::needsBackControl); item[items++] = &backNav;
			backNav.selectDelegate().bind<&backNavHandler>();
		}
		rememberLastMenu.init("Remember Last Menu", optionRememberLastMenu); item[items++] = &rememberLastMenu;
		rememberLastMenu.selectDelegate().bind<&rememberLastMenuHandler>();
		if(!optionLargeFonts.isConst)
		{
			largeFonts.init("Large Fonts", optionLargeFonts); item[items++] = &largeFonts;
			largeFonts.selectDelegate().bind<&largeFontsHandler>();
		}
		if(!optionDPI.isConst) { dpiInit(); item[items++] = &dpi; }
		#ifndef CONFIG_ENV_WEBOS
		altGamepadConfirm.init("Alt Gamepad Confirm", input_swappedGamepadConfirm); item[items++] = &altGamepadConfirm;
		altGamepadConfirm.selectDelegate().bind<&altGamepadConfirmHandler>();
		#endif
		if(!optionIdleDisplayPowerSave.isConst)
		{
			idleDisplayPowerSave.init("Dim Screen If Idle", optionIdleDisplayPowerSave); item[items++] = &idleDisplayPowerSave;
			idleDisplayPowerSave.selectDelegate().bind<&idleDisplayPowerSaveHandler>();
		}
		if(!optionLowProfileOSNav.isConst)
		{
			lowProfileOSNav.init("Dim OS Navigation", optionLowProfileOSNav); item[items++] = &lowProfileOSNav;
			lowProfileOSNav.selectDelegate().bind<&lowProfileOSNavHandler>();
		}
		if(!optionHideOSNav.isConst)
		{
			hideOSNav.init("Hide OS Navigation", optionHideOSNav); item[items++] = &hideOSNav;
			hideOSNav.selectDelegate().bind<&hideOSNavHandler>();
		}
		if(!optionHideStatusBar.isConst)
		{
			statusBarInit(); item[items++] = &statusBar;
		}
	}

public:
	constexpr OptionView(): BaseMenuView("Options") { }
};
