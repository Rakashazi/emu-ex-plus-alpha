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

#include <imagine/gui/View.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/BaseMenuView.hh>
#include <imagine/audio/Audio.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <MultiChoiceView.hh>
#include <EmuApp.hh>
#include <FilePicker.hh>
void onCloseModalPopWorkDir(const Input::Event &e);
void chdirFromFilePath(const char *path);

class OptionView : public BaseMenuView
{
protected:
	// Video
	#ifdef CONFIG_BASE_ANDROID
		#ifdef SUPPORT_ANDROID_DIRECT_TEXTURE
		BoolMenuItem directTexture;
		#endif
		#if CONFIG_ENV_ANDROID_MINSDK >= 9
		BoolMenuItem surfaceTexture;
		#endif
	#endif
	MultiChoiceSelectMenuItem frameSkip;
	void frameSkipInit();
	const char *aspectRatioStr[4];
	MultiChoiceSelectMenuItem aspectRatio;
	void aspectRatioInit();
	MultiChoiceSelectMenuItem zoom;
	void zoomInit();
	MultiChoiceSelectMenuItem viewportZoom;
	void viewportZoomInit();
	BoolMenuItem imgFilter;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	MultiChoiceSelectMenuItem imgEffect;
	void imgEffectInit();
	#endif
	MultiChoiceSelectMenuItem overlayEffect;
	void overlayEffectInit();
	MultiChoiceSelectMenuItem overlayEffectLevel;
	void overlayEffectLevelInit();
	#if defined EMU_FRAMEWORK_BEST_COLOR_MODE_OPTION
	BoolMenuItem bestColorModeHint;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	BoolMenuItem secondDisplay;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	BoolMenuItem showOnSecondScreen;
	#endif
	BoolMenuItem dither;
	MultiChoiceSelectMenuItem gameOrientation;
	void gameOrientationInit();

	// Audio
	BoolMenuItem snd;
	#ifdef CONFIG_AUDIO_LATENCY_HINT
	MultiChoiceSelectMenuItem soundBuffers;
	void soundBuffersInit();
	#endif
	MultiChoiceSelectMenuItem audioRate;
	void audioRateInit();
	#ifdef CONFIG_AUDIO_OPENSL_ES
	BoolMenuItem sndUnderrunCheck;
	#endif
	#ifdef CONFIG_AUDIO_SOLO_MIX
	BoolMenuItem audioSoloMix;
	#endif

	// Input
	BoolMenuItem altGamepadConfirm;
	#ifdef CONFIG_BLUETOOTH_SCAN_SECS
	MultiChoiceSelectMenuItem btScanSecs;
	void btScanSecsInit();
	#endif
	#ifdef CONFIG_BLUETOOTH
	BoolMenuItem keepBtActive;
	#endif
	#ifdef CONFIG_BLUETOOTH_SCAN_CACHE_USAGE
	BoolMenuItem btScanCache;
	#endif
	#ifdef CONFIG_BASE_ANDROID
	MultiChoiceSelectMenuItem relativePointerDecel;
	void relativePointerDecelInit();
	#endif

	// System
	MultiChoiceSelectMenuItem autoSaveState;
	void autoSaveStateInit();
	BoolMenuItem confirmAutoLoadState;
	BoolMenuItem confirmOverwriteState;
	void savePathUpdated(const char *newPath);
	char savePathStr[256] {0};
	TextMenuItem savePath;
	BoolMenuItem checkSavePathWriteAccess;
	static constexpr uint MIN_FAST_FORWARD_SPEED = 2;
	void fastForwardSpeedinit();
	MultiChoiceSelectMenuItem fastForwardSpeed;
	#if defined CONFIG_BASE_ANDROID
	void processPriorityInit();
	MultiChoiceSelectMenuItem processPriority;
	#endif

	// GUI
	BoolMenuItem pauseUnfocused;
	MultiChoiceSelectMenuItem fontSize;
	void fontSizeInit();
	BoolMenuItem notificationIcon;
	MultiChoiceSelectMenuItem statusBar;
	MultiChoiceSelectMenuItem lowProfileOSNav;
	MultiChoiceSelectMenuItem hideOSNav;
	BoolMenuItem idleDisplayPowerSave;
	BoolMenuItem navView;
	BoolMenuItem backNav;
	BoolMenuItem rememberLastMenu;
	#ifdef EMU_FRAMEWORK_BUNDLED_GAMES
	BoolMenuItem showBundledGames;
	#endif
	MultiChoiceSelectMenuItem menuOrientation;
	void menuOrientationInit();

	virtual void loadVideoItems(MenuItem *item[], uint &items);
	virtual void loadAudioItems(MenuItem *item[], uint &items);
	virtual void loadInputItems(MenuItem *item[], uint &items);
	virtual void loadSystemItems(MenuItem *item[], uint &items);
	virtual void loadGUIItems(MenuItem *item[], uint &items);

	MenuItem *item[25]{nullptr};

public:
	OptionView(Base::Window &win);
	void init(uint idx, bool highlightFirst);
};

class BiosSelectMenu : public BaseMultiChoiceView
{
public:
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2]{nullptr};
	typedef DelegateFunc<void ()> BiosChangeDelegate;
	BiosChangeDelegate onBiosChangeD;
	FsSys::PathString *biosPathStr = nullptr;
	int (*fsFilter)(const char *name, int type) = nullptr;

	BiosSelectMenu(const char *name, Base::Window &win): BaseMultiChoiceView(name, win) {}
	BiosSelectMenu(const char *name, FsSys::PathString *biosPathStr, int (*fsFilter)(const char *name, int type), Base::Window &win):
		BaseMultiChoiceView(name, win), biosPathStr(biosPathStr), fsFilter(fsFilter) {}
	BiosChangeDelegate &onBiosChange() { return onBiosChangeD; };
	void onSelectFile(const char* name, const Input::Event &e);
	void init(FsSys::PathString *biosPathStr, int (*fsFilter)(const char *name, int type), bool highlightFirst);
	void init(bool highlightFirst);
};

using PathChangeDelegate = DelegateFunc<void (const char *newPath)>;

class FirmwarePathSelector
{
public:
	PathChangeDelegate onPathChange;

	constexpr FirmwarePathSelector() {}
	void onClose(const Input::Event &e);
	void init(const char *name, bool highlightFirst);
};
