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

#define LOGTAG "AlertView"

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
	int xSize = rect.xSize() * .8;
	text.maxLineSize = projP.unprojectXSize(xSize) * 0.95;
	text.compile();

	int menuYSize = menu.items * menu.item[0]->ySize()*2;
	int labelYSize = IG::makeEvenRoundedUp(projP.projectYSize(text.ySize + (text.nominalHeight * .5)));
	IG::WindowRect viewFrame;
	viewFrame.setPosRel({rect.xSize()/2, rect.ySize()/2},
			{xSize, labelYSize + menuYSize}, C2DO);

	labelFrame = projP.unProjectRect(viewFrame.x, viewFrame.y, viewFrame.x2, viewFrame.y + labelYSize);

	IG::WindowRect menuViewFrame;
	menuViewFrame.setPosRel({viewFrame.x, viewFrame.y + (int)labelYSize},
			{viewFrame.xSize(), menuYSize}, LT2DO);
	menu.placeRect(menuViewFrame);
}

void AlertView::inputEvent(const Input::Event &e)
{
	if(e.state == Input::PUSHED)
	{
		if(e.isDefaultCancelButton())
		{
			dismiss();
			return;
		}
	}
	menu.inputEvent(e);
}

void AlertView::draw(Base::FrameTimeBase frameTime)
{
	using namespace Gfx;
	setBlendMode(BLEND_MODE_ALPHA);
	noTexProgram.use(View::projP.makeTranslate());
	setColor(.4, .4, .4, .8);
	GeomRect::draw(labelFrame);
	setColor(.1, .1, .1, .6);
	GeomRect::draw(menu.viewRect(), projP);
	setColor(COLOR_WHITE);
	texAlphaReplaceProgram.use();
	text.draw(labelFrame.xPos(C2DO), projP.alignYToPixel(labelFrame.yPos(C2DO)), C2DO);
	//setClipRect(1);
	//setClipRectBounds(menu.viewRect());
	menu.draw(frameTime);
	//setClipRect(0);
}


YesNoAlertView::YesNoAlertView(Base::Window &win):
	AlertView(win),
	yes
	{
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto callback = onYesD;
			dismiss();
			if(callback) callback(e);
		}
	},
	no
	{
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto callback = onNoD;
			dismiss();
			if(callback) callback(e);
		}
	}
{}

void YesNoAlertView::init(const char *label, bool highlightFirst, const char *choice1, const char *choice2)
{
	yes.init(choice1 ? choice1 : "Yes"); menuItem[0] = &yes;
	no.init(choice2 ? choice2 : "No"); menuItem[1] = &no;
	assert(!onYesD);
	assert(!onNoD);
	AlertView::init(label, menuItem, highlightFirst);
}

void YesNoAlertView::deinit()
{
	logMsg("deinit alert");
	AlertView::deinit();
	onYesD = {};
	onNoD = {};
}
