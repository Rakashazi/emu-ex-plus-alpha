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
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Timer.hh>
#include <imagine/audio/Manager.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/data-type/image/PixmapWriter.hh>
#include <imagine/util/typeTraits.hh>
#include <cstring>
#include <optional>

struct InputDeviceConfig;

class EmuApp : public Base::Application
{
public:
	using OnMainMenuOptionChanged = DelegateFunc<void()>;
	using CreateSystemCompleteDelegate = DelegateFunc<void (Input::Event e)>;
	using NavView = BasicNavView;

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

	EmuApp(Base::ApplicationInitParams, Base::ApplicationContext &, Gfx::Error &);

	bool willCreateSystem(ViewAttachParams attach, Input::Event e);
	void createSystemWithMedia(GenericIO io, const char *path, const char *name,
		Input::Event e, EmuSystemCreateParams, ViewAttachParams,
		CreateSystemCompleteDelegate onComplete);
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
	void launchSystemWithResumePrompt(Input::Event e, bool addToRecent);
	void launchSystem(Input::Event e, bool tryAutoState, bool addToRecent);
	static bool hasArchiveExtension(const char *name);
	void setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func);
	void dispatchOnMainMenuItemOptionChanged();
	[[gnu::format(printf, 4, 5)]]
	void printfMessage(unsigned secs, bool error, const char *format, ...);
	void postMessage(const char *msg);
	void postMessage(bool error, const char *msg);
	void postMessage(unsigned secs, bool error, const char *msg);
	void postErrorMessage(const char *msg);
	void postErrorMessage(unsigned secs, const char *msg);
	void unpostMessage();
	void printScreenshotResult(int num, bool success);
	void saveAutoState();
	bool loadAutoState();
	EmuSystem::Error saveState(const char *path);
	EmuSystem::Error saveStateWithSlot(int slot);
	EmuSystem::Error loadState(const char *path);
	EmuSystem::Error loadStateWithSlot(int slot);
	static void setDefaultVControlsButtonSpacing(int spacing);
	static void setDefaultVControlsButtonStagger(int stagger);
	FS::PathString mediaSearchPath();
	void setMediaSearchPath(std::optional<FS::PathString>);
	FS::PathString firmwareSearchPath();
	void setFirmwareSearchPath(const char *path);
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	void addTurboInputEvent(unsigned action);
	void removeTurboInputEvent(unsigned action);
	void runTurboInputEvents();
	void resetInput();
	static FS::PathString assetPath(Base::ApplicationContext);
	static FS::PathString libPath(Base::ApplicationContext);
	static FS::PathString supportPath(Base::ApplicationContext);
	static AssetIO openAppAssetIO(Base::ApplicationContext, const char *name, IO::AccessHint access);
	static void saveSessionOptions();
	void loadSessionOptions();
	static bool hasSavedSessionOptions();
	void deleteSessionOptions();
	void syncEmulationThread();
	EmuAudio &audio();
	EmuVideo &video();
	EmuViewController &viewController();
	void cancelAutoSaveStateTimer();
	void startAutoSaveStateTimer();
	void configFrameTime();
	void setActiveFaceButtons(unsigned btns);
	void updateKeyboardMapping();
	void toggleKeyboard();
	void updateVControllerMapping();
	Gfx::PixmapTexture &asset(AssetID) const;
	void updateVControlImg(VController &);
	void updateInputDevices(Base::ApplicationContext);
	void setOnUpdateInputDevices(DelegateFunc<void ()>);
	SysVController &defaultVController();
	static std::unique_ptr<View> makeView(ViewAttachParams, ViewID);
	void applyOSNavStyle(Base::ApplicationContext, bool inGame);
	void setCPUNeedsLowLatency(Base::ApplicationContext, bool needed);
	void runFrames(EmuSystemTask *, EmuVideo *, EmuAudio *, int frames, bool skipForward);
	void skipFrames(EmuSystemTask *, uint32_t frames, EmuAudio *);
	bool skipForwardFrames(EmuSystemTask *task, uint32_t frames);
	void buildKeyInputMapping();
	const KeyMapping &keyInputMapping();
	std::vector<InputDeviceConfig> &inputDeviceConfigs();
	IG::Audio::Manager &audioManager();
	bool setWindowDrawableConfig(Gfx::DrawableConfig);
	Gfx::DrawableConfig windowDrawableConfig() const;
	void setRenderPixelFormat(std::optional<IG::PixelFormat>);
	IG::PixelFormat renderPixelFormat() const;
	void renderSystemFramebuffer(EmuVideo &);
	bool writeScreenshot(IG::Pixmap, const char *path);
	std::pair<int, FS::PathString> makeNextScreenshotFilename();
	Base::ApplicationContext appContext() const;
	static EmuApp &get(Base::ApplicationContext);

	template<class T, class Func>
	void pushAndShowNewCollectValueInputView(ViewAttachParams attach, Input::Event e,
	const char *msgText, const char *initialContent, Func &&collectedValueFunc)
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

	template<class Func1, class Func2>
	void pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e,
		const char *label, const char *yesStr, const char *noStr,
		Func1 &&onYes, Func2 &&onNo)
	{
		pushAndShowNewYesNoAlertView(attach, e, label, yesStr, noStr,
			TextMenuItem::makeSelectDelegate(std::forward<Func1>(onYes)),
			TextMenuItem::makeSelectDelegate(std::forward<Func2>(onNo)));
	}

	template <size_t S>
	static AssetIO openAppAssetIO(Base::ApplicationContext app, std::array<char, S> name, IO::AccessHint access)
	{
		return openAppAssetIO(app, name.data(), access);
	}

protected:
	mutable Gfx::Renderer renderer;
	ViewManager viewManager{};
	IG::Audio::Manager audioManager_;
	EmuAudio emuAudio;
	EmuVideo emuVideo{};
	EmuVideoLayer emuVideoLayer;
	EmuSystemTask emuSystemTask;
	mutable Gfx::PixmapTexture assetBuffImg[(unsigned)AssetID::END]{};
	#ifdef CONFIG_EMUFRAMEWORK_VCONTROLS
	SysVController vController;
	#endif
	std::optional<EmuViewController> emuViewController{};
	Base::Timer autoSaveStateTimer;
	DelegateFunc<void ()> onUpdateInputDevices_{};
	OnMainMenuOptionChanged onMainMenuOptionChanged_{};
	std::vector<InputDeviceConfig> inputDevConf;
	KeyMapping keyMapping{};
	TurboInput turboActions{};
	FS::PathString lastLoadPath{};
	[[no_unique_address]] IG::Data::PixmapWriter pixmapWriter;
	Gfx::DrawableConfig windowDrawableConf{};
	IG::PixelFormat renderPixelFmt{};

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
	void initVControls();
	Gfx::PixmapTexture *collectTextCloseAsset() const;
	ConfigParams loadConfigFile(Base::ApplicationContext);
	void saveConfigFile(Base::ApplicationContext);
	void saveConfigFile(IO &);
	void initOptions(Base::ApplicationContext);
	std::optional<IG::PixelFormat> renderPixelFormatOption() const;
	void applyRenderPixelFormat();
	std::optional<IG::PixelFormat> windowDrawablePixelFormatOption() const;
	std::optional<Gfx::ColorSpace> windowDrawableColorSpaceOption() const;
};
