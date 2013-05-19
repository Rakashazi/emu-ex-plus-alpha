/*  This file is part of EmuFramework.

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

#define thisModuleName "creditsView"
#include <CreditsView.hh>
#include <util/gui/ViewStack.hh>
extern ViewStack viewStack;

void CreditsView::draw(Gfx::FrameTimeBase frameTime)
{
	using namespace Gfx;
	if(!updateAnimation())
		return;
	setColor(1., 1., 1., fade.m.now);
	//gfx_setColor(GCOLOR_WHITE);
	text.draw(/*(1.-fade.m.now)/8. * (displayState == DISMISS ? -1 : 1)*/
			gXPos(rect, C2DO), gYPos(rect, C2DO), C2DO, C2DO);
}

void CreditsView::place()
{
	text.compile();
}

void CreditsView::inputEvent(const Input::Event &e)
{
	if((e.isPointer() && rect.overlaps(e.x, e.y) && e.state == Input::RELEASED)
			|| (!e.isPointer() && e.state == Input::PUSHED))
	{
		viewStack.popAndShow();
	}
}

void CreditsView::init()
{
	text.init(str, View::defaultFace);
	place();
	View::init(&fade, 1);
}

void CreditsView::deinit()
{
	text.deinit();
}
