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

#include <gui/BaseMenuView.hh>

void BaseMenuView::init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align)
{
	//logMsg("init menu with %d items", items);
	var_selfs(item);
	var_selfs(items);
	var_selfs(align);
	tbl.init(this, items, *this);
	if(highlightFirst && items)
		tbl.selected = 0;
}

void BaseMenuView::deinit()
{
	//logMsg("deinit BaseMenuView");
	tbl.deinit();
	iterateTimes(tbl.cells, i)
	{
		item[i]->deinit();
	}
}

void BaseMenuView::place()
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

void BaseMenuView::inputEvent(const Input::Event &e)
{
	tbl.inputEvent(e, *this);
}

void BaseMenuView::clearSelection()
{
	tbl.clearSelection();
}

void BaseMenuView::draw(Base::FrameTimeBase frameTime)
{
	tbl.draw(*this);
}

void BaseMenuView::drawElement(const GuiTable1D &table, uint i, Gfx::GCRect rect) const
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	item[i]->draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), align);
}

void BaseMenuView::onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
{
	item[i]->select(this, e);
}
