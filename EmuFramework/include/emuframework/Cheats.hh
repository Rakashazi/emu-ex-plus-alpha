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

#include <emuframework/EmuAppHelper.hh>
#include <emuframework/viewUtils.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <vector>

namespace EmuEx
{

using namespace IG;
using RefreshCheatsDelegate = DelegateFunc<void ()>;

class BaseCheatsView : public TableView, public EmuAppHelper<BaseCheatsView>
{
public:
	BaseCheatsView(ViewAttachParams attach);

protected:
	TextMenuItem edit;
	std::vector<BoolMenuItem> cheat;

	virtual void loadCheatItems() = 0;
};

class BaseEditCheatListView : public TableView, public EmuAppHelper<BaseEditCheatListView>
{
public:
	BaseEditCheatListView(ViewAttachParams attach, TableView::ItemSourceDelegate);
	void setOnCheatListChanged(RefreshCheatsDelegate del);

protected:
	std::vector<TextMenuItem> cheat;
	RefreshCheatsDelegate onCheatListChanged_;

	void onCheatListChanged();
	virtual void loadCheatItems() = 0;
};

template <class CheatViewImpl>
class BaseEditCheatView : public TableView, public EmuAppHelper<BaseEditCheatView<CheatViewImpl>>
{
public:
	using EmuAppHelper<BaseEditCheatView<CheatViewImpl>>::app;

	BaseEditCheatView(UTF16Convertible auto &&viewName, ViewAttachParams attach, UTF16Convertible auto &&cheatName,
		TableView::ItemSourceDelegate itemSrc, TextMenuItem::SelectDelegate removed,
		RefreshCheatsDelegate onCheatListChanged_):
		TableView
		{
			IG_forward(viewName),
			attach,
			itemSrc
		},
		name
		{
			IG_forward(cheatName), attach,
			[this](const Input::Event &e)
			{
				pushAndShowNewCollectValueInputView<const char*>(attachParams(), e,
					"Input description", static_cast<CheatViewImpl*>(this)->cheatNameString(),
					[this](CollectTextInputView&, auto str)
					{
						name.compile(str);
						static_cast<CheatViewImpl*>(this)->renamed(str);
						onCheatListChanged();
						postDraw();
						return true;
					});
			}
		},
		remove
		{
			"Delete Cheat", attach,
			removed
		},
		onCheatListChanged_{onCheatListChanged_} {}


protected:
	TextMenuItem name, remove;
	RefreshCheatsDelegate onCheatListChanged_;

	void onCheatListChanged()
	{
		onCheatListChanged_.callSafe();
	}
};

}
