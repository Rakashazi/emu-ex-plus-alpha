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

#include <cstddef>
#include <vector>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/algorithm.h>

class TextTableView : public TableView
{
public:
	int activeItem = -1;

	TextTableView(ViewAttachParams attach, uint itemsHint);
	TextTableView(const char *name, ViewAttachParams attach, uint itemsHint);
	void appendItem(const char *name, TextMenuItem::SelectDelegate del);
	template<class C>
	void appendItem(const char *name, C &&del) { appendItem(name, TextMenuItem::wrapSelectDelegate(del)); }
	void setItem(uint idx, const char *name, TextMenuItem::SelectDelegate del);
	TextMenuItem &item(uint idx);
	void setItems(uint items);
	void onAddedToController(Input::Event e) override;
	void drawElement(Gfx::Renderer &r, uint i, MenuItem &item, Gfx::GCRect rect) const override;

protected:
	std::vector<TextMenuItem> textItem{};
};
