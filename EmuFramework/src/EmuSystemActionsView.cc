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

#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/RecentGameView.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/InputManagerView.hh>
#include <emuframework/TouchConfigView.hh>
#include <emuframework/BundledGamesView.hh>
#include "private.hh"

class ResetAlertView : public BaseAlertView
{
public:
	ResetAlertView(ViewAttachParams attach, const char *label):
		BaseAlertView(attach, label,
			[this](const TableView &)
			{
				return 3;
			},
			[this](const TableView &, int idx) -> MenuItem&
			{
				switch(idx)
				{
					default: bug_unreachable("idx == %d", idx); [[fallthrough]];
					case 0: return soft;
					case 1: return hard;
					case 2: return cancel;
				}
			}),
		soft
		{
			"Soft Reset",
			[this]()
			{
				dismiss();
				EmuSystem::reset(EmuSystem::RESET_SOFT);
				emuViewController.showEmulation();
			}
		},
		hard
		{
			"Hard Reset",
			[this]()
			{
				dismiss();
				EmuSystem::reset(EmuSystem::RESET_HARD);
				emuViewController.showEmulation();
			}
		},
		cancel
		{
			"Cancel",
			[this]()
			{
				dismiss();
			}
		}
	{}

protected:
	TextMenuItem soft, hard, cancel;
};

void EmuSystemActionsView::onShow()
{
	TableView::onShow();
	logMsg("refreshing action menu state");
	cheats.setActive(EmuSystem::gameIsRunning());
	reset.setActive(EmuSystem::gameIsRunning());
	saveState.setActive(EmuSystem::gameIsRunning());
	loadState.setActive(EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot));
	stateSlotText[12] = EmuSystem::saveSlotChar(EmuSystem::saveStateSlot);
	stateSlot.compile(renderer(), projP);
	screenshot.setActive(EmuSystem::gameIsRunning());
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon.setActive(EmuSystem::gameIsRunning());
	#endif
	resetSessionOptions.setActive(EmuApp::hasSavedSessionOptions());
	close.setActive(EmuSystem::gameIsRunning());
}

void EmuSystemActionsView::loadStandardItems()
{
	if(EmuSystem::hasCheats)
	{
		item.emplace_back(&cheats);
	}
	item.emplace_back(&reset);
	item.emplace_back(&loadState);
	item.emplace_back(&saveState);
	stateSlotText[12] = EmuSystem::saveSlotChar(EmuSystem::saveStateSlot);
	item.emplace_back(&stateSlot);
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	item.emplace_back(&addLauncherIcon);
	#endif
	item.emplace_back(&screenshot);
	item.emplace_back(&resetSessionOptions);
	item.emplace_back(&close);
}

EmuSystemActionsView::EmuSystemActionsView(ViewAttachParams attach, bool customMenu):
	TableView{"System Actions", attach, item},
	cheats
	{
		"Cheats",
		[this](Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				pushAndShow(makeEmuView(attachParams(), EmuApp::ViewID::LIST_CHEATS), e);
			}
		}
	},
	reset
	{
		"Reset",
		[this](Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(EmuSystem::hasResetModes)
				{
					emuViewController.pushAndShowModal(makeView<ResetAlertView>("Really reset?"), e, false);
				}
				else
				{
					auto ynAlertView = makeView<YesNoAlertView>("Really reset?");
					ynAlertView->setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							EmuSystem::reset(EmuSystem::RESET_SOFT);
							emuViewController.showEmulation();
						});
					emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
				}
			}
		}
	},
	loadState
	{
		"Load State",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			if(item.active() && EmuSystem::gameIsRunning())
			{
				auto ynAlertView = std::make_unique<YesNoAlertView>(attachParams(), "Really load state?");
				ynAlertView->setOnYes(
					[](TextMenuItem &, View &view, Input::Event e)
					{
						view.dismiss();
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							EmuApp::printfMessage(4, true, "Load State: %s", err->what());
						}
						else
							emuViewController.showEmulation();
					});
				emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
			}
		}
	},
	saveState
	{
		"Save State",
		[this](Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				static auto doSaveState =
					[]()
					{
						if(auto err = EmuApp::saveStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							EmuApp::printfMessage(4, true, "Save State: %s", err->what());
						}
						else
							emuViewController.showEmulation();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto ynAlertView = std::make_unique<YesNoAlertView>(attachParams(), "Really overwrite state?");
					ynAlertView->setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							doSaveState();
						});
					emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
				}
			}
		}
	},
	stateSlot
	{
		stateSlotText,
		[this](Input::Event e)
		{
			pushAndShow(makeView<StateSlotView>(), e);
		}
	},
	stateSlotText // Can't init with string literal due to GCC bug #43453
		{'S', 't', 'a', 't', 'e', ' ', 'S', 'l', 'o', 't', ' ', '(', '0', ')' },
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon
	{
		"Add Game Shortcut to Launcher",
		[this](Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(!strlen(EmuSystem::gamePath()))
				{
					// shortcuts to bundled games not yet supported
					return;
				}
				EmuApp::pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Shortcut Name", EmuSystem::fullGameName().data(),
					[this](auto str)
					{
						Base::addLauncherIcon(str, EmuSystem::fullGamePath());
						EmuApp::printfMessage(2, false, "Added shortcut:\n%s", str);
						return true;
					});
			}
			else
			{
				EmuApp::postMessage("Load a game first");
			}
		}
	},
	#endif
	screenshot
	{
		"Screenshot Next Frame",
		[]()
		{
			if(EmuSystem::gameIsRunning())
			{
				emuVideo.takeGameScreenshot();
				EmuSystem::runFrame(nullptr, &emuVideo, nullptr);
			}
		}
	},
	resetSessionOptions
	{
		"Reset Saved Options",
		[this](Input::Event e)
		{
			if(!EmuApp::hasSavedSessionOptions())
				return;
			auto ynAlertView = makeView<YesNoAlertView>(
				"Reset saved options for the currently running system to defaults? Some options only take effect next time the system loads.");
			ynAlertView->setOnYes(
				[this](TextMenuItem &item, View &view, Input::Event)
				{
					resetSessionOptions.setActive(false);
					view.dismiss();
					EmuApp::deleteSessionOptions();
				});
			emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
		}
	},
	close
	{
		"Close Game",
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really close current game?");
			ynAlertView->setOnYes(
				[this](TextMenuItem &, View &view, Input::Event)
				{
					view.dismiss();
					emuViewController.closeSystem(true); // pops any System Actions views in stack
				});
			emuViewController.pushAndShowModal(std::move(ynAlertView), e, false);
		}
	}
{
	if(!customMenu)
	{
		loadStandardItems();
	}
}
