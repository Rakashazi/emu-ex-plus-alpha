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
#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/RecentGameView.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuOptions.hh>
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
				startGameFromMenu();
			}
		},
		hard
		{
			"Hard Reset",
			[this]()
			{
				dismiss();
				EmuSystem::reset(EmuSystem::RESET_HARD);
				startGameFromMenu();
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
	item.emplace_back(&close);
}

EmuSystemActionsView::EmuSystemActionsView(ViewAttachParams attach, bool customMenu):
	TableView{"System Actions", attach, item},
	cheats
	{
		"Cheats",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &cheatsMenu = *makeView(attachParams(), EmuApp::ViewID::LIST_CHEATS);
				pushAndShow(cheatsMenu, e);
			}
		}
	},
	reset
	{
		"Reset",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(EmuSystem::hasResetModes)
				{
					auto &resetAlertView = *new ResetAlertView{attachParams(), "Really reset?"};
					modalViewController.pushAndShow(resetAlertView, e, false);
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really reset?"};
					ynAlertView.setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							EmuSystem::reset(EmuSystem::RESET_SOFT);
							startGameFromMenu();
						});
					modalViewController.pushAndShow(ynAlertView, e, false);
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
				auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really load state?"};
				ynAlertView.setOnYes(
					[](TextMenuItem &, View &view, Input::Event e)
					{
						view.dismiss();
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							popup.printf(4, true, "Load State: %s", err->what());
						}
						else
							startGameFromMenu();
					});
				modalViewController.pushAndShow(ynAlertView, e, false);
			}
		}
	},
	saveState
	{
		"Save State",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				static auto doSaveState =
					[]()
					{
						if(auto err = EmuApp::saveStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							popup.printf(4, true, "Save State: %s", err->what());
						}
						else
							startGameFromMenu();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really overwrite state?"};
					ynAlertView.setOnYes(
						[](TextMenuItem &, View &view, Input::Event e)
						{
							view.dismiss();
							doSaveState();
						});
					modalViewController.pushAndShow(ynAlertView, e, false);
				}
			}
		}
	},
	stateSlot
	{
		stateSlotText,
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &ssMenu = *new StateSlotView{attachParams()};
			pushAndShow(ssMenu, e);
		}
	},
	stateSlotText // Can't init with string literal due to GCC bug #43453
		{'S', 't', 'a', 't', 'e', ' ', 'S', 'l', 'o', 't', ' ', '(', '0', ')' },
	#if defined CONFIG_BASE_ANDROID && !defined CONFIG_MACHINE_OUYA
	addLauncherIcon
	{
		"Add Game Shortcut to Launcher",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				if(!strlen(EmuSystem::gamePath()))
				{
					// shortcuts to bundled games not yet supported
					return;
				}
				EmuApp::pushAndShowNewCollectTextInputView(attachParams(), e, "Shortcut Name", EmuSystem::fullGameName().data(),
					[this](CollectTextInputView &view, const char *str)
					{
						if(str && strlen(str))
						{
							Base::addLauncherIcon(str, EmuSystem::fullGamePath());
							popup.printf(2, false, "Added shortcut:\n%s", str);
						}
						view.dismiss();
						return 0;
					});
			}
			else
			{
				popup.post("Load a game first");
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
				EmuSystem::runFrame(&emuVideo, false);
			}
		}
	},
	close
	{
		"Close Game",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			auto &ynAlertView = *new YesNoAlertView{attachParams(), "Really close current game?"};
			ynAlertView.setOnYes(
				[this](TextMenuItem &, View &view, Input::Event)
				{
					view.dismiss();
					EmuSystem::closeGame(true);
					popAndShow();
				});
			modalViewController.pushAndShow(ynAlertView, e, false);
		}
	}
{
	if(!customMenu)
	{
		loadStandardItems();
	}
}
