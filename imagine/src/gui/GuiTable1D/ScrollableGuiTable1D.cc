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

#define thisModuleName "ScrollableGuiTable1D"

#include "GuiTable1D.hh"
#include <gfx/GeomRect.hh>
#include <input/Input.hh>
#include <base/Base.hh>
#include <algorithm>

void ScrollableGuiTable1D::init(GuiTableSource *src, int cells, _2DOrigin align)
{
	onlyScrollIfNeeded = 0;
	GuiTable1D::init(src, cells, align);
	ScrollView1D::init(&viewRect);
}

void ScrollableGuiTable1D::deinit() {}

void ScrollableGuiTable1D::draw()
{
	using namespace Gfx;
	ScrollView1D::updateGfx();
	setClipRectBounds(ScrollView1D::viewFrame);
	setClipRect(1);
	ScrollView1D::draw();
	GuiTable1D::draw();
	setClipRect(0);
}

void ScrollableGuiTable1D::place(Rect2<int> *frame)
{
	assert(frame);
	setXCellSize(frame->xSize());
	ScrollView1D::place(frame);
	updateView();
}

void ScrollableGuiTable1D::setScrollableIfNeeded(bool yes)
{
	onlyScrollIfNeeded = 1;
}

void ScrollableGuiTable1D::scrollToFocusRect()
{
	Rect2<int> focus = focusRect();
	//logMsg("focus box %d,%d %d,%d, scroll %d", focus.x, focus.y, focus.x2, focus.y2, gfx_toIYSize(scroll.offset));
	if(focus.ySize() > 1 && !viewFrame.contains(focus))
	{
		int diff;
		if(focus.y < viewFrame.y)
			diff = focus.y - viewFrame.y;
		else
			diff = focus.y2 - viewFrame.y2;
		diff--;
		logMsg("focus not in view by %d", diff);

		scroll.setOffset(scroll.offset + /*gfx_iYSize*/(diff));
		ScrollView1D::updateView();
	}
}

void ScrollableGuiTable1D::updateView()
{
	ScrollView1D::updateView();
	scrollToFocusRect();
}

void ScrollableGuiTable1D::inputEvent(const Input::Event &e)
{
	bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
	if(handleScroll && ScrollView1D::inputEvent(e))
	{
		selected = -1;
		return;
	}
	if(e.isPointer() && !ScrollView1D::viewFrame.overlaps(e.x, e.y))
		return;
	if(GuiTable1D::inputEvent(e) && handleScroll && !e.isPointer())
	{
		scrollToFocusRect();
	}
}
