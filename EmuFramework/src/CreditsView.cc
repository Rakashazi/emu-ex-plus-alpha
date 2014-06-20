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

#define LOGTAG "CreditsView"
#include <CreditsView.hh>
#include <meta.h>

CreditsView::CreditsView(const char *str, Base::Window &win):
	View(CONFIG_APP_NAME " " IMAGINE_VERSION, win), str(str) {}

void CreditsView::draw(Base::FrameTimeBase frameTime)
{
	using namespace Gfx;
	float fadeVal;
	if(fade.update(1, fadeVal))
	{
		postDraw();
	}
	setColor(1., 1., 1., fadeVal);
	texAlphaProgram.use(projP.makeTranslate());
	auto textRect = rect;
	if(IG::isOdd(textRect.ySize()))
		textRect.y2--;
	text.draw(projP.unProjectRect(textRect).pos(C2DO), C2DO, projP);
}

void CreditsView::place()
{
	text.compile(projP);
}

void CreditsView::inputEvent(const Input::Event &e)
{
	if((e.isPointer() && rect.overlaps({e.x, e.y}) && e.state == Input::RELEASED)
			|| (!e.isPointer() && e.state == Input::PUSHED))
	{
		dismiss();
	}
}

void CreditsView::init()
{
	text.init(str, View::defaultFace);
	fade.set(0., 1., INTERPOLATOR_TYPE_LINEAR, 20);
	place();
	View::init();
}

void CreditsView::deinit()
{
	text.deinit();
}
