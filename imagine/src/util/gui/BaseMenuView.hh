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

#include <gui/GuiTable1D/GuiTable1D.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <util/gui/ViewAnimation.hh>

class BaseMenuView : public View, public GuiTableSource
{
public:
	constexpr BaseMenuView(Base::Window &win): View(win) {}
	constexpr BaseMenuView(const char *name, Base::Window &win) : View(name, win) {}

	MenuItem **item = nullptr;
	uint items = 0;
	IG::Rect2<int> viewFrame;
	ScrollableGuiTable1D tbl;
	//FadeViewAnimation<10> fade;

	IG::Rect2<int> &viewRect() { return viewFrame; }

	void init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align = LC2DO)
	{
		//logMsg("init menu with %d items", items);
		var_selfs(item);
		var_selfs(items);
		tbl.init(this, items, *this, align);
		if(highlightFirst && items)
			tbl.selected = 0;
		//View::init(&fade);
	}

	void deinit() override
	{
		//logMsg("deinit BaseMenuView");
		tbl.deinit();
		iterateTimes(tbl.cells, i)
		{
			item[i]->deinit();
		}
	}

	void place() override
	{
		iterateTimes(items, i)
		{
			//logMsg("compile item %d", i);
			item[i]->compile();
		}
		if(items)
		{
			tbl.setYCellSize(IG::makeEvenRoundedUp(item[0]->ySize()*2));
		}
		tbl.place(&viewFrame, *this);
	}

	void inputEvent(const Input::Event &e) override
	{
		tbl.inputEvent(e, *this);
	}

	void clearSelection()
	{
		tbl.clearSelection();
	}

	void draw(Gfx::FrameTimeBase frameTime) override
	{
		using namespace Gfx;
		//if(!updateAnimation())
		//	return;
		tbl.draw(*this);
	}

	void drawElement(const GuiTable1D *table, uint i, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override
	{
		using namespace Gfx;
		//gfx_setColor(1., 1., 1., fade.m.now);
		setColor(COLOR_WHITE);
		item[i]->draw(xPos, yPos, xSize, ySize, align);
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i) override
	{
		item[i]->select(this, e);
	}
};
