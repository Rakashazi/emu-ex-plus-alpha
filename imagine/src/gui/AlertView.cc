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

#define thisModuleName "AlertView"

#include <gui/AlertView.hh>

void AlertView::init(const char *label, MenuItem **menuItem, bool highlightFirst)
{
	text.init(label, View::defaultFace);
	menu.init(menuItem, 2, highlightFirst, C2DO);
	menu.tbl.onlyScrollIfNeeded = 1;
}

void AlertView::deinit()
{
	//logMsg("deinit AlertView");
	text.deinit();
	menu.deinit();
}

void AlertView::place()
{
	uint xSize = rect.xSize() * .8;
	text.maxLineSize = Gfx::iXSize(xSize) * 0.95;
	text.compile();

	uint menuYSize = menu.items * menu.item[0]->ySize()*2;
	uint labelYSize = Gfx::toIYSize(text.ySize + (text.nominalHeight * .5));
	Rect2<int> viewFrame;
	viewFrame.setPosRel(rect.xSize()/2, rect.ySize()/2,
			xSize, labelYSize + menuYSize, C2DO);

	labelFrame = Gfx::unProjectRect(viewFrame.x, viewFrame.y, viewFrame.x2, viewFrame.y + labelYSize);

	Rect2<int> menuViewFrame;
	menuViewFrame.setPosRel(viewFrame.x, viewFrame.y + labelYSize,
			viewFrame.xSize(), menuYSize, LT2DO);
	menu.placeRect(menuViewFrame);
}

void AlertView::inputEvent(const Input::Event &e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			removeModalView();
			return;
		}
	}
	menu.inputEvent(e);
}

void AlertView::draw(Gfx::FrameTimeBase frameTime)
{
	using namespace Gfx;
	setBlendMode(BLEND_MODE_ALPHA);
	resetTransforms();
	setColor(.4, .4, .4, .8);
	GeomRect::draw(labelFrame);
	setColor(.1, .1, .1, .6);
	GeomRect::draw(menu.viewRect());

	setColor(COLOR_WHITE);
	text.draw(labelFrame.xPos(C2DO), labelFrame.yPos(C2DO), C2DO, C2DO);
	//setClipRect(1);
	//setClipRectBounds(menu.viewRect());
	menu.draw(frameTime);
	//setClipRect(0);
}
