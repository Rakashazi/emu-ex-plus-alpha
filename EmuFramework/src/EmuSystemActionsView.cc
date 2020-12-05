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
#include <emuframework/CreditsView.hh>
#include <emuframework/FilePicker.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/InputManagerView.hh>
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
				EmuSystem::reset(EmuSystem::RESET_SOFT);
				emuViewController().showEmulation();
			}
		},
		hard
		{
			"Hard Reset",
			[this]()
			{
				EmuSystem::reset(EmuSystem::RESET_HARD);
				emuViewController().showEmulation();
			}
		},
		cancel
		{
			"Cancel",
			[](){}
		}
	{}

protected:
	TextMenuItem soft, hard, cancel;
};

static std::array<char, 16> makeStateSlotStr(int slot)
{
	return string_makePrintf<16>("State Slot (%c)", EmuSystem::saveSlotChar(slot));
}

void EmuSystemActionsView::onShow()
{
	TableView::onShow();
	logMsg("refreshing action menu state");
	cheats.setActive(EmuSystem::gameIsRunning());
	reset.setActive(EmuSystem::gameIsRunning());
	saveState.setActive(EmuSystem::gameIsRunning());
	loadState.setActive(EmuSystem::gameIsRunning() && EmuSystem::stateExists(EmuSystem::saveStateSlot));
	stateSlot.compile(makeStateSlotStr(EmuSystem::saveStateSlot).data(), renderer(), projP);
	screenshot.setActive(EmuSystem::gameIsRunning());
	#ifdef CONFIG_EMUFRAMEWORK_ADD_LAUNCHER_ICON
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
	stateSlot.setName(makeStateSlotStr(EmuSystem::saveStateSlot).data());
	item.emplace_back(&stateSlot);
	#ifdef CONFIG_EMUFRAMEWORK_ADD_LAUNCHER_ICON
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
					pushAndShowModal(makeView<ResetAlertView>("Really reset?"), e);
				}
				else
				{
					auto ynAlertView = makeView<YesNoAlertView>("Really reset?");
					ynAlertView->setOnYes(
						[]()
						{
							EmuSystem::reset(EmuSystem::RESET_SOFT);
							emuViewController().showEmulation();
						});
					pushAndShowModal(std::move(ynAlertView), e);
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
					[]()
					{
						if(auto err = EmuApp::loadStateWithSlot(EmuSystem::saveStateSlot);
							err)
						{
							EmuApp::printfMessage(4, true, "Load State: %s", err->what());
						}
						else
							emuViewController().showEmulation();
					});
				pushAndShowModal(std::move(ynAlertView), e);
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
							emuViewController().showEmulation();
					};

				if(EmuSystem::shouldOverwriteExistingState())
				{
					doSaveState();
				}
				else
				{
					auto ynAlertView = std::make_unique<YesNoAlertView>(attachParams(), "Really overwrite state?");
					ynAlertView->setOnYes(
						[]()
						{
							doSaveState();
						});
					pushAndShowModal(std::move(ynAlertView), e);
				}
			}
		}
	},
	stateSlot
	{
		nullptr,
		[this](Input::Event e)
		{
			pushAndShow(makeView<StateSlotView>(), e);
		}
	},
	#ifdef CONFIG_EMUFRAMEWORK_ADD_LAUNCHER_ICON
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
		[this](Input::Event e)
		{
			if(!EmuSystem::gameIsRunning())
				return;
			auto ynAlertView = makeView<YesNoAlertView>(string_makePrintf<1024>("Save screenshot to %s ?", EmuSystem::savePath()).data());
			ynAlertView->setOnYes(
				[]()
				{
					emuVideo.takeGameScreenshot();
					EmuSystem::runFrame(nullptr, &emuVideo, nullptr);
				});
			pushAndShowModal(std::move(ynAlertView), e);
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
				[this]()
				{
					resetSessionOptions.setActive(false);
					EmuApp::deleteSessionOptions();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	close
	{
		"Close Game",
		[this](Input::Event e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really close current game?");
			ynAlertView->setOnYes(
				[this]()
				{
					emuViewController().closeSystem(true); // pops any System Actions views in stack
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	}
{
	if(!customMenu)
	{
		loadStandardItems();
	}
}
