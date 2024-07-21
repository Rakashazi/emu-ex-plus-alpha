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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/base/Window.hh>
#include <imagine/input/Event.hh>
#include <imagine/util/math.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log{"View"};

void ViewI::prepareDraw() {}

bool ViewI::inputEvent(const Input::Event&, ViewInputEventParams) { return false; }

void ViewI::clearSelection() {}

void ViewI::onShow() {}

void ViewI::onHide() {}

void ViewI::onAddedToController(ViewController *, const Input::Event &) {}

void ViewI::setFocus(bool) {}

std::u16string_view ViewI::name() const { return u""; }

bool ViewI::onDocumentPicked(const DocumentPickerEvent&) { return false; }

Gfx::Renderer &ViewAttachParams::renderer() const
{
	return rendererTask.renderer();
}

ApplicationContext ViewAttachParams::appContext() const
{
	return window.appContext();
}

void ViewController::pushAndShow(std::unique_ptr<View> v, const Input::Event &e)
{
	pushAndShow(std::move(v), e, true, false);
}

void ViewController::pushAndShow(std::unique_ptr<View> v)
{
	auto e = v->appContext().defaultInputEvent();
	pushAndShow(std::move(v), e);
}

void ViewController::pop()
{
	dismissView(-1, false);
}

void ViewController::popAndShow()
{
	dismissView(-1);
}

void ViewController::popTo(View&)
{
	log.error("popTo() not implemented for this controller");
}

bool ViewController::inputEvent(const Input::Event &)
{
	return false;
}

bool ViewController::moveFocusToNextView(const Input::Event &, _2DOrigin)
{
	return false;
};

View* ViewController::parentView(View&) { return {}; }

std::optional<bool> ViewManager::needsBackControlOption() const
{
	if(!needsBackControlIsMutable || needsBackControl == needsBackControlDefault)
		return {};
	return needsBackControl;
}

void ViewManager::setTableXIndentMM(float indentMM, const Window &win)
{
	auto oldIndent = std::exchange(tableXIndentPx, win.widthMMInPixels(indentMM));
	if(tableXIndentPx != oldIndent)
	{
		//log.debug("setting X indent:{}mm ({} as pixels)", indentMM, tableXIndentPx);
	}
}

float ViewManager::defaultTableXIndentMM(const Window &win)
{
	auto wMM = win.sizeMM().x;
	return
		wMM < 150. ? 1. :
		wMM < 250. ? 2. :
		4.;
}

void ViewManager::setTableXIndentToDefault(const Window &win)
{
	setTableXIndentMM(defaultTableXIndentMM(win), win);
}

void View::pushAndShow(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView, bool isModal)
{
	assumeExpr(controller_);
	controller_->pushAndShow(std::move(v), e, needsNavView, isModal);
}

void View::pushAndShowModal(std::unique_ptr<View> v, const Input::Event &e, bool needsNavView)
{
	pushAndShow(std::move(v), e, needsNavView, true);
}

void View::popTo(View &v)
{
	assumeExpr(controller_);
	controller_->popTo(v);
}

void View::popTo() { popTo(*this); }

void View::dismiss(bool refreshLayout)
{
	if(controller_)
	{
		controller_->dismissView(*this, refreshLayout);
	}
	else
	{
		log.warn("called dismiss with no controller");
	}
}

void View::dismissPrevious()
{
	if(controller_)
	{
		controller_->dismissView(-2);
	}
	else
	{
		log.warn("called dismissPrevious with no controller");
	}
}

Gfx::GlyphTextureSet &View::defaultFace() { return manager().defaultFace; }

Gfx::GlyphTextureSet &View::defaultBoldFace() { return manager().defaultBoldFace; }

Gfx::Color View::menuTextColor(bool isSelected)
{
	return isSelected ? Gfx::Color{0.f, .8f, 1.f} : Gfx::Color(Gfx::ColorName::WHITE);
}

int View::navBarHeight(const Gfx::GlyphTextureSet &face)
{
	return makeEvenRoundedUp(int(face.nominalHeight() * 1.75f));
}

void View::setViewRect(WindowRect viewRect, WindowRect displayRect)
{
	this->viewRect_ = viewRect;
	this->displayRect_ = displayRect;
}

void View::setViewRect(WindowRect viewRect)
{
	setViewRect(viewRect, viewRect);
}

void View::postDraw()
{
	if(win) [[likely]]
		win->postDraw();
}

Window &View::window() const
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
	return {*manager_, *win, *rendererTask_};
}

Screen *View::screen() const
{
	return win ? win->screen() : nullptr;
}

ApplicationContext View::appContext() const
{
	return window().appContext();
}

bool View::onDocumentPicked(const DocumentPickerEvent& e)
{
	auto vPtr = parentView();
	return vPtr ? vPtr->onDocumentPicked(e) : false;
}

std::u16string View::nameString(const MenuItem &item)
{
	return item.text().string();
}

void View::show()
{
	prepareDraw();
	onShow();
	postDraw();
}

bool View::moveFocusToNextView(const Input::Event &e, _2DOrigin direction)
{
	if(!controller_)
		return false;
	return controller_->moveFocusToNextView(e, direction);
}

View* View::parentView()
{
	if(!controller_)
		return {};
	return controller_->parentView(*this);
}

void View::setWindow(Window *w)
{
	win = w;
}

void View::onDismiss() {}

void View::setController(ViewController *c, const Input::Event &e)
{
	controller_ = c;
	if(c)
	{
		onAddedToController(c, e);
	}
}

void View::setController(ViewController *c)
{
	setController(c, appContext().defaultInputEvent());
}

ViewController *View::controller() const
{
	return controller_;
}

WindowRect View::displayInsetRect(Direction d) const
{
	return displayInsetRect(d, viewRect(), displayRect());
}

WindowRect View::displayInsetRect(Direction d, WindowRect viewRect, WindowRect displayRect)
{
	switch(d)
	{
		case Direction::TOP: return {displayRect.pos(LT2DO), {displayRect.x2, viewRect.y}};
		case Direction::RIGHT: return {{viewRect.x2, displayRect.y}, displayRect.pos(RB2DO)};
		case Direction::BOTTOM: return {{displayRect.x, viewRect.y2}, displayRect.pos(RB2DO)};
		case Direction::LEFT: return {displayRect.pos(LT2DO), {viewRect.x, displayRect.y2}};
	}
	bug_unreachable("Direction == %d", (int)d);
}

bool View::pointIsInView(WPt pos)
{
	return viewRect().overlaps(pos);
}

}
