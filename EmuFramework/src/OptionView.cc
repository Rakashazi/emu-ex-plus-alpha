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

#include <OptionView.hh>
#include <MsgPopup.hh>
#include <FilePicker.hh>
#include <algorithm>

extern MsgPopup popup;
extern EmuNavView viewNav;

#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	#ifdef CONFIG_ENV_WEBOS
	void OptionView::touchCtrlInit()
	{
		touchCtrl.init(int(optionTouchCtrl));
		touchCtrl.selectDelegate().bind<&touchCtrlHandler>();
	}
	#else
	void OptionView::touchCtrlInit()
	{
		static const char *str[] =
		{
			"Off", "On", "Auto"
		};
		touchCtrl.init(str, int(optionTouchCtrl), sizeofArray(str));
	}
	#endif
#endif

void OptionView::autoSaveStateInit()
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
	autoSaveState.init(str, val, sizeofArray(str));
}

void OptionView::statusBarInit()
{
	static const char *str[] =
	{
		"Off", "In Game", "On",
	};
	int val = 2;
	if(optionHideStatusBar < 2)
		val = optionHideStatusBar;
	statusBar.init(str, val, sizeofArray(str));
}

void OptionView::frameSkipInit()
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
	frameSkip.init(str, val, sizeofArray(str), baseVal);
}

void OptionView::audioRateInit()
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

	audioRate.init(str, val, rates);
}

#ifdef CONFIG_BASE_ANDROID
static void glSyncHackHandler(BoolMenuItem &item, const Input::Event &e)
{
	item.toggle();
	glSyncHackEnabled = item.on;
}
#endif

#ifdef CONFIG_BLUETOOTH_SCAN_SECS
void OptionView::btScanSecsInit()
{
	static const char *str[] =
	{
		"2secs", "4secs", "6secs", "8secs", "10secs"
	};

	int val = 1;
	switch(BluetoothAdapter::scanSecs)
	{
		bcase 2: val = 0;
		bcase 4: val = 1;
		bcase 6: val = 2;
		bcase 8: val = 3;
		bcase 10: val = 4;
	}
	btScanSecs.init(str, val, sizeofArray(str));
}
#endif

enum { O_AUTO = -1, O_90, O_270, O_0, O_180 };

int convertOrientationMenuValueToOption(int val)
{
	if(val == O_AUTO)
		return Gfx::VIEW_ROTATE_AUTO;
	else if(val == O_90)
		return Gfx::VIEW_ROTATE_90;
	else if(val == O_270)
		return Gfx::VIEW_ROTATE_270;
	else if(val == O_0)
		return Gfx::VIEW_ROTATE_0;
	else if(val == O_180)
		return Gfx::VIEW_ROTATE_180;
	bug_exit("invalid value %d", val);
	return 0;
}

static void orientationInit(MultiChoiceSelectMenuItem &item, uint option)
{
	static const char *str[] =
	{
		#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || CONFIG_ENV_WEBOS_OS >= 3
		"Auto",
		#endif

		#if defined(CONFIG_BASE_IOS) || defined(CONFIG_BASE_ANDROID) || (defined CONFIG_ENV_WEBOS && CONFIG_ENV_WEBOS_OS <= 2)
		"Landscape", "Landscape 2", "Portrait", "Portrait 2"
		#else
		"90 Left", "90 Right", "Standard", "Upside Down"
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
	else if(option == Gfx::VIEW_ROTATE_180)
		initVal = O_180;
	item.init(str, initVal, sizeofArray(str), baseVal);
}

void OptionView::gameOrientationInit()
{
	orientationInit(gameOrientation, optionGameOrientation);
}

void OptionView::menuOrientationInit()
{
	orientationInit(menuOrientation, optionMenuOrientation);
}

void OptionView::aspectRatioInit()
{
	static const char *str[] = { systemAspectRatioString, "1:1", "Full Screen" };
	aspectRatio.init(str, optionAspectRatio, sizeofArray(str));
}

#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
void OptionView::soundBuffersInit()
{
	static const char *str[] = { "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
	soundBuffers.init(str, std::max((int)optionSoundBuffers - 3, 0), sizeofArray(str));
}
#endif

void OptionView::zoomInit()
{
	static const char *str[] = { "100%", "90%", "80%", "70%", "Integer-only", "Integer-only (Height)" };
	int val = 0;
	switch(optionImageZoom.val)
	{
		bcase 100: val = 0;
		bcase 90: val = 1;
		bcase 80: val = 2;
		bcase 70: val = 3;
		bcase optionImageZoomIntegerOnly: val = 4;
		bcase optionImageZoomIntegerOnlyY: val = 5;
	}
	zoom.init(str, val, sizeofArray(str));
}

void OptionView::imgFilterInit()
{
	static const char *str[] = { "None", "Linear" };
	imgFilter.init(str, optionImgFilter, sizeofArray(str));
}

void OptionView::overlayEffectInit()
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
	overlayEffect.init(str, init, sizeofArray(str));
}

void OptionView::overlayEffectLevelInit()
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
	overlayEffectLevel.init(str, init, sizeofArray(str));
}

void OptionView::fontSizeInit()
{
	static const char *str[] =
	{
			"2", "2.5",
			"3", "3.5",
			"4", "4.5",
			"5", "5.5",
			"6", "6.5",
			"7", "7.5",
			"8", "8.5",
			"9", "9.5",
			"10", "10.50",
	};
	uint init = 0;
	switch(optionFontSize)
	{
		bcase 2500: init = 1;
		bcase 3000: init = 2;
		bcase 3500: init = 3;
		bcase 4000: init = 4;
		bcase 4500: init = 5;
		bcase 5000: init = 6;
		bcase 5500: init = 7;
		bcase 6000: init = 8;
		bcase 6500: init = 9;
		bcase 7000: init = 10;
		bcase 7500: init = 11;
		bcase 8000: init = 12;
		bcase 8500: init = 13;
		bcase 9000: init = 14;
		bcase 9500: init = 15;
		bcase 10000: init = 16;
		bcase 10500: init = 17;
	}
	fontSize.init(str, init, sizeofArray(str));
}

void OptionView::relativePointerDecelInit()
{
	static const char *str[] = { "Low", "Med.", "High" };
	int init = 0;
	if(optionRelPointerDecel == optionRelPointerDecelLow)
		init = 0;
	if(optionRelPointerDecel == optionRelPointerDecelMed)
		init = 1;
	if(optionRelPointerDecel == optionRelPointerDecelHigh)
		init = 2;
	relativePointerDecel.init(str, init, sizeofArray(str));
}

#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
void OptionView::processPriorityInit()
{
	static const char *str[] = { "Normal", "High", "Very High" };
	auto prio = Base::processPriority();
	int init = 0;
	if(optionProcessPriority.val == 0)
		init = 0;
	if(optionProcessPriority.val == -6)
		init = 1;
	if(optionProcessPriority.val == -14)
		init = 2;
	processPriority.init(str, init, sizeofArray(str));
}
#endif

typedef DelegateFunc<void (const char *newPath)> PathChangeDelegate;

static int dirFsFilter(const char *name, int type)
{
	return type == Fs::TYPE_DIR;
}

template <size_t S>
static void printPathMenuEntryStr(char (&str)[S])
{
	FsSys::cPath basenameTemp;
	string_printf(str, "Save Path: %s", strlen(optionSavePath) ? string_basename(optionSavePath, basenameTemp) : "Same as Game");
}

static class SavePathSelectMenu
{
public:
	constexpr SavePathSelectMenu() { }
	PathChangeDelegate onPathChange;

	void onClose(const Input::Event &e)
	{
		snprintf(optionSavePath, sizeof(FsSys::cPath), "%s", FsSys::workDir());
		logMsg("set save path %s", (char*)optionSavePath);
		if(onPathChange) onPathChange(optionSavePath);
		View::removeModalView();
		workDirStack.pop();
	}

	void init(bool highlightFirst)
	{
		static const char *str[] { "Set Custom Path", "Same as Game" };
		auto &multiChoiceView = *allocModalView<MultiChoiceView>();
		multiChoiceView.init(str, sizeofArray(str), highlightFirst);
		multiChoiceView.onSelect() =
			[this](int i, const Input::Event &e)
			{
				View::removeModalView();
				if(i == 0)
				{
					workDirStack.push();
					FsSys::chdir(optionSavePath);
					auto &fPicker = *allocModalView<EmuFilePicker>();
					fPicker.init(!e.isPointer(), dirFsFilter);
					fPicker.onClose() = [this](const Input::Event &e){onClose(e);};
					View::addModalView(fPicker);
				}
				else
				{
					strcpy(optionSavePath, "");
					if(onPathChange) onPathChange("");
				}
				return 0;
			};
		View::addModalView(multiChoiceView);
	}
} pathSelectMenu;



void OptionView::loadVideoItems(MenuItem *item[], uint &items)
{
	name_ = "Video Options";
	if(!optionFrameSkip.isConst) { frameSkipInit(); item[items++] = &frameSkip; }
	if(!optionGameOrientation.isConst) { gameOrientationInit(); item[items++] = &gameOrientation; }
	aspectRatioInit(); item[items++] = &aspectRatio;
	imgFilterInit(); item[items++] = &imgFilter;
	overlayEffectInit(); item[items++] = &overlayEffect;
	overlayEffectLevelInit(); item[items++] = &overlayEffectLevel;
	zoomInit(); item[items++] = &zoom;
	#ifdef CONFIG_BASE_ANDROID
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		if(Base::androidSDK() < 14)
		{
			assert(optionDirectTexture != OPTION_DIRECT_TEXTURE_UNSET);
			directTexture.init(optionDirectTexture, Gfx::supportsAndroidDirectTexture()); item[items++] = &directTexture;
		}
		#endif
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
		if(Base::androidSDK() >= 14 && !optionSurfaceTexture.isConst)
		{
			surfaceTexture.init(optionSurfaceTexture); item[items++] = &surfaceTexture;
		}
		#endif
	if(!optionGLSyncHack.isConst)
	{
		glSyncHack.init(optionGLSyncHack); item[items++] = &glSyncHack;
	}
	#endif
	#ifdef USE_BEST_COLOR_MODE_OPTION
	bestColorModeHint.init(optionBestColorModeHint); item[items++] = &bestColorModeHint;
	#endif
	if(!optionDitherImage.isConst)
	{
		dither.init(optionDitherImage); item[items++] = &dither;
	}
}

void OptionView::loadAudioItems(MenuItem *item[], uint &items)
{
	name_ = "Audio Options";
	snd.init(optionSound); item[items++] = &snd;
	if(!optionSoundRate.isConst) { audioRateInit(); item[items++] = &audioRate; }
	#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	soundBuffersInit(); item[items++] = &soundBuffers;
	#endif
	#ifdef CONFIG_AUDIO_OPENSL_ES
	sndUnderrunCheck.init(optionSoundUnderrunCheck); item[items++] = &sndUnderrunCheck;
	#endif
	#ifdef CONFIG_AUDIO_SOLO_MIX
	audioSoloMix.init(!optionAudioSoloMix); item[items++] = &audioSoloMix;
	#endif
}

void OptionView::loadInputItems(MenuItem *item[], uint &items)
{
	name_ = "Input Options";
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	if(!optionTouchCtrl.isConst)
	{
		touchCtrlInit(); item[items++] = &touchCtrl;
		touchCtrlConfig.init(); item[items++] = &touchCtrlConfig;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecsInit(); item[items++] = &btScanSecs;
	#endif
	#ifdef CONFIG_BLUETOOTH
	if(!optionKeepBluetoothActive.isConst)
	{
		keepBtActive.init(optionKeepBluetoothActive); item[items++] = &keepBtActive;
	}
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache.init(optionBlueToothScanCache); item[items++] = &btScanCache;
	#endif
	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	useOSInputMethod.init(!Input::eventsUseOSInputMethod()); item[items++] = &useOSInputMethod;
	#endif
	if(!optionRelPointerDecel.isConst) { relativePointerDecelInit(); item[items++] = &relativePointerDecel; }
}

void OptionView::loadSystemItems(MenuItem *item[], uint &items)
{
	name_ = "System Options";
	autoSaveStateInit(); item[items++] = &autoSaveState;
	confirmAutoLoadState.init(optionConfirmAutoLoadState); item[items++] = &confirmAutoLoadState;
	confirmOverwriteState.init(optionConfirmOverwriteState); item[items++] = &confirmOverwriteState;
	printPathMenuEntryStr(savePathStr);
	savePath.init(savePathStr, optionConfirmAutoLoadState); item[items++] = &savePath;
	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	processPriorityInit(); item[items++] = &processPriority;
	#endif
}

void OptionView::loadGUIItems(MenuItem *item[], uint &items)
{
	name_ = "GUI Options";
	if(!optionMenuOrientation.isConst) { menuOrientationInit(); item[items++] = &menuOrientation; }
	if(!optionPauseUnfocused.isConst)
	{
		pauseUnfocused.init(optionPauseUnfocused); item[items++] = &pauseUnfocused;
	}
	if(!optionNotificationIcon.isConst)
	{
		notificationIcon.init(optionNotificationIcon); item[items++] = &notificationIcon;
	}
	if(!optionTitleBar.isConst)
	{
		navView.init(optionTitleBar); item[items++] = &navView;
	}
	if(!View::needsBackControlIsConst)
	{
		backNav.init(View::needsBackControl); item[items++] = &backNav;
	}
	rememberLastMenu.init(optionRememberLastMenu); item[items++] = &rememberLastMenu;
	if(!optionFontSize.isConst)
	{
		fontSizeInit(); item[items++] = &fontSize;
	}
	#ifndef CONFIG_ENV_WEBOS
	altGamepadConfirm.init(Input::swappedGamepadConfirm); item[items++] = &altGamepadConfirm;
	#endif
	if(!optionIdleDisplayPowerSave.isConst)
	{
		idleDisplayPowerSave.init(optionIdleDisplayPowerSave); item[items++] = &idleDisplayPowerSave;
	}
	if(!optionLowProfileOSNav.isConst)
	{
		lowProfileOSNav.init(optionLowProfileOSNav); item[items++] = &lowProfileOSNav;
	}
	if(!optionHideOSNav.isConst)
	{
		hideOSNav.init(optionHideOSNav); item[items++] = &hideOSNav;
	}
	if(!optionHideStatusBar.isConst)
	{
		statusBarInit(); item[items++] = &statusBar;
	}
}

void OptionView::init(uint idx, bool highlightFirst)
{
	uint i = 0;
	switch(idx)
	{
		bcase 0: loadVideoItems(item, i);
		bcase 1: loadAudioItems(item, i);
		bcase 2: loadInputItems(item, i);
		bcase 3: loadSystemItems(item, i);
		bcase 4: loadGUIItems(item, i);
	}
	assert(i <= sizeofArray(item));
	BaseMenuView::init(item, i, highlightFirst);
}

OptionView::OptionView():
	BaseMenuView("Options"),
	// Video
	#ifdef CONFIG_BASE_ANDROID
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		directTexture
		{
			"Direct Texture",
			[](BoolMenuItem &item, const Input::Event &e)
			{
				if(!item.active)
				{
					popup.postError(Gfx::androidDirectTextureError());
					return;
				}
				item.toggle();
				Gfx::setUseAndroidDirectTexture(item.on);
				optionDirectTexture = item.on;
				if(emuView.vidImg)
					emuView.reinitImage();
			}
		},
		#endif
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
		surfaceTexture
		{
			"Fast CPU->GPU Copy",
			[](BoolMenuItem &item, const Input::Event &e)
			{
				item.toggle();
				optionSurfaceTexture = item.on;
				Gfx::setUseAndroidSurfaceTexture(item.on);
				if(emuView.vidImg)
					emuView.reinitImage();
			}
		},
		#endif
	glSyncHack
	{
		"GPU Sync Hack",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			glSyncHackEnabled = item.on;
		}
	},
	#endif
	frameSkip
	{
		"Frame Skip",
		[](MultiChoiceMenuItem &, int val)
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
			EmuSystem::configAudioPlayback();
		}
	},
	aspectRatio
	{
		"Aspect Ratio",
		[](MultiChoiceMenuItem &, int val)
		{
			optionAspectRatio.val = val;
			logMsg("set aspect ratio: %d", int(optionAspectRatio));
			emuView.placeEmu();
		}
	},
	zoom
	{
		"Zoom",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bcase 0: optionImageZoom.val = 100;
				bcase 1: optionImageZoom.val = 90;
				bcase 2: optionImageZoom.val = 80;
				bcase 3: optionImageZoom.val = 70;
				bcase 4: optionImageZoom.val = optionImageZoomIntegerOnly;
				bcase 5: optionImageZoom.val = optionImageZoomIntegerOnlyY;
			}
			logMsg("set image zoom: %d", int(optionImageZoom));
			emuView.placeEmu();
		}
	},
	imgFilter
	{
		"Image Filter",
		[](MultiChoiceMenuItem &, int val)
		{
			optionImgFilter.val = val;
			if(emuView.disp.image())
				emuView.vidImg.setFilter(val);
		}
	},
	overlayEffect
	{
		"Overlay Effect",
		[](MultiChoiceMenuItem &, int val)
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
	},
	overlayEffectLevel
	{
		"Overlay Effect Level",
		[](MultiChoiceMenuItem &, int val)
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
	},
	#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID) || defined (CONFIG_BASE_IOS)
	bestColorModeHint
	{
		"Use Highest Color Mode",
		[this](BoolMenuItem &item, const Input::Event &e)
		{
			auto alertMsg = Config::envIsAndroid ? "This option takes effect next time you launch the app. "
					"Not all devices properly support high color modes so turn this off "
					"if you experience graphics problems."
					: "This option takes effect next time you launch the app. "
					"If off, it may improve performance at the cost of color accuracy.";
			auto onMsg = Config::envIsAndroid ? "Enable" : "Toggle";
			if((!item.on && Config::envIsAndroid) || !Config::envIsAndroid)
			{
				auto &ynAlertView = *allocModalView<YesNoAlertView>();
				ynAlertView.init(alertMsg, !e.isPointer(), onMsg, "Cancel");
				ynAlertView.onYes() =
					[&item](const Input::Event &e)
					{
						item.toggle();
						optionBestColorModeHint = item.on;
					};
				View::addModalView(ynAlertView);
			}
			else
			{
				item.toggle();
				optionBestColorModeHint = item.on;
			}
		}
	},
	#endif
	dither
	{
		"Dither Image",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionDitherImage = item.on;
			Gfx::setDither(item.on);
		}
	},
	gameOrientation
	{
		"Orientation",
		[](MultiChoiceMenuItem &, int val)
		{
			optionGameOrientation.val = convertOrientationMenuValueToOption(val);
			logMsg("set game orientation: %s", Gfx::orientationName(int(optionGameOrientation)));
		}
	},
	// Audio
	snd
	{
		"Sound",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionSound = item.on;
			if(!item.on)
				Audio::closePcm();
		}
	},
	#ifdef CONFIG_AUDIO_CAN_USE_MAX_BUFFERS_HINT
	soundBuffers
	{
		"Buffer Size In Frames",
		[](MultiChoiceMenuItem &, int val)
		{
			if(Audio::isOpen())
				Audio::closePcm();
			optionSoundBuffers = val+3;
		}
	},
	#endif
	audioRate
	{
		"Sound Rate",
		[](MultiChoiceMenuItem &, int val)
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
				EmuSystem::configAudioPlayback();
			}
		}
	},
	#ifdef CONFIG_AUDIO_OPENSL_ES
	sndUnderrunCheck
	{
		"Strict Underrun Check",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			if(Audio::isOpen())
				Audio::closePcm();
			item.toggle();
			optionSoundUnderrunCheck = item.on;
		}
	},
	#endif
	#ifdef CONFIG_AUDIO_SOLO_MIX
	audioSoloMix
	{
		"Mix With Other Apps",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionAudioSoloMix = !item.on;
		}
	},
	#endif
	// Input
	#if defined(CONFIG_INPUT_ANDROID) && CONFIG_ENV_ANDROID_MINSDK >= 9
	useOSInputMethod
	{
		"Skip OS Input Method",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			Input::setEventsUseOSInputMethod(!item.on);
		}
	},
	#endif
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	touchCtrl
	{
		"On-screen Controls",
		#ifdef CONFIG_ENV_WEBOS
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionTouchCtrl = item.on;
			setOnScreenControls(item.on);
		}
		#else
		[](MultiChoiceMenuItem &, int val)
		{
			optionTouchCtrl = val;
			if(val == 2)
				EmuControls::updateAutoOnScreenControlVisible();
			else
				EmuControls::setOnScreenControls(val);
		}
		#endif
	},
	touchCtrlConfig
	{
		"On-screen Config",
		[](TextMenuItem &, const Input::Event &e)
		{
			logMsg("init touch config menu");
			auto &tcMenu = *menuAllocator.allocNew<TouchConfigView>(touchConfigFaceBtnName, touchConfigCenterBtnName);
			tcMenu.init(!e.isPointer());
			viewStack.pushAndShow(&tcMenu, &menuAllocator);
		}
	},
	#endif
	altGamepadConfirm
	{
		"Alt Gamepad Confirm",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			Input::swappedGamepadConfirm = item.on;
		}
	},
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	btScanSecs
	{
		"Bluetooth Scan",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bcase 0: BluetoothAdapter::scanSecs = 2;
				bcase 1: BluetoothAdapter::scanSecs = 4;
				bcase 2: BluetoothAdapter::scanSecs = 6;
				bcase 3: BluetoothAdapter::scanSecs = 8;
				bcase 4: BluetoothAdapter::scanSecs = 10;
			}
			logMsg("set bluetooth scan time %d", BluetoothAdapter::scanSecs);
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH
	keepBtActive
	{
		"Background Bluetooth",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionKeepBluetoothActive = item.on;
		}
	},
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	btScanCache
	{
		"Bluetooth Scan Cache",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionBlueToothScanCache = item.on;
		}
	},
	#endif
	relativePointerDecel
	{
		"Trackball Sensitivity",
		[](MultiChoiceMenuItem &, int val)
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
	},
	// System
	autoSaveState
	{
		"Auto-save State",
		[](MultiChoiceMenuItem &, int val)
		{
			switch(val)
			{
				bcase 0: optionAutoSaveState.val = 0;
				bcase 1: optionAutoSaveState.val = 1;
				bcase 2: optionAutoSaveState.val = 15;
				bcase 3: optionAutoSaveState.val = 30;
			}
			logMsg("set auto-savestate %d", optionAutoSaveState.val);
		}
	},
	confirmAutoLoadState
	{
		"Confirm Auto-load State",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionConfirmAutoLoadState = item.on;
		}
	},
	confirmOverwriteState
	{
		"Confirm Overwrite State",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionConfirmOverwriteState = item.on;
		}
	},
	savePath
	{
		"",
		[this](TextMenuItem &, const Input::Event &e)
		{
			pathSelectMenu.init(!e.isPointer());
			pathSelectMenu.onPathChange =
				[this](const char *newPath)
				{
					printPathMenuEntryStr(savePathStr);
					savePath.compile();
					EmuSystem::savePathChanged();
				};
			Base::displayNeedsUpdate();
		}
	},
	#if defined CONFIG_BASE_ANDROID && CONFIG_ENV_ANDROID_MINSDK >= 9
	processPriority
	{
		"Process Priority",
		[](MultiChoiceMenuItem &, int val)
		{
			if(val == 0)
				optionProcessPriority.val = 0;
			else if(val == 1)
				optionProcessPriority.val = -6;
			else if(val == 2)
				optionProcessPriority.val = -14;
			Base::setProcessPriority(optionProcessPriority);
		}
	},
	#endif
	statusBar
	{
		"Hide Status Bar",
		[](MultiChoiceMenuItem &, int val)
		{
			optionHideStatusBar = val;
			setupStatusBarInMenu();
		}
	},
	// GUI
	pauseUnfocused
	{
		Config::envIsPS3 ? "Pause in XMB" : "Pause if unfocused",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionPauseUnfocused = item.on;
		}
	},
	fontSize
	{
		"Font Size",
		[](MultiChoiceMenuItem &, int val)
		{
			optionFontSize = 3000;
			switch(val)
			{
				bcase 0: optionFontSize = 2000;
				bcase 1: optionFontSize = 2500;
				// 2: 3000
				bcase 3: optionFontSize = 3500;
				bcase 4: optionFontSize = 4000;
				bcase 5: optionFontSize = 4500;
				bcase 6: optionFontSize = 5000;
				bcase 7: optionFontSize = 5500;
				bcase 8: optionFontSize = 6000;
				bcase 9: optionFontSize = 6500;
				bcase 10: optionFontSize = 7000;
				bcase 11: optionFontSize = 7500;
				bcase 12: optionFontSize = 8000;
				bcase 13: optionFontSize = 8500;
				bcase 14: optionFontSize = 9000;
				bcase 15: optionFontSize = 9500;
				bcase 16: optionFontSize = 10000;
				bcase 17: optionFontSize = 10500;
			}
			setupFont();
			Gfx::onViewChange(nullptr);
		}
	},
	notificationIcon
	{
		"Suspended App Icon",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionNotificationIcon = item.on;
		}
	},
	lowProfileOSNav
	{
		"Dim OS Navigation",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionLowProfileOSNav = item.on;
			applyOSNavStyle();
		}
	},
	hideOSNav
	{
		"Hide OS Navigation",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionHideOSNav = item.on;
			applyOSNavStyle();
		}
	},
	idleDisplayPowerSave
	{
		"Dim Screen If Idle",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionIdleDisplayPowerSave = item.on;
			Base::setIdleDisplayPowerSave(item.on);
		}
	},
	navView
	{
		"Title Bar",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionTitleBar = item.on;
			viewStack.setNavView(item.on ? &viewNav : 0);
			Gfx::onViewChange(nullptr);
		}
	},
	backNav
	{
		"Title Back Navigation",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			View::setNeedsBackControl(item.on);
			viewNav.setBackImage(View::needsBackControl ? getArrowAsset() : nullptr);
			Gfx::onViewChange(nullptr);
		}
	},
	rememberLastMenu
	{
		"Remember Last Menu",
		[](BoolMenuItem &item, const Input::Event &e)
		{
			item.toggle();
			optionRememberLastMenu = item.on;
		}
	},
	menuOrientation
	{
		"Orientation",
		[](MultiChoiceMenuItem &, int val)
		{
			optionMenuOrientation.val = convertOrientationMenuValueToOption(val);
			if(!Gfx::setValidOrientations(optionMenuOrientation, 1))
				Gfx::onViewChange(nullptr);
			logMsg("set menu orientation: %s", Gfx::orientationName(int(optionMenuOrientation)));
		}
	}
{}
