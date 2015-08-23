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
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MultiChoiceView.hh>
#include <imagine/audio/Audio.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/FilePicker.hh>
void onCloseModalPopWorkDir(Input::Event e);
void chdirFromFilePath(const char *path);

class OptionView : public TableView
{
protected:
	// Video
	#ifdef __ANDROID__
	void androidTextureStorageInit();
	MultiChoiceSelectMenuItem androidTextureStorage;
	#endif
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	MultiChoiceSelectMenuItem frameInterval;
	void frameIntervalInit();
	#endif
	BoolMenuItem dropLateFrames{};
	char frameRateStr[64]{};
	TextMenuItem frameRate;
	char frameRatePALStr[64]{};
	TextMenuItem frameRatePAL;
	const char *aspectRatioStr[4]{};
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
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	MultiChoiceSelectMenuItem imgEffectPixelFormat;
	void imgEffectPixelFormatInit();
	#endif
	#if defined EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	MultiChoiceSelectMenuItem windowPixelFormat;
	void windowPixelFormatInit();
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	BoolMenuItem secondDisplay;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	BoolMenuItem showOnSecondScreen;
	#endif
	BoolMenuItem dither;

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

	// System
	MultiChoiceSelectMenuItem autoSaveState;
	void autoSaveStateInit();
	BoolMenuItem confirmAutoLoadState;
	BoolMenuItem confirmOverwriteState;
	void savePathUpdated(const char *newPath);
	char savePathStr[256]{};
	TextMenuItem savePath;
	BoolMenuItem checkSavePathWriteAccess;
	static constexpr uint MIN_FAST_FORWARD_SPEED = 2;
	void fastForwardSpeedinit();
	MultiChoiceSelectMenuItem fastForwardSpeed;
	#if defined __ANDROID__
	void processPriorityInit();
	MultiChoiceSelectMenuItem processPriority;
	BoolMenuItem manageCPUFreq;
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
	BoolMenuItem showBundledGames;
	TextHeadingMenuItem orientationHeading;
	MultiChoiceSelectMenuItem menuOrientation;
	void menuOrientationInit();
	MultiChoiceSelectMenuItem gameOrientation;
	void gameOrientationInit();

	virtual void loadVideoItems(MenuItem *item[], uint &items);
	virtual void loadAudioItems(MenuItem *item[], uint &items);
	virtual void loadInputItems(MenuItem *item[], uint &items);
	virtual void loadSystemItems(MenuItem *item[], uint &items);
	virtual void loadGUIItems(MenuItem *item[], uint &items);

	MenuItem *item[27]{};

public:
	OptionView(Base::Window &win);
	void init(uint idx);
};

class BiosSelectMenu : public BaseMultiChoiceView
{
public:
	TextMenuItem choiceEntry[2];
	MenuItem *choiceEntryItem[2]{};
	typedef DelegateFunc<void ()> BiosChangeDelegate;
	BiosChangeDelegate onBiosChangeD;
	FS::PathString *biosPathStr{};
	EmuNameFilterFunc fsFilter{};

	BiosSelectMenu(const char *name, Base::Window &win): BaseMultiChoiceView(name, win) {}
	BiosSelectMenu(const char *name, FS::PathString *biosPathStr, EmuNameFilterFunc fsFilter, Base::Window &win):
		BaseMultiChoiceView(name, win), biosPathStr(biosPathStr), fsFilter(fsFilter) {}
	BiosChangeDelegate &onBiosChange() { return onBiosChangeD; };
	void onSelectFile(const char* name, Input::Event e);
	void init(FS::PathString *biosPathStr, EmuNameFilterFunc fsFilter);
	void init();
};

using PathChangeDelegate = DelegateFunc<void (const char *newPath)>;

class FirmwarePathSelector
{
public:
	PathChangeDelegate onPathChange;

	constexpr FirmwarePathSelector() {}
	void onClose(Input::Event e);
	void init(const char *name, Input::Event e);
};
