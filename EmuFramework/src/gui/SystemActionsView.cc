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

#include <emuframework/SystemActionsView.hh>
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

static auto autoSaveName(EmuApp &app)
{
	return std::format("Autosave Slot ({})", app.autosaveManager().slotFullName());
}

static std::string saveAutosaveName(EmuApp &app)
{
	auto &autosaveManager = app.autosaveManager();
	if(!autosaveManager.timerFrequency().count())
		return "Save Autosave State";
	return std::format("Save Autosave State (Timer In {:%M:%S})",
		duration_cast<Seconds>(autosaveManager.nextTimerFireTime()));
}

SystemActionsView::SystemActionsView(ViewAttachParams attach, bool customMenu):
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
					pushAndShowModal(makeView<YesNoAlertView>("Really reset?",
						YesNoAlertView::Delegates
						{
							.onYes = [this]
							{
								system().reset(app(), EmuSystem::ResetMode::SOFT);
								app().showEmulation();
							}
						}), e);
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
		saveAutosaveName(app()), &defaultFace(),
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!item.active())
				return;
			pushAndShowModal(makeView<YesNoAlertView>("Really save state?",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						if(app().autosaveManager().save(AutosaveActionSource::Manual))
							app().showEmulation();
					}
				}), e);
		}
	},
	revertAutosave
	{
		"Load Autosave State", &defaultFace(),
		[this](TextMenuItem &item, const Input::Event &e)
		{
			if(!item.active())
				return;
			auto saveTime = app().autosaveManager().stateTimeAsString();
			if(saveTime.empty())
			{
				app().postMessage("No saved state");
				return;
			}
			pushAndShowModal(makeView<YesNoAlertView>(std::format("Really load state from: {}?", saveTime),
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						if(app().autosaveManager().load(AutosaveActionSource::Manual))
							app().showEmulation();
					}
				}), e);
		}
	},
	stateSlot
	{
		"Manual Save States", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeView<StateSlotView>(), e);
		}
	},
	addLauncherIcon
	{
		"Add Content Shortcut To Launcher", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!system().hasContent())
				return;
			if(system().contentDirectory().empty())
			{
				// shortcuts to bundled games not yet supported
				return;
			}
			app().pushAndShowNewCollectValueInputView<const char*>(attachParams(), e, "Shortcut Name", system().contentDisplayName(),
				[this](EmuApp &app, auto str)
				{
					appContext().addLauncherIcon(str, app.system().contentLocation());
					app.postMessage(2, false, std::format("Added shortcut:\n{}", str));
					return true;
				});
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
			pushAndShowModal(makeView<YesNoAlertView>(std::format("Save screenshot to folder {}?", pathName),
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						app().video().takeGameScreenshot();
						system().runFrame({}, &app().video(), nullptr);
					}
				}), e);
		}
	},
	resetSessionOptions
	{
		"Reset Saved Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(!app().hasSavedSessionOptions())
				return;
			pushAndShowModal(makeView<YesNoAlertView>(
				"Reset saved options for the currently running system to defaults? Some options only take effect next time the system loads.",
				YesNoAlertView::Delegates
				{
					.onYes = [this]
					{
						resetSessionOptions.setActive(false);
						app().deleteSessionOptions();
					}
				}), e);
		}
	},
	close
	{
		"Close Content", &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShowModal(makeView<YesNoAlertView>("Really close current content?",
				YesNoAlertView::Delegates
				{
					.onYes = [this] { app().closeSystem(); return false; } // pops any System Actions views in the stack
				}), e);
		}
	}
{
	if(!customMenu)
	{
		loadStandardItems();
	}
}

void SystemActionsView::onShow()
{
	if(app().viewController().isShowingEmulation())
		return;
	TableView::onShow();
	logMsg("refreshing action menu state");
	assert(system().hasContent());
	autosaveSlot.compile(autoSaveName(app()), renderer());
	autosaveNow.compile(saveAutosaveName(app()), renderer());
	autosaveNow.setActive(app().autosaveManager().slotName() != noAutosaveName);
	revertAutosave.setActive(app().autosaveManager().slotName() != noAutosaveName);
	resetSessionOptions.setActive(app().hasSavedSessionOptions());
}

void SystemActionsView::loadStandardItems()
{
	if(EmuSystem::hasCheats)
	{
		item.emplace_back(&cheats);
	}
	item.emplace_back(&reset);
	item.emplace_back(&autosaveSlot);
	item.emplace_back(&revertAutosave);
	item.emplace_back(&autosaveNow);
	item.emplace_back(&stateSlot);
	if(used(addLauncherIcon))
		item.emplace_back(&addLauncherIcon);
	item.emplace_back(&screenshot);
	item.emplace_back(&resetSessionOptions);
	item.emplace_back(&close);
}

}
