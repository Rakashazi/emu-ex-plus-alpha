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
#include <emuframework/TurboInput.hh>
#include <emuframework/Option.hh>
#include <imagine/input/Input.hh>
#include <imagine/input/android/MogaManager.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Timer.hh>
#include <imagine/base/VibrationManager.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/data-type/image/PixmapWriter.hh>
#include <imagine/font/Font.hh>
#include <imagine/util/used.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/enum.hh>
#include <cstring>
#include <optional>
#include <span>

namespace IG
{
class BluetoothAdapter;
class FileIO;
class BasicNavView;
}

namespace EmuEx
{

struct MainWindowData;

struct RecentContentInfo
{
	FS::PathString path{};
	FS::FileString name{};

	constexpr bool operator ==(RecentContentInfo const& rhs) const
	{
		return path == rhs.path;
	}
};

enum class Tristate : uint8_t
{
	OFF, IN_EMU, ON
};

WISE_ENUM_CLASS((AssetID, size_t),
	ARROW,
	CLOSE,
	ACCEPT,
	GAME_ICON,
	MENU,
	FAST_FORWARD,
	GAMEPAD_OVERLAY,
	KEYBOARD_OVERLAY);

class EmuApp : public IG::Application
{
public:
	using OnMainMenuOptionChanged = DelegateFunc<void()>;
	using CreateSystemCompleteDelegate = DelegateFunc<void (const Input::Event &)>;
	using NavView = BasicNavView;
	static constexpr int MAX_RECENT = 10;
	using RecentContentList = StaticArrayList<RecentContentInfo, MAX_RECENT>;

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
	static bool autoSaveStateDefault;
	static bool hasIcon;
	static bool needsGlobalInstance;

	EmuApp(IG::ApplicationInitParams, IG::ApplicationContext &);
	void mainInitCommon(IG::ApplicationInitParams, IG::ApplicationContext);
	static void onCustomizeNavView(NavView &v);
	void createSystemWithMedia(IG::IO, IG::CStringView path, std::string_view displayName,
		const Input::Event &, EmuSystemCreateParams, ViewAttachParams, CreateSystemCompleteDelegate);
	void closeSystem(bool allowAutosaveState = true);
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
	void launchSystemWithResumePrompt(const Input::Event &);
	void launchSystem(const Input::Event &, bool tryAutoState);
	void launchSystem(bool tryAutoState);
	static bool hasArchiveExtension(std::string_view name);
	void setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func);
	void dispatchOnMainMenuItemOptionChanged();
	void unpostMessage();
	void printScreenshotResult(int num, bool success);
	void saveAutoState();
	bool loadAutoState();
	bool saveState(IG::CStringView path);
	bool saveStateWithSlot(int slot);
	bool loadState(IG::CStringView path);
	bool loadStateWithSlot(int slot);
	bool shouldOverwriteExistingState() const;
	void setDefaultVControlsButtonSpacing(int spacing);
	void setDefaultVControlsButtonStagger(int stagger);
	FS::PathString contentSearchPath() const;
	FS::PathString contentSearchPath(std::string_view name) const;
	void setContentSearchPath(std::string_view path);
	FS::PathString firmwareSearchPath() const;
	void setFirmwareSearchPath(std::string_view path);
	static void updateLegacySavePath(IG::ApplicationContext, IG::CStringView path);
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	void addTurboInputEvent(unsigned action);
	void removeTurboInputEvent(unsigned action);
	void runTurboInputEvents();
	void resetInput();
	void setFastForwardSpeed(float speed);
	void saveSessionOptions();
	void loadSessionOptions();
	bool hasSavedSessionOptions();
	void deleteSessionOptions();
	void syncEmulationThread();
	void prepareAudio();
	void startAudio();
	EmuAudio &audio();
	EmuVideo &video();
	EmuViewController &viewController();
	void cancelAutoSaveStateTimer();
	void startAutoSaveStateTimer();
	void configFrameTime();
	void applyEnabledFaceButtons(std::span<const std::pair<int, bool>> applyEnableMap);
	void applyEnabledCenterButtons(std::span<const std::pair<int, bool>> applyEnableMap);
	void updateKeyboardMapping();
	void toggleKeyboard();
	void updateVControllerMapping();
	Gfx::PixmapTexture &asset(AssetID) const;
	void updateInputDevices(IG::ApplicationContext);
	void setOnUpdateInputDevices(DelegateFunc<void ()>);
	VController &defaultVController();
	static std::unique_ptr<View> makeView(ViewAttachParams, ViewID);
	void applyOSNavStyle(IG::ApplicationContext, bool inGame);
	void setCPUNeedsLowLatency(IG::ApplicationContext, bool needed);
	void runFrames(EmuSystemTaskContext, EmuVideo *, EmuAudio *, int frames, bool skipForward);
	void skipFrames(EmuSystemTaskContext, int frames, EmuAudio *);
	bool skipForwardFrames(EmuSystemTaskContext, int frames);
	FloatSeconds bestFrameTimeForScreen(VideoSystem system) const;
	void applyFrameRates(bool updateFrameTime = true);
	IG::Audio::Manager &audioManager();
	void renderSystemFramebuffer(EmuVideo &);
	bool writeScreenshot(IG::PixmapView, IG::CStringView path);
	std::pair<int, FS::PathString> makeNextScreenshotFilename();
	bool mogaManagerIsActive() const;
	void setMogaManagerActive(bool on, bool notify);
	constexpr IG::VibrationManager &vibrationManager() { return vibrationManager_; }
	std::span<const KeyCategory> inputControlCategories() const;
	BluetoothAdapter *bluetoothAdapter();
	void closeBluetoothConnections();
	ViewAttachParams attachParams();
	void addRecentContent(std::string_view path, std::string_view name);
	void addCurrentContentToRecent();
	RecentContentList &recentContent() { return recentContentList; };
	void writeRecentContent(FileIO &);
	void readRecentContent(IG::ApplicationContext, MapIO &, size_t readSize_);
	bool showHiddenFilesInPicker(){ return showHiddenFilesInPicker_; };
	void setShowHiddenFilesInPicker(bool on){ showHiddenFilesInPicker_ = on; };
	auto &customKeyConfigList() { return customKeyConfigs; };
	auto &savedInputDeviceList() { return savedInputDevs; };
	IG::Viewport makeViewport(const Window &win) const;
	void setEmuViewOnExtraWindow(bool on, IG::Screen &);
	void setWindowFrameClockSource(IG::Window::FrameTimeSource src) { winFrameTimeSrc = src; }
	IG::Window::FrameTimeSource windowFrameClockSource() const { return winFrameTimeSrc; }
	static std::u16string_view mainViewName();
	void runBenchmarkOneShot(EmuVideo &);
	void onSelectFileFromPicker(IG::IO, IG::CStringView path, std::string_view displayName,
		const Input::Event &, EmuSystemCreateParams, ViewAttachParams);
	void handleOpenFileCommand(IG::CStringView path);
	static bool hasGooglePlayStoreFeatures();
	EmuSystem &system();
	const EmuSystem &system() const;
	ApplicationContext appContext() const;
	static EmuApp &get(ApplicationContext);
	MainWindowData &mainWindowData() const;

	// Audio Options
	void setAudioOutputAPI(IG::Audio::Api);
	IG::Audio::Api audioOutputAPI() const;
	void setSoundRate(int rate);
	int soundRate() const { return optionSoundRate; }
	int soundRateMax() const { return optionSoundRate.defaultVal; }
	bool canChangeSoundRate() const { return !optionSoundRate.isConst; }
	bool setSoundVolume(int vol);
	int soundVolume() const { return optionSoundVolume; }
	void setSoundBuffers(int buffers);
	int soundBuffers() const { return optionSoundBuffers; }
	void setSoundEnabled(bool on);
	bool soundIsEnabled() const;
	void setAddSoundBuffersOnUnderrun(bool on);
	bool addSoundBuffersOnUnderrun() const { return optionAddSoundBuffersOnUnderrun; }
	void setSoundDuringFastForwardEnabled(bool on);
	bool soundDuringFastForwardIsEnabled() const;

	// Video Options
	bool setWindowDrawableConfig(Gfx::DrawableConfig);
	Gfx::DrawableConfig windowDrawableConfig() const;
	IG::PixelFormat windowPixelFormat() const;
	void setRenderPixelFormat(std::optional<IG::PixelFormat>);
	IG::PixelFormat renderPixelFormat() const;
	bool setVideoAspectRatio(double val);
	double videoAspectRatio() const;
	auto &videoFilterOption() { return optionImgFilter; }
	auto &videoEffectOption() { return optionImgEffect; }
	IG::PixelFormat videoEffectPixelFormat() const;
	auto &videoEffectPixelFormatOption() { return optionImageEffectPixelFormat; }
	auto &overlayEffectOption() { return optionOverlayEffect; }
	bool setOverlayEffectLevel(EmuVideoLayer &, uint8_t val);
	uint8_t overlayEffectLevel() { return optionOverlayEffectLevel; }
	std::pair<IG::FloatSeconds, bool> setFrameTime(VideoSystem system, IG::FloatSeconds time);
	FloatSeconds frameTime(VideoSystem) const;
	bool frameTimeIsConst(VideoSystem) const;
	void setFrameInterval(int);
	int frameInterval() const;
	void setShouldSkipLateFrames(bool on) { optionSkipLateFrames = on; }
	bool shouldSkipLateFrames() const { return optionSkipLateFrames; }
	bool setVideoZoom(uint8_t val);
	uint8_t videoZoom() const { return optionImageZoom; }
	bool setViewportZoom(uint8_t val);
	uint8_t viewportZoom() { return optionViewportZoom; }
	auto &showOnSecondScreenOption() { return optionShowOnSecondScreen; }
	auto &textureBufferModeOption() { return optionTextureBufferMode; }
	auto &videoImageBuffersOption() { return optionVideoImageBuffers; }
	void setUsePresentationTime(bool on) { usePresentationTime_ = on; }
	bool usePresentationTime() const { return usePresentationTime_; }

	// System Options
	auto &autoSaveStateOption() { return optionAutoSaveState; }
	auto &confirmAutoLoadStateOption() { return optionConfirmAutoLoadState; }
	auto &confirmOverwriteStateOption() { return optionConfirmOverwriteState; }
	auto &fastForwardSpeedOption() { return optionFastForwardSpeed; }
	auto &sustainedPerformanceModeOption() { return optionSustainedPerformanceMode; }

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
	void setEmuOrientation(Orientation);
	void setMenuOrientation(Orientation);
	Orientation emuOrientation() const { return optionEmuOrientation; }
	Orientation menuOrientation() const { return optionMenuOrientation; }
	void setShowsBundledGames(bool);
	bool showsBundledGames() const { return optionShowBundledGames; }
	auto &notificationIconOption() { return optionNotificationIcon; }
	void setShowsBluetoothScanItems(bool on);
	bool showsBluetoothScanItems() const { return optionShowBluetoothScan; }
	void setLayoutBehindSystemUI(bool);
	bool doesLayoutBehindSystemUI() const { return layoutBehindSystemUI; };

	// Input Options
	auto &notifyInputDeviceChangeOption() { return optionNotifyInputDeviceChange; }
	auto &keepBluetoothActiveOption() { return optionKeepBluetoothActive; }

	void postMessage(auto msg)
	{
		postMessage(false, std::move(msg));
	}

	void postMessage(bool error, auto msg)
	{
		postMessage(3, error, std::move(msg));
	}

	void postMessage(int secs, bool error, auto msg)
	{
		viewController().popupMessageView().post(std::move(msg), secs, error);
	}

	void postErrorMessage(auto msg)
	{
		postMessage(true, std::move(msg));
	}

	void postErrorMessage(int secs, auto msg)
	{
		postMessage(secs, true, std::move(msg));
	}

	template <std::same_as<const char*> T>
	static std::pair<T, int> scanValue(const char *str)
	{
		return {str, strlen(str) ? 1 : 0};
	}

	template <std::integral T>
	static std::pair<T, int> scanValue(const char *str)
	{
		int val;
		int items = sscanf(str, "%d", &val);
		return {val, items};
	}

	template <std::floating_point T>
	static std::pair<T, int> scanValue(const char *str)
	{
		double val;
		double denom;
		int items = sscanf(str, "%lf /%lf", &val, &denom);
		if(items > 1 && denom > 0)
		{
			val /= denom;
		}
		return {val, items};
	}

	template <std::same_as<std::pair<double, double>> T>
	static std::pair<T, int> scanValue(const char *str)
	{
		// special case for getting a fraction
		T val{};
		int items = sscanf(str, "%lf /%lf", &val.first, &val.second);
		if(!val.second)
		{
			val.second = 1.;
		}
		return {val, items};
	}

	template<class T>
	void pushAndShowNewCollectValueInputView(ViewAttachParams attach, const Input::Event &e,
		IG::CStringView msgText, IG::CStringView initialContent, IG::Callable<bool, EmuApp&, T> auto &&collectedValueFunc)
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
				auto [val, items] = scanValue<T>(str);
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
			IG::CStringView msgText, IG::CStringView initialContent, IG::Callable<bool, EmuApp&, T> auto &&collectedValueFunc)
	{
		pushAndShowNewCollectValueInputView<int>(attach, e, msgText, initialContent,
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

protected:
	IG::FontManager fontManager;
	mutable Gfx::Renderer renderer;
	ViewManager viewManager{};
	IG::Audio::Manager audioManager_;
	EmuAudio emuAudio;
	EmuVideo emuVideo{};
	EmuVideoLayer emuVideoLayer;
	EmuSystemTask emuSystemTask;
	mutable Gfx::PixmapTexture assetBuffImg[wise_enum::size<AssetID>]{};
	IG_UseMemberIf(VCONTROLS, VController, vController);
	IG::Timer autoSaveStateTimer;
	DelegateFunc<void ()> onUpdateInputDevices_{};
	OnMainMenuOptionChanged onMainMenuOptionChanged_{};
	KeyConfigContainer customKeyConfigs{};
	InputDeviceSavedConfigContainer savedInputDevs{};
	TurboInput turboActions{};
	FS::PathString contentSearchPath_{};
	[[no_unique_address]] IG::Data::PixmapReader pixmapReader;
	[[no_unique_address]] IG::Data::PixmapWriter pixmapWriter;
	[[no_unique_address]] IG::VibrationManager vibrationManager_;
	#ifdef CONFIG_BLUETOOTH
	BluetoothAdapter *bta{};
	#endif
	IG_UseMemberIf(MOGA_INPUT, std::unique_ptr<Input::MogaManager>, mogaManagerPtr){};
	RecentContentList recentContentList{};
	DoubleOption optionAspectRatio;
	DoubleOption optionFrameRate;
	DoubleOption optionFrameRatePAL;
	Byte4Option optionSoundRate;
	Byte2Option optionFontSize;
	Byte1Option optionPauseUnfocused;
	Byte1Option optionAutoSaveState;
	Byte1Option optionConfirmAutoLoadState;
	Byte1Option optionConfirmOverwriteState;
	Byte1Option optionFastForwardSpeed;
	Byte1Option optionSound;
	Byte1Option optionSoundVolume;
	Byte1Option optionSoundBuffers;
	Byte1Option optionAddSoundBuffersOnUnderrun;
	IG_UseMemberIf(IG::Audio::Config::MULTIPLE_SYSTEM_APIS, Byte1Option, optionAudioAPI);
	Byte1Option optionNotificationIcon;
	Byte1Option optionTitleBar;
	Byte1Option optionSystemActionsIsDefaultMenu;
	Byte1Option optionIdleDisplayPowerSave;
	IG_UseMemberIf(Config::NAVIGATION_BAR, Byte1Option, optionLowProfileOSNav);
	IG_UseMemberIf(Config::NAVIGATION_BAR, Byte1Option, optionHideOSNav);
	IG_UseMemberIf(Config::STATUS_BAR, Byte1Option, optionHideStatusBar);
	IG_UseMemberIf(Config::Input::DEVICE_HOTSWAP, Byte1Option, optionNotifyInputDeviceChange);
	Byte1Option optionEmuOrientation;
	Byte1Option optionMenuOrientation;
	Byte1Option optionShowBundledGames;
	IG_UseMemberIf(Config::Input::BLUETOOTH && Config::BASE_CAN_BACKGROUND_APP, Byte1Option, optionKeepBluetoothActive);
	IG_UseMemberIf(Config::Input::BLUETOOTH, Byte1Option, optionShowBluetoothScan);
	IG_UseMemberIf(Config::envIsAndroid, Byte1Option, optionSustainedPerformanceMode);
	Byte1Option optionImgFilter;
	Byte1Option optionImgEffect;
	Byte1Option optionImageEffectPixelFormat;
	Byte1Option optionOverlayEffect;
	Byte1Option optionOverlayEffectLevel;
	IG_UseMemberIf(Config::SCREEN_FRAME_INTERVAL, Byte1Option, optionFrameInterval);
	Byte1Option optionSkipLateFrames;
	Byte1Option optionImageZoom;
	Byte1Option optionViewportZoom;
	Byte1Option optionShowOnSecondScreen;
	Byte1Option optionTextureBufferMode;
	Byte1Option optionVideoImageBuffers;
	Gfx::DrawableConfig windowDrawableConf{};
	IG::PixelFormat renderPixelFmt{};
	bool showHiddenFilesInPicker_{};
	IG_UseMemberIf(Config::TRANSLUCENT_SYSTEM_UI, bool, layoutBehindSystemUI){};
	IG::WindowFrameTimeSource winFrameTimeSrc{};
	IG_UseMemberIf(Config::envIsAndroid, bool, usePresentationTime_){true};

	class ConfigParams
	{
	public:
		static constexpr uint8_t BACK_NAVIGATION_IS_SET_BIT = IG::bit(0);
		static constexpr uint8_t BACK_NAVIGATION_BIT = IG::bit(1);

		constexpr std::optional<bool> backNavigation() const
		{
			if(flags & BACK_NAVIGATION_IS_SET_BIT)
				return flags & BACK_NAVIGATION_BIT;
			return {};
		}

		constexpr void setBackNavigation(std::optional<bool> opt)
		{
			if(!opt)
				return;
			flags |= BACK_NAVIGATION_IS_SET_BIT;
			flags = IG::setOrClearBits(flags, BACK_NAVIGATION_BIT, *opt);
		}

	protected:
		uint8_t flags{};
		Gfx::DrawableConfig windowDrawableConf{};
	};

	bool willCreateSystem(ViewAttachParams, const Input::Event &);
	void onMainWindowCreated(ViewAttachParams, const Input::Event &);
	Gfx::PixmapTexture *collectTextCloseAsset() const;
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

	const DoubleOption &frameTimeOption(VideoSystem system) const
	{
		switch(system)
		{
			default:
			case VideoSystem::NATIVE_NTSC: return optionFrameRate;
			case VideoSystem::PAL: return optionFrameRatePAL;
		}
	}

	DoubleOption &frameTimeOption(VideoSystem system)
	{
		return const_cast<DoubleOption&>(std::as_const(*this).frameTimeOption(system));
	}
};

// Global instance access if required by the emulated system, valid if EmuApp::needsGlobalInstance initialized to true
EmuApp &gApp();
IG::ApplicationContext gAppContext();

}
