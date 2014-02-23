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

#include <gui/View.hh>
#include <gui/AlertView.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <gui/BaseMenuView.hh>
#include <audio/Audio.hh>
#include <EmuInput.hh>
#include <EmuOptions.hh>
#include <MultiChoiceView.hh>
#include <EmuApp.hh>
#include <FilePicker.hh>
extern WorkDirStack<1> workDirStack;
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
	BoolMenuItem glSyncHack;
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
	MultiChoiceSelectMenuItem imgFilter;
	void imgFilterInit();
	MultiChoiceSelectMenuItem overlayEffect;
	void overlayEffectInit();
	MultiChoiceSelectMenuItem overlayEffectLevel;
	void overlayEffectLevelInit();
	#if defined (CONFIG_BASE_X11) || defined (CONFIG_BASE_ANDROID) || defined (CONFIG_BASE_IOS)
	BoolMenuItem bestColorModeHint;
	void bestColorModeHintHandler(BoolMenuItem &item, const Input::Event &e);
	#endif
	#ifdef CONFIG_BASE_MULTI_WINDOW
	BoolMenuItem secondDisplay;
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

	MenuItem *item[24] {nullptr};

public:
	OptionView(Base::Window &win);
	void init(uint idx, bool highlightFirst);
};

class BiosSelectMenu : public BaseMultiChoiceView
{
public:
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2] {nullptr};
	typedef DelegateFunc<void ()> BiosChangeDelegate;
	BiosChangeDelegate onBiosChangeD;
	FsSys::cPath *biosPathStr = nullptr;
	int (*fsFilter)(const char *name, int type) = nullptr;

	constexpr BiosSelectMenu(const char *name, Base::Window &win): BaseMultiChoiceView(name, win) {}
	constexpr BiosSelectMenu(const char *name, FsSys::cPath *biosPathStr, int (*fsFilter)(const char *name, int type), Base::Window &win):
		BaseMultiChoiceView(name, win), biosPathStr(biosPathStr), fsFilter(fsFilter) {}
	BiosChangeDelegate &onBiosChange() { return onBiosChangeD; };
	void onSelectFile(const char* name, const Input::Event &e);
	void init(FsSys::cPath *biosPathStr, int (*fsFilter)(const char *name, int type), bool highlightFirst);
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
