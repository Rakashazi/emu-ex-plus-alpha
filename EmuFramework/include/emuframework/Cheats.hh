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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/viewUtils.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/gui/MenuItem.hh>
#include <vector>

namespace EmuEx
{

using namespace IG;

class CheatsView : public TableView, public EmuAppHelper
{
public:
	CheatsView(ViewAttachParams attach):
		TableView
		{
			"Cheats",
			attach,
			[this](ItemMessage msg) -> ItemReply
			{
				return msg.visit(overloaded
				{
					[&](const ItemsMessage&) -> ItemReply { return 1 + cheats.size(); },
					[&](const GetItemMessage& m) -> ItemReply
					{
						if(m.idx == 0)
							return &edit;
						else
							return &cheats[m.idx - 1];
					},
				});
			}
		},
		edit
		{
			"Add/Edit", attach,
			[this](const Input::Event &e)
			{
				auto editCheatsView = app().makeEditCheatsView(attachParams(), *this);
				pushAndShow(std::move(editCheatsView), e);
			}
		}
	{
		loadCheatItems();
	}

	void onCheatsChanged()
	{
		auto selectedCell = selected;
		loadCheatItems();
		highlightCell(selectedCell);
		place();
	}

protected:
	TextMenuItem edit;
	std::vector<BoolMenuItem> cheats;

	void loadCheatItems()
	{
		cheats.clear();
		system().forEachCheat([this](auto& c, std::string_view name)
		{
			cheats.emplace_back(name, attachParams(), system().isCheatEnabled(c), [this, &c](BoolMenuItem& item)
			{
				system().setCheatEnabled(c, item.flipBoolValue(*this));
			});
			return true;
		});
	}
};

class BaseEditCheatsView : public TableView, public EmuAppHelper
{
public:
	BaseEditCheatsView(ViewAttachParams attach, CheatsView& cheatsView, TableView::ItemSourceDelegate itemSrc):
		TableView
		{
			"Edit Cheats",
			attach,
			itemSrc
		},
		cheatsViewPtr{&cheatsView}
	{
		loadCheatItems();
	}

	void onCheatsChanged()
	{
		auto selectedCell = selected;
		loadCheatItems();
		highlightCell(selectedCell);
		place();
		cheatsViewPtr->onCheatsChanged();
	}

protected:
	std::vector<TextMenuItem> cheats;
	CheatsView* cheatsViewPtr;

	void loadCheatItems()
	{
		cheats.clear();
		system().forEachCheat([this](Cheat& c, std::string_view name)
		{
			cheats.emplace_back(name, attachParams(), [this, &c](const Input::Event& e)
			{
				pushAndShow(app().makeEditCheatView(attachParams(), c, *this), e);
			});
			return true;
		});
	}

	void addNewCheat(const char* promptStr, const Input::Event& e, unsigned flags = 0)
	{
		pushAndShowNewCollectTextInputView(attachParams(), e, promptStr, "",
			[this, flags](CollectTextInputView& view, const char* str)
			{
				auto cheatPtr = system().newCheat(app(), "", {str, flags});
				if(!cheatPtr)
					return true;
				onCheatsChanged();
				view.dismiss();
				pushAndShowNewCollectTextInputView(attachParams(), {}, "Input description", "",
					[this, &cheat = *cheatPtr](CollectTextInputView &view, const char *str)
					{
						if(!system().setCheatName(cheat, str))
						{
							app().postMessage(true, "A cheat with name already exists");
							return true;
						}
						onCheatsChanged();
						view.dismiss();
						return false;
					});
				return false;
			});
	}
};

class BaseEditCheatView : public TableView, public EmuAppHelper
{
public:
	BaseEditCheatView(UTF16Convertible auto &&viewName, ViewAttachParams attach, Cheat& cheat,
		BaseEditCheatsView& editCheatsView_, TableView::ItemSourceDelegate itemSrc):
		TableView
		{
			IG_forward(viewName),
			attach,
			itemSrc
		},
		cheatPtr{&cheat},
		editCheatsView{editCheatsView_},
		name
		{
			system().cheatName(cheat), attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
					"Input description", system().cheatName(*cheatPtr),
					[this](CollectTextInputView&, auto str)
					{
						if(!system().setCheatName(*cheatPtr, str))
						{
							app().postMessage(true, "A cheat with name already exists");
							return false;
						}
						name.compile(str);
						onCheatsChanged();
						postDraw();
						return true;
					});
			}
		},
		remove
		{
			"Delete", attach,
			[this](const Input::Event &e)
			{
				pushAndShowModal(makeView<YesNoAlertView>("Really delete this cheat?",
					YesNoAlertView::Delegates{.onYes = [this]{ removeCheat(); }}), e);
			}
		} {}

	void onCheatsChanged() { editCheatsView.onCheatsChanged(); }

	void removeCheat()
	{
		system().removeCheat(*cheatPtr);
		onCheatsChanged();
		dismiss();
	}

	void removeCheatCode(this auto&& self, CheatCode& c)
	{
		self.cheatPtr = self.system().removeCheatCode(*self.cheatPtr, c);
		self.onCheatsChanged();
		if(!self.cheatPtr)
			self.dismiss();
		else
			self.loadItems();
	}

	bool modifyCheatCode(this auto&& self, CheatCode& c, CheatCodeDesc desc)
	{
		if(!strlen(desc.str))
		{
			self.removeCheatCode(c);
			return true;
		}
		if(!self.system().modifyCheatCode(self.app(), *self.cheatPtr, c, desc))
		{
			self.postDraw();
			return false;
		}
		self.onCheatsChanged();
		self.loadItems();
		self.place();
		self.postDraw();
		return true;
	}

protected:
	Cheat* cheatPtr;
	BaseEditCheatsView& editCheatsView;
	std::vector<MenuItem*> items;
	std::vector<DualTextMenuItem> codes;
	TextMenuItem name, remove;

	void addNewCheatCode(this auto&& self, const char* promptStr, const Input::Event& e, unsigned flags = 0)
	{
		pushAndShowNewCollectTextInputView(self.attachParams(), e, promptStr, "",
			[&self, flags](CollectTextInputView& view, const char* str)
			{
				if(!self.system().addCheatCode(self.app(), self.cheatPtr, {str, flags}))
					return true;
				self.loadItems();
				self.onCheatsChanged();
				view.dismiss();
				return false;
			});
	}
};

}
