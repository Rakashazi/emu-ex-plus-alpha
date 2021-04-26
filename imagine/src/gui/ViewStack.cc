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

#define LOGTAG "ViewStack"
#include <imagine/gui/ViewStack.hh>
#include <imagine/gui/NavView.hh>
#include <imagine/base/Window.hh>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/string.h>
#include <utility>

BasicViewController::BasicViewController() {}

void BasicViewController::push(std::unique_ptr<View> v, Input::Event e)
{
	if(view)
	{
		logMsg("removing existing view from basic view controller");
		pop();
	}
	assumeExpr(v);
	v->setController(this, e);
	view = std::move(v);
	logMsg("push view in basic view controller");
}

void BasicViewController::pushAndShow(std::unique_ptr<View> v, Input::Event e, bool, bool)
{
	push(std::move(v), e);
	place();
	view->show();
	view->postDraw();
}

void BasicViewController::dismissView(View &v, bool)
{
	if(&v != view.get())
		return;
	dismissView(0, false);
}

void BasicViewController::dismissView(int idx, bool)
{
	if(!view || idx != 0 || idx != -1)
		return;
	auto &win = view->window();
	view->waitForDrawFinished();
	view.reset();
	if(removeViewDel)
		removeViewDel();
	win.postDraw();
}

void BasicViewController::place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP)
{
	viewRect = rect;
	this->projP = projP;
	place();
}

void BasicViewController::place()
{
	if(!view)
		return;
	assert(viewRect.xSize() && viewRect.ySize());
	view->waitForDrawFinished();
	view->setViewRect(viewRect, projP);
	view->place();
}

bool BasicViewController::inputEvent(Input::Event e)
{
	return view->inputEvent(e);
}

void BasicViewController::draw(Gfx::RendererCommands &cmds)
{
	if(!view)
		return;
	view->draw(cmds);
}

ViewStack::ViewStack() {}

void ViewStack::setNavView(std::unique_ptr<NavView> navView)
{
	if(view.size())
		top().waitForDrawFinished();
	nav = std::move(navView);
	if(nav)
	{
		nav->setController(this);
		showNavLeftBtn();
	}
}

NavView *ViewStack::navView() const
{
	return nav.get();
}

void ViewStack::place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP)
{
	viewRect = rect;
	this->projP = projP;
	place();
}

void ViewStack::place()
{
	if(!view.size())
		return;
	top().waitForDrawFinished();
	assert(viewRect.xSize() && viewRect.ySize());
	customViewRect = viewRect;
	if(navViewIsActive())
	{
		nav->setTitle(top().name());
		auto navRect = IG::makeWindowRectRel(viewRect.pos(LT2DO), {viewRect.xSize(), IG::makeEvenRoundedUp(int(nav->titleFace()->nominalHeight()*(double)1.75))});
		nav->setViewRect(navRect, projP);
		nav->place();
		customViewRect.y += nav->viewRect().ySize();
	}
	else
	{
		navViewHasFocus = false;
	}
	top().setViewRect(customViewRect, projP);
	top().place();
}

bool ViewStack::inputEvent(Input::Event e)
{
	if(!view.size())
		return false;
	if(e.isPointer())
	{
		if(navViewIsActive() && nav->viewRect().overlaps(e.pos()))
		{
			if(nav->inputEvent(e))
			{
				return true;
			}
		}
		if(e.pushed())
		{
			navViewHasFocus = false;
			nav->clearSelection();
		}
		return top().inputEvent(e);
	}
	else
	{
		if(navViewHasFocus)
			return nav->inputEvent(e);
		else
			return top().inputEvent(e);
	}
}

bool ViewStack::moveFocusToNextView(Input::Event e, _2DOrigin direction)
{
	if(changingViewFocus || !view.size() || !navViewIsActive())
		return false;
	if(!direction.onTop() && !direction.onBottom())
		return false;
	changingViewFocus = true;
	auto endChangingViewFocus = IG::scopeGuard([this]() { changingViewFocus = false; });
	if(navViewHasFocus)
	{
		if(top().inputEvent(e))
		{
			navViewHasFocus = false;
			return true;
		}
		return false;
	}
	else
	{
		if(!nav->hasButtons())
			return false;
		if(nav->inputEvent(e))
		{
			top().setFocus(false);
			navViewHasFocus = true;
			return true;
		}
		return false;
	}
}

void ViewStack::prepareDraw()
{
	if(!view.size())
		return;
	top().prepareDraw();
	if(navViewIsActive())
		nav->prepareDraw();
}

void ViewStack::draw(Gfx::RendererCommands &cmds)
{
	if(!view.size())
		return;
	top().draw(cmds);
	if(navViewIsActive())
		nav->draw(cmds);
}

void ViewStack::push(std::unique_ptr<View> v, Input::Event e)
{
	assumeExpr(v);
	if(view.size())
	{
		top().waitForDrawFinished();
		top().onHide();
	}
	v->setController(this, e);
	view.emplace_back(std::move(v), true);
	logMsg("push view, %d in stack", (int)view.size());
	if(nav)
	{
		showNavLeftBtn();
		navViewHasFocus = false;
		nav->clearSelection();
	}
}

void ViewStack::push(std::unique_ptr<View> v)
{
	auto e = v->appContext().defaultInputEvent();
	push(std::move(v), e);
}

void ViewStack::pushAndShow(std::unique_ptr<View> v, Input::Event e, bool needsNavView, bool isModal)
{
	push(std::move(v), e);
	view.back().needsNavView = needsNavView;
	view.back().isModal = isModal;
	place();
	top().show();
	top().postDraw();
}

void ViewStack::pop()
{
	if(!view.size())
		return;
	top().waitForDrawFinished();
	view.back().v->onDismiss();
	view.pop_back();
	logMsg("pop view, %d in stack", (int)view.size());
	if(nav)
	{
		showNavLeftBtn();
		if(view.size())
		{
			nav->setTitle(top().name());
			if(navViewHasFocus)
				top().setFocus(false);
		}
	}
}

void ViewStack::popViews(int num)
{
	auto win = view.size() ? &top().window() : nullptr;
	iterateTimes(num, i)
	{
		pop();
	}
	if(win)
		win->postDraw();
	if(!view.size())
		return;
	place();
	top().show();
}

void ViewStack::popAndShow()
{
	popViews(1);
}

void ViewStack::popToRoot()
{
	if(view.size() > 1)
		popViews(view.size() - 1);
}

void ViewStack::popAll()
{
	popViews(view.size());
}

void ViewStack::popTo(View &v)
{
	popTo(viewIdx(v));
}

void ViewStack::popTo(int idx)
{
	if(idx < 0)
		return;
	unsigned popToSize = idx + 1;
	while(view.size() > popToSize)
		pop();
	place();
	top().show();
	top().postDraw();
}

void ViewStack::show()
{
	if(view.size())
		top().show();
}

View &ViewStack::top() const
{
	assumeExpr(view.size());
	return *view.back().v;
}

View &ViewStack::viewAtIdx(uint32_t idx) const
{
	assumeExpr(idx < view.size());
	return *view[idx].v;
}

int ViewStack::viewIdx(View &v) const
{
	for(int i = 0; auto &viewEntry : view)
	{
		if(viewEntry.v.get() == &v)
			return i;
		i++;
	}
	return -1;
}

int ViewStack::viewIdx(View::NameStringView name) const
{
	for(int i = 0; auto &viewEntry : view)
	{
		if(viewEntry.v->name() == name)
			return i;
		i++;
	}
	return -1;
}

int ViewStack::viewIdx(const char *name) const
{
	return viewIdx(View::makeNameString(name));
}

bool ViewStack::contains(View &v) const
{
	return viewIdx(v) != -1;
}

bool ViewStack::contains(View::NameStringView name) const
{
	return viewIdx(name) != -1;
}

bool ViewStack::contains(const char *name) const
{
	return contains(View::makeNameString(name));
}

void ViewStack::dismissView(View &v, bool refreshLayout)
{
	auto idx = viewIdx(v);
	if(idx < 0)
	{
		logWarn("view:%p not found to dismiss", &v);
		return;
	}
	return dismissView(idx, refreshLayout);
}

void ViewStack::dismissView(int idx, bool refreshLayout)
{
	if(idx < 0)
	{
		// negative index is treated as an offset from the current size
		idx = size() + idx;
	}
	if(idx == 0)
	{
		logWarn("not dismissing root view");
		return;
	}
	if(idx < 0 || (uint32_t)idx >= size())
	{
		logWarn("view dismiss index out of range:%d", idx);
		return;
	}
	if((uint32_t)idx == size() - 1)
	{
		// topmost view case
		if(refreshLayout)
			popAndShow();
		else
			pop();
	}
	else
	{
		logMsg("dismissing view at index:%d", idx);
		view[idx].v->onDismiss();
		view.erase(view.begin() + idx);
	}
}

void ViewStack::showNavView(bool show)
{
	showNavView_ = show;
}

void ViewStack::setShowNavViewBackButton(bool show)
{
	showNavBackBtn = show;
	if(nav)
		showNavLeftBtn();
}

void ViewStack::showNavLeftBtn()
{
	nav->showLeftBtn(showNavBackBtn && view.size() > 1);
	if(navViewHasFocus && !nav->hasButtons())
	{
		navViewHasFocus = false;
		if(view.size())
		{
			top().setFocus(true);
		}
	}
}

uint32_t ViewStack::size() const
{
	return view.size();
}

bool ViewStack::topNeedsNavView() const
{
	if(!view.size())
		return false;
	return view.back().needsNavView;
}

bool ViewStack::navViewIsActive() const
{
	return nav && showNavView_ && topNeedsNavView();
}

bool ViewStack::viewHasFocus() const
{
	return !navViewHasFocus;
}

bool ViewStack::hasModalView() const
{
	if(!view.size())
		return false;
	return view.back().isModal;
}

void ViewStack::popModalViews()
{
	while(view.size() > 1 && view.back().isModal)
		pop();
	place();
	top().show();
	top().postDraw();
}
