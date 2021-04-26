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
	TextTableView(ViewAttachParams attach, uint32_t itemsHint);
	TextTableView(NameString name, ViewAttachParams attach, uint32_t itemsHint);
	TextTableView(const char *name, ViewAttachParams attach, uint32_t itemsHint);
	void appendItem(const char *name, TextMenuItem::SelectDelegate del);
	void setItem(uint32_t idx, const char *name, TextMenuItem::SelectDelegate del);
	TextMenuItem &item(uint32_t idx);
	void setItems(uint32_t items);
	void onAddedToController(ViewController *c, Input::Event e) override;
	void drawElement(Gfx::RendererCommands &cmds, uint32_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const override;

	template<class Func>
	void appendItem(const char *name, Func &&func)
	{
		appendItem(name, TextMenuItem::makeSelectDelegate(std::forward<Func>(func)));
	}

protected:
	std::vector<TextMenuItem> textItem{};
	int activeItem = -1;
};
