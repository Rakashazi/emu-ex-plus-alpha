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

class StackAllocator
{
public:
	constexpr StackAllocator() {}
	uint itemEndOffset[5] {0};
	uint items = 0;
	uint8 storage[1024*74] __attribute__((aligned)) {0};

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

class BasicViewController : public ViewController
{
	View *view {nullptr};
	IG::WindowRect viewRect;
	typedef DelegateFunc<void ()> RemoveViewDelegate;
	RemoveViewDelegate removeViewDel;

public:
	constexpr BasicViewController() {}
	RemoveViewDelegate &onRemoveView() { return removeViewDel; }
	void push(View &v);
	void pushAndShow(View &v, StackAllocator *allocator, bool needsNavView) override;
	void pushAndShow(View &v);
	void pop();
	void dismissView(View &v) override;
	void place(const IG::WindowRect &rect);
	void place();
	bool hasView() { return view; }
	void inputEvent(const Input::Event &e);
	void draw(Base::FrameTimeBase frameTime);
	void init(const Base::Window &win);
};

class ViewStack : public ViewController
{
private:
	View *view[5] {nullptr};
	StackAllocator *viewAllocator[5] {nullptr};
	NavView *nav = nullptr;
	//ViewController *nextController = nullptr;
	IG::WindowRect viewRect, customViewRect;

public:
	uint size = 0;
	bool useNavView = 0;

	constexpr ViewStack() {}
	constexpr ViewStack(View &root): view{&root}, size{1} {}
	void init(const Base::Window &win);
	void setNavView(NavView *nav);
	NavView *navView() const;
	void place(const IG::WindowRect &rect);
	void place();
	void inputEvent(const Input::Event &e);
	void draw(Base::FrameTimeBase frameTime);
	void push(View &v, StackAllocator *allocator);
	void push(View &v);
	void pushAndShow(View &v, StackAllocator *allocator, bool needsNavView) override;
	void pushAndShow(View &v, StackAllocator *allocator);
	void pushAndShow(View &v);
	void pop();
	void popAndShow();
	void popToRoot();
	void popTo(View &v);
	void show();
	View &top() const;
	View &viewAtIdx(uint idx) const;
	int viewIdx(View &v) const;
	bool contains(View &v) const;
	void dismissView(View &v) override;
};
