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

#define LOGTAG "ScrollGuiTable1D"

#include <imagine/gui/GuiTable1D.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/input/Input.hh>
#include <imagine/base/Base.hh>
#include <algorithm>

void ScrollableGuiTable1D::init(GuiTableSource *src, int cells, View &view)
{
	onlyScrollIfNeeded = 0;
	GuiTable1D::init(src, cells);
	ScrollView1D::init(&viewRect);
}

void ScrollableGuiTable1D::deinit() {}

void ScrollableGuiTable1D::draw(View &view)
{
	using namespace Gfx;
	ScrollView1D::updateGfx(view);
	// TODO
	//setClipRectBounds(view.window(), ScrollView1D::viewFrame);
	//setClipRect(1);
	ScrollView1D::draw();
	GuiTable1D::draw();
	//setClipRect(0);
}

void ScrollableGuiTable1D::place(IG::WindowRect *frame, View &view)
{
	assert(frame);
	setXCellSize(frame->xSize());
	ScrollView1D::place(frame, view);
	updateView();
}

void ScrollableGuiTable1D::setScrollableIfNeeded(bool yes)
{
	onlyScrollIfNeeded = 1;
}

void ScrollableGuiTable1D::scrollToFocusRect()
{
	IG::WindowRect focus = focusRect();
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

void ScrollableGuiTable1D::inputEvent(const Input::Event &e, View &view)
{
	bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
	if(handleScroll && ScrollView1D::inputEvent(e, view))
	{
		selected = -1;
		return;
	}
	if(e.isPointer() && !ScrollView1D::viewFrame.overlaps({e.x, e.y}))
		return;
	if(GuiTable1D::inputEvent(e, view) && handleScroll && !e.isPointer())
	{
		scrollToFocusRect();
	}
}
