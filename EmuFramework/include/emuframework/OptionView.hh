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

#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/audio/AudioManager.hh>
#include <imagine/audio/defs.hh>
#include <imagine/util/container/ArrayList.hh>
#include <emuframework/EmuSystem.hh>

class OptionCategoryView : public TableView
{
public:
	OptionCategoryView(ViewAttachParams attach);

protected:
	TextMenuItem subConfig[5];
};

class VideoOptionView : public TableView
{
public:
	VideoOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	static constexpr uint MAX_ASPECT_RATIO_ITEMS = 5;

	StaticArrayList<TextMenuItem, 5> textureBufferModeItem{};
	MultiChoiceMenuItem textureBufferMode;
	#if defined CONFIG_BASE_SCREEN_FRAME_INTERVAL
	TextMenuItem frameIntervalItem[4];
	MultiChoiceMenuItem frameInterval;
	#endif
	BoolMenuItem dropLateFrames;
	TextMenuItem frameRate;
	TextMenuItem frameRatePAL;
	StaticArrayList<TextMenuItem, MAX_ASPECT_RATIO_ITEMS> aspectRatioItem;
	MultiChoiceMenuItem aspectRatio;
	TextMenuItem zoomItem[6];
	MultiChoiceMenuItem zoom;
	TextMenuItem viewportZoomItem[4];
	MultiChoiceMenuItem viewportZoom;
	BoolMenuItem imgFilter;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	TextMenuItem imgEffectItem[4];
	MultiChoiceMenuItem imgEffect;
	#endif
	TextMenuItem overlayEffectItem[6];
	MultiChoiceMenuItem overlayEffect;
	TextMenuItem overlayEffectLevelItem[5];
	MultiChoiceMenuItem overlayEffectLevel;
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	TextMenuItem imgEffectPixelFormatItem[3];
	MultiChoiceMenuItem imgEffectPixelFormat;
	#endif
	#if defined EMU_FRAMEWORK_WINDOW_PIXEL_FORMAT_OPTION
	TextMenuItem windowPixelFormatItem[5];
	MultiChoiceMenuItem windowPixelFormat;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	BoolMenuItem secondDisplay;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	BoolMenuItem showOnSecondScreen;
	#endif
	TextMenuItem gpuMultithreadingItem[3];
	MultiChoiceMenuItem gpuMultithreading;
	TextHeadingMenuItem visualsHeading;
	TextHeadingMenuItem screenShapeHeading;
	TextHeadingMenuItem advancedHeading;
	TextHeadingMenuItem systemSpecificHeading;
	StaticArrayList<MenuItem*, 28> item{};

	void pushAndShowFrameRateSelectMenu(EmuSystem::VideoSystem vidSys, Input::Event e);
	bool onFrameTimeChange(EmuSystem::VideoSystem vidSys, IG::FloatSeconds time);
	void setOverlayEffectLevel(uint8_t val);
	void setZoom(uint8_t val);
	void setViewportZoom(uint8_t val);
	void setAspectRatio(double val);
	unsigned idxOfBufferMode(Gfx::TextureBufferMode mode);
};

class AudioOptionView : public TableView
{
public:
	AudioOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	static constexpr unsigned MAX_APIS = 2;

	BoolMenuItem snd;
	BoolMenuItem soundDuringFastForward;
	TextMenuItem soundVolumeItem[4];
	MultiChoiceMenuItem soundVolume;
	TextMenuItem soundBuffersItem[7];
	MultiChoiceMenuItem soundBuffers;
	BoolMenuItem addSoundBuffersOnUnderrun;
	StaticArrayList<TextMenuItem, 5> audioRateItem{};
	MultiChoiceMenuItem audioRate;
	#ifdef CONFIG_AUDIO_MANAGER_SOLO_MIX
	BoolMenuItem audioSoloMix;
	#endif
	#ifdef CONFIG_AUDIO_MULTIPLE_SYSTEM_APIS
	StaticArrayList<TextMenuItem, MAX_APIS + 1> apiItem{};
	MultiChoiceMenuItem api;
	#endif
	StaticArrayList<MenuItem*, 15> item{};

	void updateAudioRateItem();
	unsigned idxOfAPI(IG::Audio::Api api);
};

class SystemOptionView : public TableView
{
public:
	SystemOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem autoSaveStateItem[4];
	MultiChoiceMenuItem autoSaveState;
	BoolMenuItem confirmAutoLoadState;
	BoolMenuItem confirmOverwriteState;
	TextMenuItem savePath;
	BoolMenuItem checkSavePathWriteAccess;
	static constexpr uint MIN_FAST_FORWARD_SPEED = 2;
	TextMenuItem fastForwardSpeedItem[6];
	MultiChoiceMenuItem fastForwardSpeed;
	#if defined __ANDROID__
	TextMenuItem processPriorityItem[3];
	MultiChoiceMenuItem processPriority;
	BoolMenuItem performanceMode;
	#endif
	StaticArrayList<MenuItem*, 24> item{};

	void onSavePathChange(const char *path);
	virtual void onFirmwarePathChange(const char *path, Input::Event e);
	void pushAndShowFirmwarePathMenu(const char *name, Input::Event e, bool allowFiles = false);
	void pushAndShowFirmwareFilePathMenu(const char *name, Input::Event e);
};

class GUIOptionView : public TableView
{
public:
	GUIOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	BoolMenuItem pauseUnfocused;
	TextMenuItem fontSizeItem[10];
	MultiChoiceMenuItem fontSize;
	BoolMenuItem notificationIcon;
	TextMenuItem statusBarItem[3];
	MultiChoiceMenuItem statusBar;
	TextMenuItem lowProfileOSNavItem[3];
	MultiChoiceMenuItem lowProfileOSNav;
	TextMenuItem hideOSNavItem[3];
	MultiChoiceMenuItem hideOSNav;
	BoolMenuItem idleDisplayPowerSave;
	BoolMenuItem navView;
	BoolMenuItem backNav;
	BoolMenuItem systemActionsIsDefaultMenu;
	BoolMenuItem showBundledGames;
	BoolMenuItem showBluetoothScan;
	TextHeadingMenuItem orientationHeading;
	TextMenuItem menuOrientationItem[Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 5 : 4];
	MultiChoiceMenuItem menuOrientation;
	TextMenuItem gameOrientationItem[Config::BASE_SUPPORTS_ORIENTATION_SENSOR ? 5 : 4];
	MultiChoiceMenuItem gameOrientation;
	StaticArrayList<MenuItem*, 20> item{};

	void setFontSize(uint16_t val);
};

class BiosSelectMenu : public TableView
{
public:
	using BiosChangeDelegate = DelegateFunc<void ()>;

	BiosSelectMenu(NameString name, ViewAttachParams attach, FS::PathString *biosPathStr, BiosChangeDelegate onBiosChange,
		EmuSystem::NameFilterFunc fsFilter);

protected:
	TextMenuItem selectFile{};
	TextMenuItem unset{};
	BiosChangeDelegate onBiosChangeD{};
	FS::PathString *biosPathStr{};
	EmuSystem::NameFilterFunc fsFilter{};
};
