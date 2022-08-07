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

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/util/container/ArrayList.hh>
#include <memory>

namespace IG
{
class TextTableView;
}

namespace IG::Gfx
{
struct DrawableConfig;
}

namespace EmuEx
{

using namespace IG;
class EmuVideoLayer;
class EmuAudio;
enum class ImageEffectId : uint8_t;

class OptionCategoryView : public TableView, public EmuAppHelper<OptionCategoryView>
{
public:
	OptionCategoryView(ViewAttachParams attach, EmuAudio &audio, EmuVideoLayer &videoLayer);

protected:
	TextMenuItem subConfig[6];
};

class VideoOptionView : public TableView, public EmuAppHelper<VideoOptionView>
{
public:
	VideoOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();
	void setEmuVideoLayer(EmuVideoLayer &videoLayer);

protected:
	static constexpr int MAX_ASPECT_RATIO_ITEMS = 5;
	EmuVideoLayer *videoLayer{};

	StaticArrayList<TextMenuItem, 5> textureBufferModeItem{};
	MultiChoiceMenuItem textureBufferMode;
	IG_UseMemberIf(Config::SCREEN_FRAME_INTERVAL, TextMenuItem, frameIntervalItem[4]);
	IG_UseMemberIf(Config::SCREEN_FRAME_INTERVAL, MultiChoiceMenuItem, frameInterval);
	BoolMenuItem dropLateFrames;
	TextMenuItem frameRate;
	TextMenuItem frameRatePAL;
	StaticArrayList<TextMenuItem, MAX_ASPECT_RATIO_ITEMS> aspectRatioItem;
	MultiChoiceMenuItem aspectRatio;
	TextMenuItem zoomItem[6];
	MultiChoiceMenuItem zoom;
	TextMenuItem viewportZoomItem[4];
	MultiChoiceMenuItem viewportZoom;
	TextMenuItem contentRotationItem[5];
	MultiChoiceMenuItem contentRotation;
	BoolMenuItem imgFilter;
	TextMenuItem imgEffectItem[4];
	MultiChoiceMenuItem imgEffect;
	TextMenuItem overlayEffectItem[6];
	MultiChoiceMenuItem overlayEffect;
	TextMenuItem overlayEffectLevelItem[5];
	MultiChoiceMenuItem overlayEffectLevel;
	TextMenuItem imgEffectPixelFormatItem[3];
	MultiChoiceMenuItem imgEffectPixelFormat;
	StaticArrayList<TextMenuItem, 4> windowPixelFormatItem{};
	MultiChoiceMenuItem windowPixelFormat;
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_X11
	BoolMenuItem secondDisplay;
	#endif
	#if defined CONFIG_BASE_MULTI_WINDOW && defined CONFIG_BASE_MULTI_SCREEN
	BoolMenuItem showOnSecondScreen;
	#endif
	TextMenuItem imageBuffersItem[3];
	MultiChoiceMenuItem imageBuffers;
	TextMenuItem renderPixelFormatItem[3];
	MultiChoiceMenuItem renderPixelFormat;
	IG_UseMemberIf(Config::envIsAndroid, BoolMenuItem, presentationTime);
	TextHeadingMenuItem visualsHeading;
	TextHeadingMenuItem screenShapeHeading;
	TextHeadingMenuItem advancedHeading;
	TextHeadingMenuItem systemSpecificHeading;
	StaticArrayList<MenuItem*, 32> item{};

	void pushAndShowFrameRateSelectMenu(VideoSystem, const Input::Event &);
	bool onFrameTimeChange(VideoSystem vidSys, IG::FloatSeconds time);
	TextMenuItem::SelectDelegate setZoomDel();
	TextMenuItem::SelectDelegate setViewportZoomDel();
	TextMenuItem::SelectDelegate setContentRotationDel();
	TextMenuItem::SelectDelegate setFrameIntervalDel();
	TextMenuItem::SelectDelegate setImgEffectDel();
	TextMenuItem::SelectDelegate setOverlayEffectDel();
	TextMenuItem::SelectDelegate setOverlayEffectLevelDel();
	TextMenuItem::SelectDelegate setRenderPixelFormatDel();
	TextMenuItem::SelectDelegate setImgEffectPixelFormatDel();
	TextMenuItem::SelectDelegate setWindowDrawableConfigDel(Gfx::DrawableConfig);
	TextMenuItem::SelectDelegate setImageBuffersDel();
	EmuVideo &emuVideo() const;
};

class AudioOptionView : public TableView, public EmuAppHelper<AudioOptionView>
{
public:
	AudioOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	static constexpr int MAX_APIS = 2;

	BoolMenuItem snd;
	BoolMenuItem soundDuringFastSlowMode;
	TextMenuItem soundVolumeItem[4];
	MultiChoiceMenuItem soundVolume;
	TextMenuItem soundBuffersItem[7];
	MultiChoiceMenuItem soundBuffers;
	BoolMenuItem addSoundBuffersOnUnderrun;
	StaticArrayList<TextMenuItem, 5> audioRateItem{};
	MultiChoiceMenuItem audioRate;
	IG_UseMemberIf(IG::Audio::Manager::HAS_SOLO_MIX, BoolMenuItem, audioSoloMix);
	using ApiItemContainer = StaticArrayList<TextMenuItem, MAX_APIS + 1>;
	IG_UseMemberIf(IG::Audio::Config::MULTIPLE_SYSTEM_APIS, ApiItemContainer, apiItem);
	IG_UseMemberIf(IG::Audio::Config::MULTIPLE_SYSTEM_APIS, MultiChoiceMenuItem, api);
	StaticArrayList<MenuItem*, 22> item{};

	TextMenuItem::SelectDelegate setRateDel();
	TextMenuItem::SelectDelegate setBuffersDel();
	TextMenuItem::SelectDelegate setVolumeDel();
};

class SystemOptionView : public TableView, public EmuAppHelper<SystemOptionView>
{
public:
	SystemOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem autoSaveStateItem[4];
	MultiChoiceMenuItem autoSaveState;
	BoolMenuItem confirmAutoLoadState;
	BoolMenuItem confirmOverwriteState;
	TextMenuItem fastSlowModeSpeedItem[8];
	MultiChoiceMenuItem fastSlowModeSpeed;
	IG_UseMemberIf(Config::envIsAndroid, BoolMenuItem, performanceMode);
	StaticArrayList<MenuItem*, 24> item{};

	TextMenuItem::SelectDelegate setAutoSaveStateDel();
	TextMenuItem::SelectDelegate setFastSlowModeSpeedDel();
};

class FilePathOptionView : public TableView, public EmuAppHelper<FilePathOptionView>
{
public:
	FilePathOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	TextMenuItem savePath;
	StaticArrayList<MenuItem*, 6> item{};

	void onSavePathChange(std::string_view path);
	virtual bool onFirmwarePathChange(IG::CStringView path, bool isDir);
	std::unique_ptr<TextTableView> makeFirmwarePathMenu(IG::utf16String name, bool allowFiles = false, int extraItemsHint = 0);
	void pushAndShowFirmwarePathMenu(IG::utf16String name, const Input::Event &, bool allowFiles = false);
	void pushAndShowFirmwareFilePathMenu(IG::utf16String name, const Input::Event &);
};

class GUIOptionView : public TableView, public EmuAppHelper<GUIOptionView>
{
public:
	GUIOptionView(ViewAttachParams attach, bool customMenu = false);
	void loadStockItems();

protected:
	BoolMenuItem pauseUnfocused;
	TextMenuItem fontSizeItem[10];
	MultiChoiceMenuItem fontSize;
	BoolMenuItem notificationIcon;
	IG_UseMemberIf(Config::STATUS_BAR, TextMenuItem, statusBarItem[3]);
	IG_UseMemberIf(Config::STATUS_BAR, MultiChoiceMenuItem, statusBar);
	IG_UseMemberIf(Config::NAVIGATION_BAR, TextMenuItem, lowProfileOSNavItem[3]);
	IG_UseMemberIf(Config::NAVIGATION_BAR, MultiChoiceMenuItem, lowProfileOSNav);
	IG_UseMemberIf(Config::NAVIGATION_BAR, TextMenuItem, hideOSNavItem[3]);
	IG_UseMemberIf(Config::NAVIGATION_BAR, MultiChoiceMenuItem, hideOSNav);
	BoolMenuItem idleDisplayPowerSave;
	IG_UseMemberIf(CAN_HIDE_TITLE_BAR, BoolMenuItem, navView);
	BoolMenuItem backNav;
	BoolMenuItem systemActionsIsDefaultMenu;
	BoolMenuItem showBundledGames;
	BoolMenuItem showBluetoothScan;
	BoolMenuItem showHiddenFiles;
	TextHeadingMenuItem orientationHeading;
	TextMenuItem menuOrientationItem[5];
	MultiChoiceMenuItem menuOrientation;
	TextMenuItem emuOrientationItem[5];
	MultiChoiceMenuItem emuOrientation;
	IG_UseMemberIf(Config::TRANSLUCENT_SYSTEM_UI, BoolMenuItem, layoutBehindSystemUI);
	StaticArrayList<MenuItem*, 22> item{};

	TextMenuItem::SelectDelegate setFontSizeDel();
	TextMenuItem::SelectDelegate setMenuOrientationDel();
	TextMenuItem::SelectDelegate setEmuOrientationDel();
	TextMenuItem::SelectDelegate setStatusBarDel();
	TextMenuItem::SelectDelegate setLowProfileOSNavDel();
	TextMenuItem::SelectDelegate setHideOSNavDel();
};

class BiosSelectMenu : public TableView, public EmuAppHelper<BiosSelectMenu>
{
public:
	using BiosChangeDelegate = DelegateFunc<void (std::string_view displayName)>;

	BiosSelectMenu(IG::utf16String name, ViewAttachParams attach, FS::PathString *biosPathStr, BiosChangeDelegate onBiosChange,
		EmuSystem::NameFilterFunc fsFilter);

protected:
	TextMenuItem selectFile{};
	TextMenuItem unset{};
	BiosChangeDelegate onBiosChangeD{};
	FS::PathString *biosPathStr{};
	EmuSystem::NameFilterFunc fsFilter{};
};

}
