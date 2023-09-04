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

#include <emuframework/config.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuSystemTask.hh>
#include <emuframework/EmuAudio.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/EmuVideoLayer.hh>
#include <emuframework/EmuViewController.hh>
#include <emuframework/EmuInput.hh>
#include <emuframework/VController.hh>
#include <emuframework/Option.hh>
#include <emuframework/AutosaveManager.hh>
#include <emuframework/OutputTimingManager.hh>
#include <emuframework/RecentContent.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/input/android/MogaManager.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/VibrationManager.hh>
#include <imagine/base/PerformanceHintManager.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Vec3.hh>
#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/data-type/image/PixmapWriter.hh>
#include <imagine/font/Font.hh>
#include <imagine/util/used.hh>
#include <imagine/util/enum.hh>
#include <cstring>
#include <optional>
#include <span>
#include <string>

namespace IG
{
class BluetoothAdapter;
class FileIO;
class BasicNavView;
}

namespace EmuEx
{

struct MainWindowData;
class EmuMainMenuView;

enum class Tristate : uint8_t
{
	OFF, IN_EMU, ON
};

WISE_ENUM_CLASS((AssetFileID, size_t),
	ui,
	gamepadOverlay,
	keyboardOverlay);

WISE_ENUM_CLASS((AssetID, size_t),
	arrow,
	accept,
	close,
	more,
	fast,
	slow,
	speed,
	menu,
	leftSwitch,
	rightSwitch,
	load,
	save,
	display,
	screenshot,
	openFile,
	gamepadOverlay,
	keyboardOverlay);

constexpr const char *assetFilename[wise_enum::size<AssetFileID>]
{
	"ui.png",
	"gpOverlay.png",
	"kbOverlay.png",
};

struct AssetDesc
{
	AssetFileID fileID;
	FRect texBounds;
	WSize aspectRatio{1, 1};

	constexpr size_t fileIdx() const { return to_underlying(fileID); }
	constexpr auto filename() const { return assetFilename[fileIdx()]; }
};

enum class ScanValueMode
{
	NORMAL, ALLOW_BLANK
};

WISE_ENUM_CLASS((ImageChannel, uint8_t),
	All,
	Red,
	Green,
	Blue);

enum class AltSpeedMode
{
	fast, slow
};

WISE_ENUM_CLASS((CPUAffinityMode, uint8_t),
	Auto, Any, Manual
);

constexpr float menuVideoBrightnessScale = .25f;

class EmuApp : public IG::Application
{
public:
	using CreateSystemCompleteDelegate = DelegateFunc<void (const Input::Event &)>;
	using NavView = BasicNavView;

	enum class ViewID
	{
		MAIN_MENU,
		SYSTEM_ACTIONS,
		VIDEO_OPTIONS,
		AUDIO_OPTIONS,
		SYSTEM_OPTIONS,
		FILE_PATH_OPTIONS,
		GUI_OPTIONS,
		EDIT_CHEATS,
		LIST_CHEATS,
	};

	// Static app configuration
	static bool hasIcon;
	static bool needsGlobalInstance;

	EmuApp(IG::ApplicationInitParams, IG::ApplicationContext &);

	// required sub-class API functions
	AssetDesc vControllerAssetDesc(KeyInfo) const;
	static std::span<const KeyCategory> keyCategories();
	static std::span<const KeyConfigDesc> defaultKeyConfigs();
	static std::string_view systemKeyCodeToString(KeyCode);

	// optional sub-class API functions
	bool willCreateSystem(ViewAttachParams, const Input::Event &);
	static bool allowsTurboModifier(KeyCode);

	void mainInitCommon(IG::ApplicationInitParams, IG::ApplicationContext);
	static void onCustomizeNavView(NavView &v);
	void createSystemWithMedia(IG::IO, CStringView path, std::string_view displayName,
		const Input::Event &, EmuSystemCreateParams, ViewAttachParams, CreateSystemCompleteDelegate);
	void closeSystem();
	void closeSystemWithoutSave();
	void reloadSystem(EmuSystemCreateParams params = {});
	void onSystemCreated();
	void promptSystemReloadDueToSetOption(ViewAttachParams, const Input::Event &, EmuSystemCreateParams params = {});
	void pushAndShowNewCollectTextInputView(ViewAttachParams, const Input::Event &,
		const char *msgText, const char *initialContent, CollectTextInputView::OnTextDelegate);
	void pushAndShowNewYesNoAlertView(ViewAttachParams, const Input::Event &,
		const char *label, const char *choice1, const char *choice2,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	void pushAndShowModalView(std::unique_ptr<View> v, const Input::Event &);
	void pushAndShowModalView(std::unique_ptr<View> v);
	void popModalViews();
	void popMenuToRoot();
	void showSystemActionsViewFromSystem(ViewAttachParams, const Input::Event &);
	void showLastViewFromSystem(ViewAttachParams, const Input::Event &);
	void showExitAlert(ViewAttachParams, const Input::Event &);
	void showEmulation();
	void startEmulation();
	void pauseEmulation();
	void showUI(bool updateTopView = true);
	void launchSystem(const Input::Event &);
	static bool hasArchiveExtension(std::string_view name);
	void unpostMessage();
	void printScreenshotResult(bool success);
	FS::PathString contentSavePath(std::string_view name) const;
	FS::PathString contentSaveFilePath(std::string_view ext) const;
	bool saveState(CStringView path);
	bool saveStateWithSlot(int slot);
	bool loadState(CStringView path);
	bool loadStateWithSlot(int slot);
	bool shouldOverwriteExistingState() const;
	const auto &contentSearchPath() const { return contentSearchPath_; }
	FS::PathString contentSearchPath(std::string_view name) const;
	void setContentSearchPath(std::string_view path);
	FS::PathString validSearchPath(const FS::PathString &) const;
	static void updateLegacySavePath(IG::ApplicationContext, CStringView path);
	auto screenshotDirectory() const { return system().userPath(userScreenshotPath); }
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	bool handleKeyInput(KeyInfo, const Input::Event &srcEvent);
	bool handleAppActionKeyInput(InputAction, const Input::Event &srcEvent);
	void handleSystemKeyInput(KeyInfo, Input::Action, uint32_t metaState = 0);
	void runTurboInputEvents();
	void resetInput();
	void setRunSpeed(double speed);
	void saveSessionOptions();
	void loadSessionOptions();
	bool hasSavedSessionOptions();
	void deleteSessionOptions();
	void syncEmulationThread();
	void startAudio();
	EmuAudio &audio() { return emuAudio; }
	EmuVideo &video() { return emuVideo; }
	EmuVideoLayer &videoLayer() { return emuVideoLayer; }
	EmuViewController &viewController();
	const EmuViewController &viewController() const;
	const Screen &emuScreen() const;
	Window &emuWindow();
	AutosaveManager &autosaveManager() { return autosaveManager_; }
	FrameTimeConfig configFrameTime();
	void setDisabledInputKeys(std::span<const KeyCode> keys);
	void unsetDisabledInputKeys();
	Gfx::TextureSpan asset(AssetID) const;
	Gfx::TextureSpan asset(AssetDesc) const;
	VController &defaultVController() { return inputManager.vController; }
	static std::unique_ptr<View> makeView(ViewAttachParams, ViewID);
	void applyOSNavStyle(IG::ApplicationContext, bool inGame);
	void setCPUNeedsLowLatency(IG::ApplicationContext, bool needed);
	void runFrames(EmuSystemTaskContext, EmuVideo *, EmuAudio *, int frames, bool skipForward);
	void skipFrames(EmuSystemTaskContext, int frames, EmuAudio *);
	bool skipForwardFrames(EmuSystemTaskContext, int frames);
	IG::Audio::Manager &audioManager() { return audioManager_; }
	void renderSystemFramebuffer(EmuVideo &);
	bool writeScreenshot(IG::PixmapView, CStringView path);
	FS::PathString makeNextScreenshotFilename();
	bool mogaManagerIsActive() const { return bool(mogaManagerPtr); }
	void setMogaManagerActive(bool on, bool notify);
	constexpr IG::VibrationManager &vibrationManager() { return vibrationManager_; }
	BluetoothAdapter *bluetoothAdapter();
	void closeBluetoothConnections();
	ViewAttachParams attachParams();
	auto &customKeyConfigList() { return inputManager.customKeyConfigs; };
	auto &savedInputDeviceList() { return inputManager.savedInputDevs; };
	IG::Viewport makeViewport(const Window &win) const;
	void setEmuViewOnExtraWindow(bool on, IG::Screen &);
	void record(FrameTimeStatEvent, SteadyClockTimePoint t = {});
	bool supportsPresentModes() const { return windowFrameTimeSource != WindowFrameTimeSource::RENDERER; }
	void setIntendedFrameRate(Window &, FrameTimeConfig);
	static std::u16string_view mainViewName();
	void runBenchmarkOneShot(EmuVideo &);
	void onSelectFileFromPicker(IG::IO, CStringView path, std::string_view displayName,
		const Input::Event &, EmuSystemCreateParams, ViewAttachParams);
	void handleOpenFileCommand(CStringView path);
	static bool hasGooglePlayStoreFeatures();
	EmuSystem &system();
	const EmuSystem &system() const;
	ApplicationContext appContext() const { return system().appContext(); }
	static EmuApp &get(ApplicationContext);
	MainWindowData &mainWindowData() const;

	// Video Options
	bool setWindowDrawableConfig(Gfx::DrawableConfig);
	Gfx::DrawableConfig windowDrawableConfig() const { return windowDrawableConf; }
	IG::PixelFormat windowPixelFormat() const;
	void setRenderPixelFormat(std::optional<IG::PixelFormat>);
	IG::PixelFormat renderPixelFormat() const { return renderPixelFmt; }
	bool setVideoAspectRatio(float val);
	float videoAspectRatio() const;
	float defaultVideoAspectRatio() const;
	auto &videoFilterOption() { return optionImgFilter; }
	auto &videoEffectOption() { return optionImgEffect; }
	IG::PixelFormat videoEffectPixelFormat() const;
	auto &videoEffectPixelFormatOption() { return optionImageEffectPixelFormat; }
	auto &overlayEffectOption() { return optionOverlayEffect; }
	bool setOverlayEffectLevel(EmuVideoLayer &, uint8_t val);
	uint8_t overlayEffectLevel() { return optionOverlayEffectLevel; }
	void setFrameInterval(int);
	int frameInterval() const;
	bool setVideoZoom(uint8_t val);
	uint8_t videoZoom() const { return optionImageZoom; }
	bool setViewportZoom(uint8_t val);
	uint8_t viewportZoom() { return optionViewportZoom; }
	auto &showOnSecondScreenOption() { return optionShowOnSecondScreen; }
	auto &textureBufferModeOption() { return optionTextureBufferMode; }
	auto &videoImageBuffersOption() { return optionVideoImageBuffers; }
	void setContentRotation(IG::Rotation);
	IG::Rotation contentRotation() const { return contentRotation_; }
	void updateContentRotation();
	float videoBrightness(ImageChannel) const;
	const Gfx::Vec3 &videoBrightnessAsRGB() const { return videoBrightnessRGB; }
	int videoBrightnessAsInt(ImageChannel ch) const { return videoBrightness(ch) * 100.f; }
	void setVideoBrightness(float brightness, ImageChannel);

	// System Options
	auto &confirmOverwriteStateOption() { return optionConfirmOverwriteState; }
	bool setAltSpeed(AltSpeedMode mode, int16_t speed);
	int16_t altSpeed(AltSpeedMode mode) const { return altSpeedRef(mode); }
	double altSpeedAsDouble(AltSpeedMode mode) const { return altSpeed(mode) / 100.; }
	void setCPUAffinity(int cpuNumber, bool on);
	bool cpuAffinity(int cpuNumber) const;
	void applyCPUAffinity(bool active);

	// GUI Options
	auto &pauseUnfocusedOption() { return optionPauseUnfocused; }
	auto &systemActionsIsDefaultMenuOption() { return optionSystemActionsIsDefaultMenu; }
	void setIdleDisplayPowerSave(bool on);
	bool idleDisplayPowerSave() const { return optionIdleDisplayPowerSave; }
	bool setFontSize(int size); // size in micro-meters
	int fontSize() const;
	void applyFontSize(Window &win);
	IG::FontSettings fontSettings(Window &win) const;
	void setShowsTitleBar(bool on);
	bool showsTitleBar() const { return optionTitleBar; };
	void setLowProfileOSNavMode(Tristate mode);
	void setHideOSNavMode(Tristate mode);
	void setHideStatusBarMode(Tristate mode);
	Tristate lowProfileOSNavMode() const { return (Tristate)(uint8_t)optionLowProfileOSNav; }
	Tristate hideOSNavMode() const { return (Tristate)(uint8_t)optionHideOSNav; }
	Tristate hideStatusBarMode() const { return (Tristate)(uint8_t)optionHideStatusBar; }
	void setEmuOrientation(Orientations);
	void setMenuOrientation(Orientations);
	Orientations emuOrientation() const { return optionEmuOrientation; }
	Orientations menuOrientation() const { return optionMenuOrientation; }
	void setShowsBundledGames(bool);
	bool showsBundledGames() const { return optionShowBundledGames; }
	auto &notificationIconOption() { return optionNotificationIcon; }
	void setShowsBluetoothScanItems(bool on);
	bool showsBluetoothScanItems() const { return optionShowBluetoothScan; }
	void setLayoutBehindSystemUI(bool);
	bool doesLayoutBehindSystemUI() const { return layoutBehindSystemUI; };

	void postMessage(UTF16Convertible auto &&msg)
	{
		postMessage(false, IG_forward(msg));
	}

	void postMessage(bool error, UTF16Convertible auto &&msg)
	{
		postMessage(3, error, IG_forward(msg));
	}

	void postMessage(int secs, bool error, UTF16Convertible auto &&msg)
	{
		viewController().popup.post(IG_forward(msg), secs, error);
	}

	void postErrorMessage(UTF16Convertible auto &&msg)
	{
		postMessage(true, IG_forward(msg));
	}

	void postErrorMessage(int secs, UTF16Convertible auto &&msg)
	{
		postMessage(secs, true, IG_forward(msg));
	}

	template <std::same_as<const char*> T>
	static std::pair<T, int> scanValue(const char *str, ScanValueMode mode)
	{
		return {str, mode == ScanValueMode::ALLOW_BLANK || strlen(str) ? 1 : 0};
	}

	template <std::integral T>
	static std::pair<T, int> scanValue(const char *str, ScanValueMode)
	{
		int val;
		int items = sscanf(str, "%d", &val);
		return {val, items};
	}

	template <std::floating_point T>
	static std::pair<T, int> scanValue(const char *str, ScanValueMode)
	{
		T val, denom;
		int items = sscanf(str, std::is_same_v<T, double> ? "%lf /%lf" : "%f /%f", &val, &denom);
		if(items > 1 && denom != 0)
		{
			val /= denom;
		}
		return {val, items};
	}

	template <class T>
	requires std::same_as<T, std::pair<float, float>> || std::same_as<T, std::pair<double, double>>
	static std::pair<T, int> scanValue(const char *str, ScanValueMode)
	{
		// special case for getting a fraction
		using PairValue = typename T::first_type;
		PairValue val, denom{};
		int items = sscanf(str, std::is_same_v<PairValue, double> ? "%lf /%lf" : "%f /%f", &val, &denom);
		if(denom == 0)
		{
			denom = 1.;
		}
		return {{val, denom}, items};
	}

	template <class T>
	requires std::same_as<T, std::pair<int, int>>
	static std::pair<T, int> scanValue(const char *str, ScanValueMode)
	{
		using PairValue = typename T::first_type;
		PairValue val, val2{};
		int items = sscanf(str, "%d %d", &val, &val2);
		return {{val, val2}, items};
	}

	template<class T, ScanValueMode mode = ScanValueMode::NORMAL>
	void pushAndShowNewCollectValueInputView(ViewAttachParams attach, const Input::Event &e,
		CStringView msgText, CStringView initialContent, IG::Callable<bool, EmuApp&, T> auto &&collectedValueFunc)
	{
		pushAndShowNewCollectTextInputView(attach, e, msgText, initialContent,
			[collectedValueFunc](CollectTextInputView &view, const char *str)
			{
				if(!str)
				{
					view.dismiss();
					return false;
				}
				auto &app = get(view.appContext());
				auto [val, items] = scanValue<T>(str, mode);
				if(items <= 0)
				{
					app.postErrorMessage("Enter a value");
					return true;
				}
				else if(!collectedValueFunc(app, val))
				{
					return true;
				}
				else
				{
					view.dismiss();
					return false;
				}
			});
	}

	template<class T, T low, T high>
	void pushAndShowNewCollectValueRangeInputView(ViewAttachParams attach, const Input::Event &e,
		CStringView msgText, CStringView initialContent, IG::Callable<bool, EmuApp&, T> auto &&collectedValueFunc)
	{
		pushAndShowNewCollectValueInputView<T>(attach, e, msgText, initialContent,
			[collectedValueFunc](EmuApp &app, auto val)
			{
				if(val >= low && val <= high)
				{
					return collectedValueFunc(app, val);
				}
				else
				{
					app.postErrorMessage("Value not in range");
					return false;
				}
			});
	}

	template<class T, T low, T high, T low2, T high2>
	void pushAndShowNewCollectValuePairRangeInputView(ViewAttachParams attach, const Input::Event &e,
		CStringView msgText, CStringView initialContent, Callable<bool, EmuApp&, std::pair<T, T>> auto &&collectedValueFunc)
	{
		pushAndShowNewCollectValueInputView<std::pair<T, T>>(attach, e, msgText, initialContent,
			[collectedValueFunc](EmuApp &app, auto val)
			{
				if(val.first >= low && val.first <= high && val.second >= low2 && val.second <= high2)
				{
					return collectedValueFunc(app, val);
				}
				else
				{
					app.postErrorMessage("Values not in range");
					return false;
				}
			});
	}

protected:
	IG::FontManager fontManager;
	mutable Gfx::Renderer renderer;
	ViewManager viewManager;
	IG::Audio::Manager audioManager_;
	EmuAudio emuAudio;
	EmuVideo emuVideo;
	EmuVideoLayer emuVideoLayer;
	EmuSystemTask emuSystemTask;
	mutable Gfx::Texture assetBuffImg[wise_enum::size<AssetFileID>];
	AutosaveManager autosaveManager_;
public:
	InputManager inputManager;
	OutputTimingManager outputTimingManager;
protected:
	IG_UseMemberIf(enableFrameTimeStats, FrameTimeStats, frameTimeStats);
	IG_UseMemberIf(Config::threadPerformanceHints, SteadyClockTimePoint, frameStartTimePoint){};
	Gfx::Vec3 videoBrightnessRGB{1.f, 1.f, 1.f};
	FS::PathString contentSearchPath_;
	[[no_unique_address]] IG::Data::PixmapReader pixmapReader;
	[[no_unique_address]] IG::Data::PixmapWriter pixmapWriter;
	[[no_unique_address]] IG::VibrationManager vibrationManager_;
	[[no_unique_address]] PerformanceHintManager perfHintManager;
	[[no_unique_address]] PerformanceHintSession perfHintSession;
	BluetoothAdapter *bta{};
	IG_UseMemberIf(MOGA_INPUT, std::unique_ptr<Input::MogaManager>, mogaManagerPtr);
public:
	RecentContent recentContent;
	std::string userScreenshotPath;
protected:
	IG_UseMemberIf(Config::cpuAffinity, CPUMask, cpuAffinityMask){};
	int savedAdvancedFrames{};
	static constexpr int16_t defaultFastModeSpeed{800};
	static constexpr int16_t defaultSlowModeSpeed{50};
	int16_t fastModeSpeed{defaultFastModeSpeed};
	int16_t slowModeSpeed{defaultSlowModeSpeed};
	Byte2Option optionFontSize;
	Byte1Option optionPauseUnfocused;
	Byte1Option optionConfirmOverwriteState;
	Byte1Option optionNotificationIcon;
	Byte1Option optionTitleBar;
	Byte1Option optionSystemActionsIsDefaultMenu;
	Byte1Option optionIdleDisplayPowerSave;
	IG_UseMemberIf(Config::NAVIGATION_BAR, Byte1Option, optionLowProfileOSNav);
	IG_UseMemberIf(Config::NAVIGATION_BAR, Byte1Option, optionHideOSNav);
	IG_UseMemberIf(Config::STATUS_BAR, Byte1Option, optionHideStatusBar);
	Orientations optionEmuOrientation;
	Orientations optionMenuOrientation;
	Byte1Option optionShowBundledGames;
	IG_UseMemberIf(Config::Input::BLUETOOTH, Byte1Option, optionShowBluetoothScan);
	Byte1Option optionImgFilter;
	Byte1Option optionImgEffect;
	Byte1Option optionImageEffectPixelFormat;
	Byte1Option optionOverlayEffect;
	Byte1Option optionOverlayEffectLevel;
	Byte1Option optionFrameInterval;
	Byte1Option optionImageZoom;
	Byte1Option optionViewportZoom;
	Byte1Option optionShowOnSecondScreen;
	Byte1Option optionTextureBufferMode;
	Byte1Option optionVideoImageBuffers;
	Gfx::DrawableConfig windowDrawableConf;
	IG::PixelFormat renderPixelFmt;
	IG::Rotation contentRotation_{IG::Rotation::ANY};
	IG_UseMemberIf(Config::TRANSLUCENT_SYSTEM_UI, bool, layoutBehindSystemUI){};
public:
	bool showHiddenFilesInPicker{};
	IG_UseMemberIf(Config::envIsAndroid, bool, useSustainedPerformanceMode){};
	IG_UseMemberIf(Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, bool, keepBluetoothActive){};
	IG_UseMemberIf(Config::Input::DEVICE_HOTSWAP, bool, notifyOnInputDeviceChange){true};
	IG_UseMemberIf(Config::multipleScreenFrameRates, FrameRate, overrideScreenFrameRate){};
	WindowFrameTimeSource windowFrameTimeSource{WindowFrameTimeSource::AUTO};
	IG_UseMemberIf(Config::cpuAffinity, CPUAffinityMode, cpuAffinityMode){CPUAffinityMode::Auto};
	IG_UseMemberIf(Config::envIsAndroid && Config::DEBUG_BUILD, bool, useNoopThread){};
	IG_UseMemberIf(enableFrameTimeStats, bool, showFrameTimeStats){};
	IG_UseMemberIf(Gfx::supportsPresentModes, Gfx::PresentMode, presentMode){};
	IG_UseMemberIf(Gfx::supportsPresentationTime, bool, usePresentationTime){true};
	bool allowBlankFrameInsertion{};
	bool enableBlankFrameInsertion{};

protected:
	struct ConfigParams
	{
		Gfx::DrawableConfig windowDrawableConf{};
	};

	void onMainWindowCreated(ViewAttachParams, const Input::Event &);
	Gfx::TextureSpan collectTextCloseAsset() const;
	ConfigParams loadConfigFile(IG::ApplicationContext);
	void saveConfigFile(IG::ApplicationContext);
	void saveConfigFile(FileIO &);
	void initOptions(IG::ApplicationContext);
	std::optional<IG::PixelFormat> renderPixelFormatOption() const;
	void applyRenderPixelFormat();
	std::optional<IG::PixelFormat> windowDrawablePixelFormatOption() const;
	std::optional<Gfx::ColorSpace> windowDrawableColorSpaceOption() const;
	FS::PathString sessionConfigPath();
	void loadSystemOptions();
	void saveSystemOptions();
	void saveSystemOptions(FileIO &);
	bool allWindowsAreFocused() const;
	void configureSecondaryScreens();
	void addOnFrameDelayed();
	void addOnFrame();
	void removeOnFrame();
	IG::OnFrameDelegate onFrameDelayed(int8_t delay);
	void addOnFrameDelegate(IG::OnFrameDelegate);
	void onFocusChange(bool in);
	void configureAppForEmulation(bool running);
	int16_t &altSpeedRef(AltSpeedMode mode) { return mode == AltSpeedMode::slow ? slowModeSpeed : fastModeSpeed; }
	const int16_t &altSpeedRef(AltSpeedMode mode) const { return mode == AltSpeedMode::slow ? slowModeSpeed : fastModeSpeed; }
	void reportFrameWorkTime();
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuApp &gApp();
IG::ApplicationContext gAppContext();

}
