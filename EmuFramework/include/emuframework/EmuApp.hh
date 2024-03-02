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
#include <emuframework/EmuInput.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/AutosaveManager.hh>
#include <emuframework/OutputTimingManager.hh>
#include <emuframework/RecentContent.hh>
#include <emuframework/RewindManager.hh>
#include <imagine/input/inputDefs.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/ToastView.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/VibrationManager.hh>
#include <imagine/base/PerformanceHintManager.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/gfx/Renderer.hh>
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
class YesNoAlertView;
class ToastView;
}

namespace EmuEx
{

struct MainWindowData;
class EmuMainMenuView;
class EmuViewController;

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
	rewind,
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

WISE_ENUM_CLASS((PresentationTimeMode, uint8_t),
	off, basic, full
);

WISE_ENUM_CLASS((CPUAffinityMode, uint8_t),
	Auto, Any, Manual
);

struct SystemKeyInputFlags
{
	bool allowTurboModifier{true};
};

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
	void setupStaticBackupMemoryFile(FileIO &, std::string_view ext, size_t staticSize, uint8_t initValue = 0) const;
	void readState(std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	DynArray<uint8_t> saveState();
	bool saveState(CStringView path);
	bool saveStateWithSlot(int slot);
	bool loadState(CStringView path);
	bool loadStateWithSlot(int slot);
	bool shouldOverwriteExistingState() const;
	const auto &contentSearchPath() const { return contentSearchPath_; }
	FS::PathString contentSearchPath(std::string_view name) const;
	void setContentSearchPath(auto &&path) { contentSearchPath_ = IG_forward(path); }
	FS::PathString validSearchPath(const FS::PathString &) const;
	static void updateLegacySavePath(IG::ApplicationContext, CStringView path);
	auto screenshotDirectory() const { return system().userPath(userScreenshotPath); }
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	bool handleKeyInput(KeyInfo, const Input::Event &srcEvent);
	bool handleAppActionKeyInput(InputAction, const Input::Event &srcEvent);
	void handleSystemKeyInput(KeyInfo, Input::Action, uint32_t metaState = 0, SystemKeyInputFlags flags = {});
	void runTurboInputEvents();
	void resetInput();
	void setRunSpeed(double speed);
	void saveSessionOptions();
	void loadSessionOptions();
	bool hasSavedSessionOptions();
	void deleteSessionOptions();
	void syncEmulationThread();
	void startAudio();
	EmuViewController &viewController();
	const EmuViewController &viewController() const;
	IG::ToastView &toastView();
	const Screen &emuScreen() const;
	Window &emuWindow();
	const Window &emuWindow() const;
	FrameTimeConfig configFrameTime();
	void setDisabledInputKeys(std::span<const KeyCode> keys);
	void unsetDisabledInputKeys();
	Gfx::TextureSpan asset(AssetID) const;
	Gfx::TextureSpan asset(AssetDesc) const;
	VController &defaultVController() { return inputManager.vController; }
	static std::unique_ptr<View> makeView(ViewAttachParams, ViewID);
	std::unique_ptr<YesNoAlertView> makeCloseContentView();
	void applyOSNavStyle(IG::ApplicationContext, bool inGame);
	void setCPUNeedsLowLatency(IG::ApplicationContext, bool needed);
	bool advanceFrames(FrameParams, EmuSystemTask *);
	void runFrames(EmuSystemTaskContext, EmuVideo *, EmuAudio *, int frames);
	void skipFrames(EmuSystemTaskContext, int frames, EmuAudio *);
	bool skipForwardFrames(EmuSystemTaskContext, int frames);
	void notifyWindowPresented();
	void renderSystemFramebuffer(EmuVideo &);
	void renderSystemFramebuffer() { renderSystemFramebuffer(video); }
	bool writeScreenshot(IG::PixmapView, CStringView path);
	FS::PathString makeNextScreenshotFilename();
	bool mogaManagerIsActive() const { return bool(mogaManagerPtr); }
	void setMogaManagerActive(bool on, bool notify);
	BluetoothAdapter *bluetoothAdapter();
	void closeBluetoothConnections();
	ViewAttachParams attachParams();
	auto &customKeyConfigList() { return inputManager.customKeyConfigs; };
	auto &savedInputDeviceList() { return inputManager.savedInputDevs; };
	IG::Viewport makeViewport(const Window &win) const;
	void setEmuViewOnExtraWindow(bool on, IG::Screen &);
	void record(FrameTimeStatEvent, SteadyClockTimePoint t = {});
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
	IG::PixelFormat videoEffectPixelFormat() const;
	bool setVideoZoom(uint8_t val);
	bool setViewportZoom(uint8_t val);
	bool supportsShowOnSecondScreen(ApplicationContext ctx) { return ctx.androidSDK() >= 17; }
	void setContentRotation(IG::Rotation);
	void updateVideoContentRotation();
	void updateContentRotation();
	float videoBrightness(ImageChannel) const;
	const Gfx::Vec3 &videoBrightnessAsRGB() const { return videoBrightnessRGB; }
	int videoBrightnessAsInt(ImageChannel ch) const { return videoBrightness(ch) * 100.f; }
	void setVideoBrightness(float brightness, ImageChannel);
	Gfx::PresentMode effectivePresentMode() const
	{
		if(frameTimeSource == FrameTimeSource::Renderer)
			return Gfx::PresentMode::Auto;
		return presentMode;
	};
	FrameTimeSource effectiveFrameTimeSource() const
	{
		return emuWindow().evalFrameTimeSource(frameTimeSource);
	};

	// System Options
	bool setAltSpeed(AltSpeedMode mode, int16_t speed);
	int16_t altSpeed(AltSpeedMode mode) const { return mode == AltSpeedMode::slow ? slowModeSpeed.value() : fastModeSpeed.value(); }
	double altSpeedAsDouble(AltSpeedMode mode) const { return altSpeed(mode) / 100.; }
	void setCPUAffinity(int cpuNumber, bool on);
	bool cpuAffinity(int cpuNumber) const;
	void applyCPUAffinity(bool active);

	// GUI Options
	void setIdleDisplayPowerSave(bool on);
	bool setFontSize(int size); // size in micro-meters
	void applyFontSize(Window &win);
	IG::FontSettings fontSettings(Window &win) const;
	void setShowsTitleBar(bool on);
	void setLowProfileOSNavMode(Tristate mode);
	void setHideOSNavMode(Tristate mode);
	void setHideStatusBarMode(Tristate mode);
	void setEmuOrientation(Orientations);
	void setMenuOrientation(Orientations);
	void setShowsBundledGames(bool);
	auto canShowNotificationIcon(IG::ApplicationContext ctx) { return Config::envIsAndroid && ctx.androidSDK() < 17; }
	void setShowsBluetoothScanItems(bool on);
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
		toastView().post(IG_forward(msg), secs, error);
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
public:
	IG::Audio::Manager audioManager;
	EmuAudio audio;
	EmuVideo video;
	EmuVideoLayer videoLayer;
	AutosaveManager autosaveManager{*this};
	InputManager inputManager;
	OutputTimingManager outputTimingManager;
	RewindManager rewindManager{*this};
	ConditionalMember<enableFrameTimeStats, FrameTimeStats> frameTimeStats;
	[[no_unique_address]] IG::VibrationManager vibrationManager;
protected:
	EmuSystemTask emuSystemTask{*this};
	mutable Gfx::Texture assetBuffImg[wise_enum::size<AssetFileID>];
	int savedAdvancedFrames{};
	Gfx::Vec3 videoBrightnessRGB{1.f, 1.f, 1.f};
	FS::PathString contentSearchPath_;
	[[no_unique_address]] IG::Data::PixmapReader pixmapReader;
	[[no_unique_address]] IG::Data::PixmapWriter pixmapWriter;
	[[no_unique_address]] PerformanceHintManager perfHintManager;
	[[no_unique_address]] PerformanceHintSession perfHintSession;
	BluetoothAdapter *bta{};
	ConditionalMember<MOGA_INPUT, std::unique_ptr<Input::MogaManager>> mogaManagerPtr;
	Gfx::DrawableConfig windowDrawableConf;
	IG::PixelFormat renderPixelFmt;
	ConditionalMember<Config::TRANSLUCENT_SYSTEM_UI, bool> layoutBehindSystemUI{};
	bool enableBlankFrameInsertion{};
public:
	RecentContent recentContent;
	std::string userScreenshotPath;
	ConditionalProperty<Config::cpuAffinity, CPUMask, CFGKEY_CPU_AFFINITY_MASK> cpuAffinityMask;
	Property<int16_t, CFGKEY_FAST_MODE_SPEED,
		PropertyDesc<int16_t>{.defaultValue = 800, .isValid = isValidFastSpeed}> fastModeSpeed;
	Property<int16_t, CFGKEY_SLOW_MODE_SPEED,
		PropertyDesc<int16_t>{.defaultValue = 50, .isValid = isValidSlowSpeed}> slowModeSpeed;
	Property<int16_t, CFGKEY_FONT_Y_SIZE, PropertyDesc<int16_t>{
		.defaultValue = Config::MACHINE_IS_PANDORA ? 6500 : (Config::envIsIOS || Config::envIsAndroid) ? 3000 : 8000,
		.mutableDefault = true,
		.isValid = isValidFontSize}> fontSize;
	Property<int8_t, CFGKEY_FRAME_INTERVAL, PropertyDesc<int8_t>{.defaultValue = 1, .isValid = isValidFrameInterval}> frameInterval;
	ConditionalProperty<Config::envIsAndroid, bool, CFGKEY_NOTIFICATION_ICON,
		PropertyDesc<bool>{.defaultValue = true, .mutableDefault = true}> showsNotificationIcon;
	ConditionalProperty<CAN_HIDE_TITLE_BAR, bool, CFGKEY_TITLE_BAR,
		PropertyDesc<bool>{.defaultValue = true}> showsTitleBar;
	ConditionalProperty<Config::NAVIGATION_BAR, Tristate, CFGKEY_LOW_PROFILE_OS_NAV,
		PropertyDesc<Tristate>{.defaultValue = Tristate::IN_EMU}> lowProfileOSNav;
	ConditionalProperty<Config::NAVIGATION_BAR, Tristate, CFGKEY_HIDE_OS_NAV,
		PropertyDesc<Tristate>{.defaultValue = Tristate::OFF, .mutableDefault = true}> hidesOSNav;
	ConditionalProperty<Config::STATUS_BAR, Tristate, CFGKEY_HIDE_STATUS_BAR,
		PropertyDesc<Tristate>{.defaultValue = Tristate::IN_EMU}> hidesStatusBar;
	Property<Orientations, CFGKEY_GAME_ORIENTATION> emuOrientation;
	Property<Orientations, CFGKEY_MENU_ORIENTATION> menuOrientation;
	Property<bool, CFGKEY_SHOW_BUNDLED_GAMES, PropertyDesc<bool>{.defaultValue = true}> showsBundledGames;
	ConditionalProperty<Config::Input::BLUETOOTH, bool, CFGKEY_SHOW_BLUETOOTH_SCAN,
		PropertyDesc<bool>{.defaultValue = true, .mutableDefault = true}> showsBluetoothScan;
	Property<PixelFormatID, CFGKEY_IMAGE_EFFECT_PIXEL_FORMAT,
		PropertyDesc<PixelFormatID>{.defaultValue = PIXEL_NONE, .isValid = imageEffectPixelFormatIsValid}> imageEffectPixelFormat;
	Property<int8_t, CFGKEY_VIEWPORT_ZOOM, PropertyDesc<int8_t>{.defaultValue = 100, .isValid = optionViewportZoomIsValid}> viewportZoom;
	Property<uint8_t, CFGKEY_IMAGE_ZOOM, PropertyDesc<uint8_t>{.defaultValue = 100, .isValid = optionImageZoomIsValid}> imageZoom;
	ConditionalProperty<Config::BASE_MULTI_WINDOW && Config::BASE_MULTI_SCREEN, bool, CFGKEY_SHOW_ON_2ND_SCREEN> showOnSecondScreen;
	Property<Gfx::TextureBufferMode, CFGKEY_TEXTURE_BUFFER_MODE> textureBufferMode;
	Property<Rotation, CFGKEY_CONTENT_ROTATION,
		PropertyDesc<Rotation>{.defaultValue = Rotation::ANY, .isValid = enumIsValidUpToLast}> contentRotation;
	Property<bool, CFGKEY_IDLE_DISPLAY_POWER_SAVE> idleDisplayPowerSave;
	Property<bool, CFGKEY_SHOW_HIDDEN_FILES> showHiddenFilesInPicker;
	Property<bool, CFGKEY_CONFIRM_OVERWRITE_STATE, PropertyDesc<bool>{.defaultValue = true}> confirmOverwriteState;
	Property<bool, CFGKEY_SYSTEM_ACTIONS_IS_DEFAULT_MENU, PropertyDesc<bool>{.defaultValue = true}> systemActionsIsDefaultMenu;
	ConditionalProperty<Config::windowFocus, bool, CFGKEY_PAUSE_UNFOCUSED, PropertyDesc<bool>{
		.defaultValue = true}> pauseUnfocused;
	ConditionalProperty<Config::envIsAndroid, bool, CFGKEY_SUSTAINED_PERFORMANCE_MODE> useSustainedPerformanceMode;
	ConditionalProperty<Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, bool, CFGKEY_KEEP_BLUETOOTH_ACTIVE> keepBluetoothActive;
	ConditionalProperty<Config::Input::DEVICE_HOTSWAP, bool, CFGKEY_NOTIFY_INPUT_DEVICE_CHANGE,
		PropertyDesc<bool>{.defaultValue = true}> notifyOnInputDeviceChange;
	ConditionalMember<Config::multipleScreenFrameRates, FrameRate> overrideScreenFrameRate{};
	Property<FrameTimeSource, CFGKEY_FRAME_CLOCK, PropertyDesc<FrameTimeSource>{
		.defaultValue = FrameTimeSource::Unset, .isValid = enumIsValidUpToLast}> frameTimeSource;
	ConditionalProperty<Config::cpuAffinity, CPUAffinityMode, CFGKEY_CPU_AFFINITY_MODE,
		PropertyDesc<CPUAffinityMode>{.defaultValue = CPUAffinityMode::Auto, .isValid = enumIsValidUpToLast}> cpuAffinityMode;
	ConditionalMember<Config::envIsAndroid && Config::DEBUG_BUILD, bool> useNoopThread{};
	ConditionalMember<enableFrameTimeStats, bool> showFrameTimeStats{};
	ConditionalProperty<Gfx::supportsPresentModes, Gfx::PresentMode, CFGKEY_RENDERER_PRESENT_MODE,
		PropertyDesc<Gfx::PresentMode>{.defaultValue = Gfx::PresentMode::Auto, .isValid = enumIsValidUpToLast}> presentMode;
	ConditionalMember<Gfx::supportsPresentationTime, PresentationTimeMode> presentationTimeMode{PresentationTimeMode::basic};
	Property<bool, CFGKEY_BLANK_FRAME_INSERTION> allowBlankFrameInsertion;

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
	void reportFrameWorkTime();
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuApp &gApp();
IG::ApplicationContext gAppContext();

}
