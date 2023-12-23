#pragma once

/*  This file is part of Imagine.

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

#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/utility.h>
#include <vector>
#include <cassert>

namespace IG
{

class TextTableView : public TableView
{
public:
	TextTableView(UTF16Convertible auto &&name, ViewAttachParams attach, size_t itemsHint):
		TableView{IG_forward(name), attach, textItem}
	{
		textItem.reserve(itemsHint);
	}

	TextTableView(ViewAttachParams attach, size_t itemsHint): TextTableView{UTF16String{}, attach, itemsHint} {}

	TextMenuItem &appendItem(UTF16Convertible auto &&name, TextMenuItem::SelectDelegate del)
	{
		return textItem.emplace_back(IG_forward(name), attachParams(), del);
	}

	TextMenuItem &setItem(size_t idx, UTF16Convertible auto &&name, TextMenuItem::SelectDelegate del)
	{
		assert(idx < textItem.size());
		textItem[idx] = {IG_forward(name), attachParams(), del};
		return textItem[idx];
	}

	TextMenuItem &item(size_t idx);
	void onAddedToController(ViewController *, const Input::Event &) override;
	void drawElement(Gfx::RendererCommands &__restrict__ cmds, size_t i, MenuItem &item, WRect rect, int xIndent) const override;

protected:
	std::vector<TextMenuItem> textItem;
	int activeItem = -1;
};

}
