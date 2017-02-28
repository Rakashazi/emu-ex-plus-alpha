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

#include <imagine/gui/View.hh>
#include <imagine/logger/logger.h>

Gfx::GlyphTextureSet View::defaultFace{};
Gfx::GlyphTextureSet View::defaultBoldFace{};
bool View::needsBackControl = needsBackControlDefault;

void View::pushAndShow(View &v, Input::Event e, bool needsNavView)
{
	assert(controller);
	controller->pushAndShow(v, e, needsNavView);
}

void View::pop()
{
	assert(controller);
	controller->pop();
}

void View::popAndShow()
{
	assert(controller);
	controller->popAndShow();
}

void View::dismiss()
{
	if(controller)
	{
		controller->dismissView(*this);
	}
	else
	{
		logWarn("called dismiss with no controller");
	}
}

bool View::compileGfxPrograms(Gfx::Renderer &r)
{
	Gfx::TextureSampler::initDefaultNearestMipClampSampler(r);
	auto compiled = r.noTexProgram.compile(r);
	// for text
	compiled |= r.texAlphaProgram.compile(r);
	compiled |= r.texAlphaReplaceProgram.compile(r);
	return compiled;
}

void View::setViewRect(IG::WindowRect rect, Gfx::ProjectionPlane projP)
{
	this->viewRect() = rect;
	this->projP = projP;
}

void View::postDraw()
{
	if(likely(win))
		win->postDraw();
}

Base::Window &View::window()
{
	assert(win);
	return *win;
}

Gfx::Renderer &View::renderer()
{
	assert(renderer_);
	return *renderer_;
}

ViewAttachParams View::attachParams()
{
	return {*win, *renderer_};
}

Base::Screen *View::screen()
{
	return win ? win->screen() : nullptr;
}

void View::setNeedsBackControl(bool on)
{
	if(!needsBackControlIsConst) // only modify on environments that make sense
	{
		needsBackControl = on;
	}
}

void View::show()
{
	onShow();
	//logMsg("showed view");
	postDraw();
}

void View::setController(ViewController *c, Input::Event e)
{
	controller = c;
	if(c)
	{
		onAddedToController(e);
	}
}

bool View::pointIsInView(IG::WP pos)
{
	return viewRect().overlaps(pos);
}
