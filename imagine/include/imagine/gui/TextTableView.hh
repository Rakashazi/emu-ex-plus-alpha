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
#include <utility>
#include <vector>

class TextTableView : public TableView
{
public:
	TextTableView(IG::utf16String name, ViewAttachParams attach, unsigned itemsHint):
		TableView{std::move(name), attach, textItem}
	{
		textItem.reserve(itemsHint);
	}

	TextTableView(ViewAttachParams attach, unsigned itemsHint);
	void appendItem(IG::utf16String name, TextMenuItem::SelectDelegate del);
	void setItem(size_t idx, IG::utf16String name, TextMenuItem::SelectDelegate del);
	TextMenuItem &item(size_t idx);
	void setItems(size_t items);
	void onAddedToController(ViewController *c, Input::Event e) override;
	void drawElement(Gfx::RendererCommands &cmds, size_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const override;

protected:
	std::vector<TextMenuItem> textItem{};
	int activeItem = -1;
};
