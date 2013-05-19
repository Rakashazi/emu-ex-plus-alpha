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
#include <utility>

struct StackAllocator
{
	constexpr StackAllocator() {}
	uint itemEndOffset[5] {0};
	uint items = 0;
	uint8 storage[1024*15] __attribute__((aligned)) {0};

	void *alloc(uint size)
	{
		if(items >= sizeofArray(itemEndOffset))
		{
			bug_exit("too many items");
		}

		auto itemStart = items ? itemEndOffset[items-1] : 0;
		auto itemEnd = itemStart + size;
		auto itemEndAligned = IG::alignRoundedUp(itemEnd, 16);
		itemEndOffset[items] = itemEndAligned;
		items++;
		logMsg("allocated %u bytes @ offset 0x%X (%d)", size, itemStart, itemStart);
		if(itemEnd != itemEndAligned)
		{
			logMsg("end offset 0x%X (%u) aligned to 0x%X (%u)", itemEnd, itemEnd, itemEndAligned, itemEndAligned);
		}

		if(itemEndOffset[items-1] > sizeof(storage))
		{
			bug_exit("out of space");
		}

		return &storage[itemStart];
	}

	template<typename T, typename... ARGS>
	T *allocNew(ARGS&&... args)
	{
		auto allocated = alloc(sizeof(T));
		auto obj = new(allocated) T(std::forward<ARGS>(args)...);
		return obj;
	}

	void pop()
	{
		assert(items);
		items--;
	}
};

class ViewStack
{
	View *view[5] {nullptr};
	StackAllocator *viewAllocator[5] {nullptr};
	NavView *nav = nullptr;
	Rect2<int> viewRect, customViewRect;
public:
	uint size = 0;
	bool useNavView = 0;
	constexpr ViewStack() { }
	constexpr ViewStack(View &root): view{&root}, size{1} { }

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

	void push(View *v, StackAllocator *allocator)
	{
		assert(size != sizeofArray(view));
		view[size] = v;
		viewAllocator[size] = allocator;
		size++;
		logMsg("push view, %d in stack", size);

		if(nav)
		{
			nav->setLeftBtnActive(size > 1);
		}
	}

	void push(View *v)
	{
		push(v, nullptr);
	}

	void pushAndShow(View *v, StackAllocator *allocator)
	{
		push(v, allocator);
		place();
		v->show();
		Base::displayNeedsUpdate();
	}

	void pushAndShow(View *v)
	{
		pushAndShow(v, nullptr);
	}

	void pop()
	{
		top()->deinit();
		if(viewAllocator[size-1])
		{
			viewAllocator[size-1]->pop();
		}
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
