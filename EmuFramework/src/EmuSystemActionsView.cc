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

#include <emuframework/EmuSystemActionsView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuVideo.hh>
#include <emuframework/CreditsView.hh>
#include <emuframework/StateSlotView.hh>
#include <emuframework/InputManagerView.hh>
#include <emuframework/BundledGamesView.hh>
#include "AutosaveSlotView.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/TextEntry.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

class ResetAlertView : public BaseAlertView, public EmuAppHelper<ResetAlertView>
{
public:
	ResetAlertView(ViewAttachParams attach, UTF16Convertible auto &&label, EmuSystem &sys):
		BaseAlertView{attach, IG_forward(label), items},
		items
		{
			TextMenuItem
			{
				"Soft Reset", &defaultFace(),
				[this, &sys]()
				{
					sys.reset(app(), EmuSystem::ResetMode::SOFT);
					app().showEmulation();
				}
			},
			TextMenuItem
			{
				"Hard Reset", &defaultFace(),
				[this, &sys]()
				{
					sys.reset(app(), EmuSystem::ResetMode::HARD);
					app().showEmulation();
				}
			},
			TextMenuItem{"Cancel", &defaultFace(), [](){}}
		} {}

protected:
	std::array<TextMenuItem, 3> items;
};

static auto makeStateSlotStr(EmuSystem &sys, int slot)
{
	return fmt::format("State Slot ({})", sys.saveSlotChar(slot));
}

static auto autoSaveName(EmuApp &app)
{
	return fmt::format("Autosave Slot ({})", app.currentAutosaveName());
}

void EmuSystemActionsView::onShow()
{
	if(app().viewController().isShowingEmulation())
		return;
	TableView::onShow();
	logMsg("refreshing action menu state");
	auto hasContent = system().hasContent();
	cheats.setActive(hasContent);
	reset.setActive(hasContent);
	autosaveSlot.compile(autoSaveName(app()), renderer(), projP);
	autosaveNow.setActive(hasContent && app().currentAutosave() != noAutosaveName);
	revertAutosave.setActive(hasContent && app().currentAutosave() != noAutosaveName);
	saveState.setActive(hasContent);
	loadState.setActive(hasContent && system().stateExists(system().stateSlot()));
	stateSlot.compile(makeStateSlotStr(system(), system().stateSlot()), renderer(), projP);
	screenshot.setActive(hasContent);
	doIfUsed(addLauncherIcon, [&](auto &mItem){ mItem.setActive(hasContent); });
	resetSessionOptions.setActive(app().hasSavedSessionOptions());
	close.setActive(hasContent);
}

void EmuSystemActionsView::loadStandardItems()
{
	if(EmuSystem::hasCheats)
	{
		item.emplace_back(&cheats);
	}
	item.emplace_back(&reset);
	item.emplace_back(&revertAutosave);
	item.emplace_back(&autosaveNow);
	item.emplace_back(&autosaveSlot);
	item.emplace_back(&loadState);
	item.emplace_back(&saveState);
	item.emplace_back(&stateSlot);
	if(used(addLauncherIcon))
		item.emplace_back(&addLauncherIcon);
	item.emplace_back(&screenshot);
	item.emplace_back(&resetSessionOptions);
	item.emplace_back(&close);
}

EmuSystemActionsView::EmuSystemActionsView(ViewAttachParams attach, bool customMenu):
	TableView{"System Actions", attach, item},
	cheats
	{
		"Cheats", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(system().hasContent())
			{
				pushAndShow(EmuApp::makeView(attachParams(), EmuApp::ViewID::LIST_CHEATS), e);
			}
		}
	},
	reset
	{
		"Reset", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(system().hasContent())
			{
				if(EmuSystem::hasResetModes)
				{
					pushAndShowModal(makeView<ResetAlertView>("Really reset?", system()), e);
				}
				else
				{
					auto ynAlertView = makeView<YesNoAlertView>("Really reset?");
					ynAlertView->setOnYes(
						[this]()
						{
							system().reset(app(), EmuSystem::ResetMode::SOFT);
							app().showEmulation();
						});
					pushAndShowModal(std::move(ynAlertView), e);
				}
			}
		}
	},
	autosaveSlot
	{
		autoSaveName(app()), &defaultFace(),
		[this](const Input::Event &e) { pushAndShow(makeView<AutosaveSlotView>(), e); }
	},
	autosaveNow
	{
		"Save Autosave State", &defaultFace(),
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!item.active())
				return;
			auto ynAlertView = makeView<YesNoAlertView>("Really save state?");
			ynAlertView->setOnYes(
				[this]()
				{
					if(app().saveAutosave())
						app().showEmulation();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	revertAutosave
	{
		"Load Autosave State", &defaultFace(),
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!item.active())
				return;
			auto saveTime = app().currentAutosaveTimeAsString();
			if(saveTime.empty())
			{
				app().postMessage("No saved state");
				return;
			}
			auto ynAlertView = makeView<YesNoAlertView>(fmt::format("Really load state from: {}?", saveTime));
			ynAlertView->setOnYes(
				[this]()
				{
					if(app().loadAutosave())
						app().showEmulation();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	loadState
	{
		"Load State", &defaultFace(),
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			if(item.active() && system().hasContent())
			{
				auto ynAlertView = makeView<YesNoAlertView>("Really load state?");
				ynAlertView->setOnYes(
					[this]()
					{
						if(app().loadStateWithSlot(system().stateSlot()))
							app().showEmulation();
					});
				pushAndShowModal(std::move(ynAlertView), e);
			}
		}
	},
	saveState
	{
		"Save State", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(system().hasContent())
			{
				static auto doSaveState =
					[](EmuApp &app)
					{
						if(app.saveStateWithSlot(app.system().stateSlot()))
							app.showEmulation();
					};
				if(app().shouldOverwriteExistingState())
				{
					doSaveState(app());
				}
				else
				{
					auto ynAlertView = makeView<YesNoAlertView>("Really overwrite state?");
					ynAlertView->setOnYes(
						[this]()
						{
							doSaveState(app());
						});
					pushAndShowModal(std::move(ynAlertView), e);
				}
			}
		}
	},
	stateSlot
	{
		makeStateSlotStr(system(), system().stateSlot()), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<StateSlotView>(), e);
		}
	},
	addLauncherIcon
	{
		"Add Game Shortcut to Launcher", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(system().hasContent())
			{
				if(system().contentDirectory().empty())
				{
					// shortcuts to bundled games not yet supported
					return;
				}
				app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Shortcut Name", system().contentDisplayName(),
					[this](EmuApp &app, auto str)
					{
						appContext().addLauncherIcon(str, app.system().contentLocation());
						app.postMessage(2, false, fmt::format("Added shortcut:\n{}", str));
						return true;
					});
			}
			else
			{
				app().postMessage("Load a game first");
			}
		}
	},
	screenshot
	{
		"Screenshot Next Frame", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			auto pathName = appContext().fileUriDisplayName(app().screenshotDirectory());
			if(pathName.empty())
			{
				app().postMessage("Save path isn't valid");
				return;
			}
			auto ynAlertView = makeView<YesNoAlertView>(fmt::format("Save screenshot to folder {}?", pathName));
			ynAlertView->setOnYes(
				[this]()
				{
					app().video().takeGameScreenshot();
					system().runFrame({}, &app().video(), nullptr);
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	resetSessionOptions
	{
		"Reset Saved Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!app().hasSavedSessionOptions())
				return;
			auto ynAlertView = makeView<YesNoAlertView>(
				"Reset saved options for the currently running system to defaults? Some options only take effect next time the system loads.");
			ynAlertView->setOnYes(
				[this]()
				{
					resetSessionOptions.setActive(false);
					app().deleteSessionOptions();
				});
			pushAndShowModal(std::move(ynAlertView), e);
		}
	},
	close
	{
		"Close Game", &defaultFace(),
		[this](const Input::Event &e)
		{
			auto ynAlertView = makeView<YesNoAlertView>("Really close current game?");
			ynAlertView->setOnYes(
				[this]()
				{
					app().closeSystem(); // pops any System Actions views in stack
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

}
