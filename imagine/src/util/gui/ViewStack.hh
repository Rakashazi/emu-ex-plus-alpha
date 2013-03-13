#pragma once

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

#include <gui/View.hh>
#include <gui/NavView.hh>

class ViewStack
{
	View *view[5] = {nullptr};
	NavView *nav = nullptr;
	Rect2<int> viewRect, customViewRect;
public:
	uint size = 0;
	bool useNavView = 0;
	constexpr ViewStack() { }

	void init()
	{
		viewRect = Gfx::viewportRect();
	}

	void setNavView(NavView *nav)
	{
		var_selfs(nav);
		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
			useNavView = 1;
		}
	}

	NavView *navView() const
	{
		return nav;
	}

	void place(const Rect2<int> &rect)
	{
		viewRect = rect;
		place();
	}

	void place()
	{
		if(!size)
			return;
		customViewRect = viewRect;
		if(useNavView && nav)
		{
			nav->setTitle(top()->name());
			nav->viewRect.setPosRel(viewRect.x, viewRect.y, viewRect.xSize(), nav->text.face->nominalHeight()*1.75, LT2DO);
			nav->place();
			customViewRect.y += nav->viewRect.ySize();
		}
		top()->placeRect(customViewRect);
	}

	void inputEvent(const Input::Event &e)
	{
		if(useNavView && nav && e.isPointer() && nav->viewRect.overlaps(e.x, e.y))
		{
			nav->inputEvent(e);
		}
		top()->inputEvent(e);
	}

	void draw(Gfx::FrameTimeBase frameTime)
	{
		top()->draw(frameTime);
		if(useNavView && nav) nav->draw();
	}

	void push(View *v)
	{
		assert(size != sizeofArray(view));
		view[size] = v;
		size++;
		logMsg("push view, %d in stack", size);

		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
		}
	}

	void pushAndShow(View *v)
	{
		push(v);
		place();
		v->show();
		Base::displayNeedsUpdate();
	}

	void pop()
	{
		top()->deinit();
		size--;
		logMsg("pop view, %d in stack", size);
		assert(size != 0);

		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
			nav->setTitle(top()->name());
			useNavView = 1;
		}
	}

	void popAndShow()
	{
		pop();
		place();
		top()->show();
		Base::displayNeedsUpdate();
	}

	void popToRoot()
	{
		while(size > 1)
			pop();
		place();
		top()->show();
		Base::displayNeedsUpdate();
	}

	void popTo(View *v)
	{
		while(size > 1 && view[size-1] != v)
			pop();
		place();
		top()->show();
		Base::displayNeedsUpdate();
	}

	void show()
	{
		top()->show();
	}

	View *top() const
	{
		assert(size != 0);
		return view[size-1];
	}

	bool contains(View *v)
	{
		iterateTimes(size, i)
		{
			if(view[i] == v)
				return 1;
		}
		return 0;
	}
};
