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
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/engine-globals.h>
#include <imagine/gui/GuiTable1D.hh>
#include <imagine/gui/MenuItem.hh>

class BaseMenuView : public View, public GuiTableSource
{
public:
	MenuItem **item = nullptr;
	uint items = 0;
	IG::WindowRect viewFrame;
	ScrollableGuiTable1D tbl;
	_2DOrigin align;

	BaseMenuView(Base::Window &win): View(win) {}
	BaseMenuView(const char *name, Base::Window &win) : View(name, win) {}
	IG::WindowRect &viewRect() { return viewFrame; }
	void init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align = LC2DO);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void clearSelection();
	void draw(Base::FrameTimeBase frameTime) override;
	void drawElement(const GuiTable1D &table, uint i, Gfx::GCRect rect) const override;
	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i) override;
};
