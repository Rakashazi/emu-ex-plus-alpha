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
	constexpr BaseMenuView() { }
	constexpr BaseMenuView(const char *name) : View(name) { }

	MenuItem **item = nullptr;
	uint items = 0;
	Rect2<int> viewFrame;
	ScrollableGuiTable1D tbl;
	//FadeViewAnimation<10> fade;

	Rect2<int> &viewRect() { return viewFrame; }

	void init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align = LC2DO)
	{
		//logMsg("init menu with %d items", items);
		var_selfs(item);
		var_selfs(items);
		tbl.init(this, items, align);
		if(highlightFirst && items)
			tbl.selected = 0;
		//View::init(&fade);
	}

	void deinit()
	{
		//logMsg("deinit BaseMenuView");
		tbl.deinit();
		iterateTimes(tbl.cells, i)
		{
			item[i]->deinit();
		}
	}

	void place()
	{
		iterateTimes(items, i)
		{
			//logMsg("compile item %d", i);
			item[i]->compile();
		}

		tbl.setYCellSize(item[0]->ySize()*2);
		tbl.place(&viewFrame);
	}

	void inputEvent(const Input::Event &e)
	{
		tbl.inputEvent(e);
	}

	void clearSelection()
	{
		tbl.clearSelection();
	}

	void draw(Gfx::FrameTimeBase frameTime)
	{
		using namespace Gfx;
		//if(!updateAnimation())
		//	return;
		tbl.draw();
	}

	void drawElement(const GuiTable1D *table, uint i, Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		//gfx_setColor(1., 1., 1., fade.m.now);
		setColor(1., 1., 1., 1.);
		item[i]->draw(xPos, yPos, xSize, ySize, align);
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		item[i]->select(this, e);
	}
};
