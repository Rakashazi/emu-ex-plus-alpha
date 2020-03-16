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

#include <imagine/input/Input.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/io/IO.hh>
#include <imagine/fs/FSDefs.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/util/typeTraits.hh>

class EmuApp
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

	static bool autoSaveStateDefault;
	static bool hasIcon;

	static bool willCreateSystem(ViewAttachParams attach, Input::Event e);
	static void createSystemWithMedia(GenericIO io, const char *path, const char *name,
		Input::Event e, CreateSystemCompleteDelegate onComplete);
	static void exitGame(bool allowAutosaveState = true);
	static void reloadGame();
	static void promptSystemReloadDueToSetOption(ViewAttachParams attach, Input::Event e);
	static void onMainWindowCreated(ViewAttachParams attach, Input::Event e);
	static void onCustomizeNavView(NavView &v);
	static void pushAndShowNewCollectTextInputView(ViewAttachParams attach, Input::Event e,
		const char *msgText, const char *initialContent, CollectTextInputView::OnTextDelegate onText);
	static void pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e,
		const char *label, const char *choice1, const char *choice2,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	static void pushAndShowModalView(std::unique_ptr<View> v, Input::Event e);
	static void popModalViews();
	static void popMenuToRoot();
	static void showSystemActionsViewFromSystem(ViewAttachParams attach, Input::Event e);
	static void showLastViewFromSystem(ViewAttachParams attach, Input::Event e);
	static void showExitAlert(ViewAttachParams attach, Input::Event e);
	static void launchSystemWithResumePrompt(Input::Event e, bool addToRecent);
	static void launchSystem(Input::Event e, bool tryAutoState, bool addToRecent);
	static bool hasArchiveExtension(const char *name);
	static void setOnMainMenuItemOptionChanged(OnMainMenuOptionChanged func);
	static void refreshCheatViews();
	[[gnu::format(printf, 3, 4)]]
	static void printfMessage(uint secs, bool error, const char *format, ...);
	static void postMessage(const char *msg);
	static void postMessage(bool error, const char *msg);
	static void postMessage(uint secs, bool error, const char *msg);
	static void postErrorMessage(const char *msg);
	static void postErrorMessage(uint secs, const char *msg);
	static void unpostMessage();
	static void printScreenshotResult(int num, bool success);
	static void saveAutoState();
	static bool loadAutoState();
	static EmuSystem::Error saveState(const char *path);
	static EmuSystem::Error saveStateWithSlot(int slot);
	static EmuSystem::Error loadState(const char *path);
	static EmuSystem::Error loadStateWithSlot(int slot);
	static void setDefaultVControlsButtonSpacing(int spacing);
	static void setDefaultVControlsButtonStagger(int stagger);
	static FS::PathString mediaSearchPath();
	static FS::PathString firmwareSearchPath();
	static void setFirmwareSearchPath(const char *path);
	static std::unique_ptr<View> makeCustomView(ViewAttachParams attach, ViewID id);
	static void addTurboInputEvent(uint action);
	static void removeTurboInputEvent(uint action);
	static FS::PathString assetPath();
	static FS::PathString libPath();
	static FS::PathString supportPath();
	static AssetIO openAppAssetIO(const char *name, IO::AccessHint access);
	static void saveSessionOptions();
	static void loadSessionOptions();
	static bool hasSavedSessionOptions();
	static void deleteSessionOptions();
	static void syncEmulationThread();

	template<class T, class Func>
	static void pushAndShowNewCollectValueInputView(ViewAttachParams attach, Input::Event e,
	const char *msgText, const char *initialContent, Func &&collectedValueFunc)
	{
		EmuApp::pushAndShowNewCollectTextInputView(attach, e, msgText, initialContent,
			[=](CollectTextInputView &view, const char *str)
			{
				auto controller = view.controller();
				assumeExpr(controller);
				if(!str)
				{
					controller->popAndShow();
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
				if(items <= 0)
				{
					postErrorMessage("Enter a value");
					return true;
				}
				else if(!collectedValueFunc(val))
				{
					return true;
				}
				else
				{
					controller->popAndShow();
					return false;
				}
			});
	}

	template<class Func1, class Func2>
	static void pushAndShowNewYesNoAlertView(ViewAttachParams attach, Input::Event e,
		const char *label, const char *yesStr, const char *noStr,
		Func1 &&onYes, Func2 &&onNo)
	{
		pushAndShowNewYesNoAlertView(attach, e, label, yesStr, noStr,
			TextMenuItem::makeSelectDelegate(std::forward<Func1>(onYes)),
			TextMenuItem::makeSelectDelegate(std::forward<Func2>(onNo)));
	}

	template <size_t S>
	static AssetIO openAppAssetIO(std::array<char, S> name, IO::AccessHint access)
	{
		return openAppAssetIO(name.data(), access);
	}
};
