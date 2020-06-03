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
#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Window.hh>
#include <imagine/logger/logger.h>

Gfx::GlyphTextureSet View::defaultFace{};
Gfx::GlyphTextureSet View::defaultBoldFace{};
bool View::needsBackControl = needsBackControlDefault;

Gfx::Renderer &ViewAttachParams::renderer() const
{
	return rTask.renderer();
}

void ViewController::pushAndShow(std::unique_ptr<View> v, Input::Event e)
{
	pushAndShow(std::move(v), e, true, false);
}

void ViewController::popAndShow()
{
	pop();
};

void ViewController::popTo(View &v)
{
	pop();
}

bool ViewController::moveFocusToNextView(Input::Event, _2DOrigin)
{
	return false;
};

View::~View() {}

void View::pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal)
{
	assumeExpr(controller_);
	controller_->pushAndShow(std::move(v), e, needsNavView, isModal);
}

void View::pushAndShowModal(std::unique_ptr<View> v, Input::Event e, bool needsNavView)
{
	pushAndShow(std::move(v), e, needsNavView, true);
}

void View::pop()
{
	assumeExpr(controller_);
	controller_->pop();
}

void View::popAndShow()
{
	assumeExpr(controller_);
	controller_->popAndShow();
}

void View::popTo(View &v)
{
	assumeExpr(controller_);
	controller_->popTo(v);
}

void View::dismiss()
{
	if(controller_)
	{
		controller_->dismissView(*this);
	}
	else
	{
		logWarn("called dismiss with no controller");
	}
}

bool View::compileGfxPrograms(Gfx::Renderer &r)
{
	r.makeCommonTextureSampler(Gfx::CommonTextureSampler::NEAREST_MIP_CLAMP);
	auto compiled = r.noTexProgram.compile(r);
	// for text
	compiled |= r.texAlphaProgram.compile(r);
	compiled |= r.texAlphaReplaceProgram.compile(r);
	return compiled;
}

void View::clearSelection() {}

void View::onShow() {}

void View::onHide() {}

void View::onAddedToController(Input::Event e) {}

void View::prepareDraw() {}

void View::setFocus(bool) {}

void View::setViewRect(IG::WindowRect rect, Gfx::ProjectionPlane projP)
{
	this->viewRect_ = rect;
	this->projP = projP;
}

void View::postDraw()
{
	if(likely(win))
		win->postDraw();
}

Base::Window &View::window() const
{
	assumeExpr(win);
	return *win;
}

Gfx::Renderer &View::renderer() const
{
	assumeExpr(rendererTask_);
	return rendererTask_->renderer();
}

Gfx::RendererTask &View::rendererTask() const
{
	assumeExpr(rendererTask_);
	return *rendererTask_;
}

ViewAttachParams View::attachParams() const
{
	return {*win, *rendererTask_};
}

Base::Screen *View::screen() const
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

bool View::moveFocusToNextView(Input::Event e, _2DOrigin direction)
{
	if(!controller_)
		return false;
	return controller_->moveFocusToNextView(e, direction);
}

void View::setController(ViewController *c, Input::Event e)
{
	controller_ = c;
	if(c)
	{
		onAddedToController(e);
	}
}

ViewController *View::controller() const
{
	return controller_;
}

IG::WindowRect View::viewRect() const
{
	return viewRect_;
}

Gfx::ProjectionPlane View::projection() const
{
	return projP;
}

bool View::pointIsInView(IG::WP pos)
{
	return viewRect().overlaps(pos);
}

void View::waitForDrawFinished()
{
	// currently a no-op due to RendererTask only running present() async
	/*assumeExpr(rendererTask_);
	rendererTask_->waitForDrawFinished();*/
}
