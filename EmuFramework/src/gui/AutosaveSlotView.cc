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

#include "AutosaveSlotView.hh"
#include <emuframework/EmuApp.hh>
#include <imagine/gui/AlertView.hh>
#include <emuframework/viewUtils.hh>
#include <format>

namespace EmuEx
{

using SlotTextMenuItem = AutosaveSlotView::SlotTextMenuItem;

class ManageAutosavesView : public TableView, public EmuAppHelper
{
public:
	ManageAutosavesView(ViewAttachParams, AutosaveSlotView &srcView,
		const std::vector<SlotTextMenuItem> &menuItems);
	void updateItem(std::string_view name, std::string_view newName);
	bool hasItems() const { return extraSlotItems.size(); }

private:
	AutosaveSlotView &srcView;
	std::vector<SlotTextMenuItem> extraSlotItems;
};

class EditAutosaveView : public TableView, public EmuAppHelper
{
public:
	EditAutosaveView(ViewAttachParams attach, ManageAutosavesView &srcView_, std::string_view slotName_):
		TableView{slotName_, attach, menuItems},
		srcView{srcView_},
		slotName{slotName_},
		rename
		{
			"Rename", attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
					"Input name", slotName,
					[this](CollectTextInputView &, auto str)
					{
						if(appContext().fileUriExists(system().contentLocalSaveDirectory(str)))
						{
							app().postErrorMessage("A save slot with that name already exists");
							return false;
						}
						if(!app().autosaveManager.renameSlot(slotName, str))
						{
							app().postErrorMessage("Error renaming save slot");
							return false;
						}
						srcView.updateItem(slotName, str);
						dismiss();
						return true;
					});
			}
		},
		remove
		{
			"Delete", attach,
			[this](const Input::Event &e)
			{
				if(slotName == app().autosaveManager.slotName())
				{
					app().postErrorMessage("Can't delete the currently active save slot");
					return;
				}
				pushAndShowModal(makeView<YesNoAlertView>("Really delete this save slot?",
					YesNoAlertView::Delegates
					{
						.onYes = [this]
						{
							app().autosaveManager.deleteSlot(slotName);
							srcView.updateItem(slotName, "");
							if(!srcView.hasItems())
								srcView.dismiss();
							dismiss();
						}
					}), e);
			}
		} {}

private:
	ManageAutosavesView &srcView;
	std::string slotName;
	TextMenuItem rename;
	TextMenuItem remove;
	std::array<MenuItem*, 2> menuItems{&rename, &remove};
};

ManageAutosavesView::ManageAutosavesView(ViewAttachParams attach, AutosaveSlotView &srcView,
	const std::vector<SlotTextMenuItem> &items):
	TableView{"Manage Save Slots", attach, extraSlotItems},
	srcView{srcView}
{
	extraSlotItems.reserve(items.size());
	for(auto &i : items)
	{
		extraSlotItems.emplace_back(i.slotName, i.text().stringView(), attach, [this](TextMenuItem &item, const Input::Event &e)
		{
			pushAndShow(makeView<EditAutosaveView>(*this, static_cast<SlotTextMenuItem&>(item).slotName), e);
		});
	}
}

static std::string slotDescription(EmuApp &app, std::string_view saveName)
{
	auto desc = app.appContext().fileUriFormatLastWriteTimeLocal(app.autosaveManager.statePath(saveName));
	if(desc.empty())
		desc = "No saved state";
	return desc;
}

void ManageAutosavesView::updateItem(std::string_view name, std::string_view newName)
{
	auto it = std::ranges::find_if(extraSlotItems, [&](auto &i) { return i.slotName == name; });
	if(it == extraSlotItems.end()) [[unlikely]]
		return;
	if(newName.empty())
	{
		extraSlotItems.erase(it);
	}
	else
	{
		it->setName(std::format("{}: {}", newName, slotDescription(app(), newName)));
		it->slotName = newName;
	}
	place();
	srcView.updateItem(name, newName);
}

AutosaveSlotView::AutosaveSlotView(ViewAttachParams attach):
	TableView{"Autosave Slot", attach, menuItems},
	newSlot
	{
		"Create New Save Slot", attach, [this](const Input::Event &e)
		{
			pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
				"Save Slot Name", "", [this](CollectTextInputView &, auto str_)
			{
				std::string_view name{str_};
				if(appContext().fileUriExists(app().system().contentLocalSaveDirectory(name)))
				{
					app().postErrorMessage("A save slot with that name already exists");
					return false;
				}
				if(!app().autosaveManager.setSlot(name))
				{
					app().postErrorMessage("Error creating save slot");
					return false;
				}
				app().showEmulation();
				refreshItems();
				return true;
			});
		}
	},
	manageSlots
	{
		"Manage Save Slots", attach, [this](const Input::Event &e)
		{
			if(extraSlotItems.empty())
			{
				app().postMessage("No extra save slots exist");
				return;
			}
			pushAndShow(makeView<ManageAutosavesView>(*this, extraSlotItems), e);
		}
	},
	actions{"Actions", attach}
{
	refreshSlots();
	loadItems();
}

void AutosaveSlotView::refreshSlots()
{
	mainSlot =
	{
		std::format("Main: {}", slotDescription(app(), "")),
		attachParams(), [this]()
		{
			if(app().autosaveManager.setSlot(""))
			{
				app().showEmulation();
				refreshItems();
			}
		}
	};
	if(app().autosaveManager.slotName().empty())
		mainSlot.setHighlighted(true);
	extraSlotItems.clear();
	auto ctx = appContext();
	auto &sys = system();
	ctx.forEachInDirectoryUri(sys.contentLocalSaveDirectory(), [&](const FS::directory_entry &e)
	{
		if(e.type() != FS::file_type::directory)
			return true;
		auto &item = extraSlotItems.emplace_back(e.name(), std::format("{}: {}", e.name(), slotDescription(app(), e.name())),
			attachParams(), [this](TextMenuItem &item)
		{
			if(app().autosaveManager.setSlot(static_cast<SlotTextMenuItem&>(item).slotName))
			{
				app().showEmulation();
				refreshItems();
			}
		});
		if(app().autosaveManager.slotName() == e.name())
			item.setHighlighted(true);
		return true;
	}, {.test = true});
	noSaveSlot =
	{
		"No Save",
		attachParams(), [this]()
		{
			if(app().autosaveManager.setSlot(noAutosaveName))
			{
				app().showEmulation();
				refreshItems();
			}
		}
	};
	if(app().autosaveManager.slotName() == noAutosaveName)
		noSaveSlot.setHighlighted(true);
}

void AutosaveSlotView::refreshItems()
{
	refreshSlots();
	loadItems();
	place();
}

void AutosaveSlotView::loadItems()
{
	menuItems.clear();
	if(!system().hasContent())
		return;
	menuItems.emplace_back(&mainSlot);
	for(auto &i : extraSlotItems)
		menuItems.emplace_back(&i);
	menuItems.emplace_back(&noSaveSlot);
	menuItems.emplace_back(&actions);
	menuItems.emplace_back(&newSlot);
	menuItems.emplace_back(&manageSlots);
	manageSlots.setActive(extraSlotItems.size());
}

void AutosaveSlotView::updateItem(std::string_view name, std::string_view newName)
{
	auto it = std::ranges::find_if(extraSlotItems, [&](auto &i) { return i.slotName == name; });
	if(it == extraSlotItems.end()) [[unlikely]]
		return;
	if(newName.empty())
	{
		extraSlotItems.erase(it);
		loadItems();
	}
	else
	{
		it->setName(std::format("{}: {}", newName, slotDescription(app(), newName)));
		it->slotName = newName;
	}
	place();
}

}
