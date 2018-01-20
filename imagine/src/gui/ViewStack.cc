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

#include <imagine/gui/ViewStack.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/math/int.hh>
#include <imagine/util/ScopeGuard.hh>
#include <utility>

void BasicViewController::push(View &v, Input::Event e)
{
	if(view)
	{
		logMsg("removing existing view from basic view controller");
		pop();
	}
	v.setController(this, e);
	view = &v;
	logMsg("push view in basic view controller");
}

void BasicViewController::pushAndShow(View &v, Input::Event e, bool needsNavView)
{
	push(v, e);
	place();
	v.show();
	v.postDraw();
}

void BasicViewController::pushAndShow(View &v, Input::Event e)
{
	pushAndShow(v, e, true);
}

void BasicViewController::pop()
{
	assert(view);
	view->postDraw();
	delete view;
	view = nullptr;
	if(removeViewDel)
		removeViewDel();
}

void BasicViewController::dismissView(View &v)
{
	pop();
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
	view->setViewRect(viewRect, projP);
	view->place();
}

bool BasicViewController::inputEvent(Input::Event e)
{
	return view->inputEvent(e);
}

void BasicViewController::draw()
{
	view->draw();
}

void ViewStack::setNavView(std::unique_ptr<NavView> navView)
{
	nav = std::move(navView);
	if(nav)
	{
		nav->setController(this, Input::defaultEvent());
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

void ViewStack::draw()
{
	if(!view.size())
		return;
	top().draw();
	if(navViewIsActive())
		nav->draw();
}

void ViewStack::push(View &v, Input::Event e)
{
	v.setController(this, e);
	view.emplace_back(std::unique_ptr<View>(&v), true);
	logMsg("push view, %d in stack", (int)view.size());
	if(nav)
	{
		showNavLeftBtn();
		navViewHasFocus = false;
		nav->clearSelection();
	}
}

void ViewStack::pushAndShow(View &v, Input::Event e, bool needsNavView)
{
	push(v, e);
	view.back().needsNavView = needsNavView;
	place();
	v.show();
	v.postDraw();
}

void ViewStack::pushAndShow(View &v, Input::Event e)
{
	pushAndShow(v, e, true);
}

void ViewStack::pop()
{
	if(!view.size())
		return;
	onRemoveView_.callSafe(*this, top());
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
	while(view.size() > 1 && &top() != &v)
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

View &ViewStack::viewAtIdx(uint idx) const
{
	assumeExpr(idx < view.size());
	return *view[idx].v;
}

int ViewStack::viewIdx(View &v) const
{
	iterateTimes(view.size(), i)
	{
		if(view[i].v.get() == &v)
			return i;
	}
	return -1;
}

int ViewStack::viewIdx(const char *name) const
{
	iterateTimes(view.size(), i)
	{
		if(string_equal(view[i].v->name(), name))
			return i;
	}
	return -1;
}

bool ViewStack::contains(View &v) const
{
	return viewIdx(v) != -1;
}

bool ViewStack::contains(const char *name) const
{
	return viewIdx(name) != -1;
}

void ViewStack::dismissView(View &v)
{
	//logMsg("dismissing view: %p", &v);
	auto idx = viewIdx(v);
	if(idx > 0)
	{
		popTo(*view[idx-1].v);
	}
	else
	{
		popAndShow();
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

uint ViewStack::size() const
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

void ViewStack::setOnRemoveView(RemoveViewDelegate del)
{
	onRemoveView_ = del;
}

bool ViewStack::viewHasFocus() const
{
	return !navViewHasFocus;
}
