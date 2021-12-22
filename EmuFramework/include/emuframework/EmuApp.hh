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
#include <imagine/util/typeTraits.hh>
#include <imagine/util/container/ArrayList.hh>
#include <cstring>
#include <optional>
#include <span>

class BluetoothAdapter;
class IO;
class GenericIO;
class BasicNavView;

struct RecentContentInfo
{
	FS::PathString path{};
	FS::FileString name{};

	constexpr bool operator ==(RecentContentInfo const& rhs) const
	{
		return path == rhs.path;
	}
};

class EmuApp : public Base::Application
{
public:
	using OnMainMenuOptionChanged = DelegateFunc<void()>;
	using CreateSystemCompleteDelegate = DelegateFunc<void (Input::Event e)>;
	using NavView = BasicNavView;
	static constexpr unsigned MAX_RECENT = 10;
	using RecentContentList = StaticArrayList<RecentContentInfo, MAX_RECENT>;

	enum class ViewID
	{
		MAIN_MENU,
		SYSTEM_ACTIONS,
		VIDEO_OPTIONS,
		AUDIO_OPTIONS,
		SYSTEM_OPTIONS,
		GUI_OPTIONS,
		EDIT_CHEATS,
		LIST_CHEATS,
	};

	enum class AssetID
	{
		ARROW,
		CLOSE,
		ACCEPT,
		GAME_ICON,
		MENU,
		FAST_FORWARD,
		GAMEPAD_OVERLAY,
		KEYBOARD_OVERLAY,
		END
	};

	static bool autoSaveStateDefault;
	static bool hasIcon;

	EmuApp(Base::ApplicationInitParams, Base::ApplicationContext &);

	bool willCreateSystem(ViewAttachParams attach, Input::Event e);
	void createSystemWithMedia(GenericIO, IG::CStringView path, std::string_view displayName,
		Input::Event, EmuSystemCreateParams, ViewAttachParams, CreateSystemCompleteDelegate);
	void exitGame(bool allowAutosaveState = true);
	void reloadGame(EmuSystemCreateParams params = {});
	void promptSystemReloadDueToSetOption(ViewAttachParams attach, Input::Event e, EmuSystemCreateParams params = {});
	void onMainWindowCreated(ViewAttachParams attach, Input::Event e);
	static void onCustomizeNavView(NavView &v);
	void pushAndShowNewCollectTextInputView(ViewAttachParams attach, Input::Event e,
		const char *msgText, const char *initialContent, CollectTextInputView::OnTextDelegate onText);
	void pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e,
		const char *label, const char *choice1, const char *choice2,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	void pushAndShowModalView(std::unique_ptr<View> v, Input::Event e);
	void pushAndShowModalView(std::unique_ptr<View> v);
	void popModalViews();
	void popMenuToRoot();
	void showSystemActionsViewFromSystem(ViewAttachParams attach, Input::Event e);
	void showLastViewFromSystem(ViewAttachParams attach, Input::Event e);
	void showExitAlert(ViewAttachParams attach, Input::Event e);
	void showEmuation();
	void launchSystemWithResumePrompt(Input::Event e);
	void launchSystem(Input::Event e, bool tryAutoState);
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
	void setDefaultVControlsButtonSpacing(int spacing);
	void setDefaultVControlsButtonStagger(int stagger);
	FS::PathString contentSearchPath() const;
	FS::PathString contentSearchPath(std::string_view name) const;
	void setContentSearchPath(std::string_view path);
	FS::PathString firmwareSearchPath() const;
	void setFirmwareSearchPath(std::string_view path);
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	void addTurboInputEvent(unsigned action);
	void removeTurboInputEvent(unsigned action);
	void runTurboInputEvents();
	void resetInput();
	void saveSessionOptions();
	void loadSessionOptions();
	bool hasSavedSessionOptions();
	void deleteSessionOptions();
	void syncEmulationThread();
	EmuAudio &audio();
	EmuVideo &video();
	EmuViewController &viewController();
	void cancelAutoSaveStateTimer();
	void startAutoSaveStateTimer();
	void configFrameTime();
	void applyEnabledFaceButtons(std::span<const std::pair<int, bool>> applyEnableMap);
	void updateKeyboardMapping();
	void toggleKeyboard();
	void updateVControllerMapping();
	Gfx::PixmapTexture &asset(AssetID) const;
	void updateInputDevices(Base::ApplicationContext);
	void setOnUpdateInputDevices(DelegateFunc<void ()>);
	VController &defaultVController();
	static std::unique_ptr<View> makeView(ViewAttachParams, ViewID);
	void applyOSNavStyle(Base::ApplicationContext, bool inGame);
	void setCPUNeedsLowLatency(Base::ApplicationContext, bool needed);
	void runFrames(EmuSystemTaskContext, EmuVideo *, EmuAudio *, int frames, bool skipForward);
	void skipFrames(EmuSystemTaskContext, uint32_t frames, EmuAudio *);
	bool skipForwardFrames(EmuSystemTaskContext, uint32_t frames);
	IG::Audio::Manager &audioManager();
	bool setWindowDrawableConfig(Gfx::DrawableConfig);
	Gfx::DrawableConfig windowDrawableConfig() const;
	void setRenderPixelFormat(std::optional<IG::PixelFormat>);
	IG::PixelFormat renderPixelFormat() const;
	void renderSystemFramebuffer(EmuVideo &);
	bool writeScreenshot(IG::Pixmap, IG::CStringView path);
	std::pair<int, FS::PathString> makeNextScreenshotFilename();
	bool mogaManagerIsActive() const;
	void setMogaManagerActive(bool on, bool notify);
	constexpr Base::VibrationManager &vibrationManager() { return vibrationManager_; }
	std::span<const KeyCategory> inputControlCategories() const;
	BluetoothAdapter *bluetoothAdapter();
	void closeBluetoothConnections();
	ViewAttachParams attachParams();
	void addRecentContent(std::string_view path, std::string_view name);
	void addCurrentContentToRecent();
	RecentContentList &recentContent() { return recentContentList; };
	void writeRecentContent(IO &);
	void readRecentContent(Base::ApplicationContext, IO &, unsigned readSize_);
	bool showHiddenFilesInPicker(){ return showHiddenFilesInPicker_; };
	void setShowHiddenFilesInPicker(bool on){ showHiddenFilesInPicker_ = on; };
	Base::ApplicationContext appContext() const;
	static EmuApp &get(Base::ApplicationContext);

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

	template<class T>
	void pushAndShowNewCollectValueInputView(ViewAttachParams attach, Input::Event e,
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
				T val;
				int items = 0;
				if constexpr(std::is_same_v<T, const char*>)
				{
					val = str;
					if(strlen(str))
						items = 1;
				}
				else if constexpr(std::is_integral_v<T>)
				{
					items = sscanf(str, "%d", &val);
				}
				else if constexpr(std::is_floating_point_v<T>)
				{
					double denom;
					items = sscanf(str, "%lf /%lf", &val, &denom);
					if(items > 1 && denom > 0)
					{
						val /= denom;
					}
				}
				else if constexpr(std::is_same_v<T, std::pair<double, double>>)
				{
					// special case for getting a fraction
					val = {};
					items = sscanf(str, "%lf /%lf", &val.first, &val.second);
					if(!val.second)
					{
						val.second = 1.;
					}
				}
				else
				{
					static_assert(IG::dependentFalseValue<T>, "can't collect value of this type");
				}
				auto &app = get(view.appContext());
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

protected:
	IG::FontManager fontManager;
	mutable Gfx::Renderer renderer;
	ViewManager viewManager{};
	IG::Audio::Manager audioManager_;
	EmuAudio emuAudio;
	EmuVideo emuVideo{};
	EmuVideoLayer emuVideoLayer;
	EmuSystemTask emuSystemTask;
	mutable Gfx::PixmapTexture assetBuffImg[(unsigned)AssetID::END]{};
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	VController vController;
	#endif
	std::optional<EmuViewController> emuViewController{};
	Base::Timer autoSaveStateTimer;
	DelegateFunc<void ()> onUpdateInputDevices_{};
	OnMainMenuOptionChanged onMainMenuOptionChanged_{};
	TurboInput turboActions{};
	FS::PathString contentSearchPath_{};
	[[no_unique_address]] IG::Data::PixmapReader pixmapReader;
	[[no_unique_address]] IG::Data::PixmapWriter pixmapWriter;
	[[no_unique_address]] Base::VibrationManager vibrationManager_;
	#ifdef CONFIG_BLUETOOTH
	BluetoothAdapter *bta{};
	#endif
	IG_UseMemberIf(Config::EmuFramework::MOGA_INPUT, std::unique_ptr<Input::MogaManager>, mogaManagerPtr){};
	RecentContentList recentContentList{};
	Gfx::DrawableConfig windowDrawableConf{};
	IG::PixelFormat renderPixelFmt{};
	bool showHiddenFilesInPicker_{};

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

	void mainInitCommon(Base::ApplicationInitParams, Base::ApplicationContext);
	Gfx::PixmapTexture *collectTextCloseAsset() const;
	ConfigParams loadConfigFile(Base::ApplicationContext);
	void saveConfigFile(Base::ApplicationContext);
	void saveConfigFile(IO &);
	void initOptions(Base::ApplicationContext);
	std::optional<IG::PixelFormat> renderPixelFormatOption() const;
	void applyRenderPixelFormat();
	std::optional<IG::PixelFormat> windowDrawablePixelFormatOption() const;
	std::optional<Gfx::ColorSpace> windowDrawableColorSpaceOption() const;
	FS::PathString sessionConfigPath();
};
