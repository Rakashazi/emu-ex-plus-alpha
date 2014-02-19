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

#include <base/Base.hh>
#include <base/Window.hh>
#include <input/Input.hh>
#include <input/DragPointer.hh>
#include <resource2/face/ResourceFace.hh>
#include <gfx/ProjectionPlane.hh>

/*class ViewAnimation
{
public:
	constexpr ViewAnimation() { }
	virtual void initShow() = 0;
	virtual void initActive() = 0;
	virtual void initDismiss() = 0;
	virtual bool update() = 0;
};*/

class View;
class StackAllocator;

class ViewController
{
public:
	constexpr ViewController() {}
	virtual void pushAndShow(View &v, StackAllocator *allocator, bool needsNavView) = 0;
	virtual void dismissView(View &v) = 0;
};

class View
{
public:
	Base::Window *win = nullptr;
	ViewController *controller = nullptr;
	static ResourceFace *defaultFace;
	static Gfx::ProjectionPlane projP;
	//enum { SHOW, ACTIVE, HIDE };
	//ViewAnimation *animation = nullptr;
	//uint displayState = 0;
	const char *name_ = "";

	constexpr View() {}
	constexpr View(Base::Window &win): win(&win) {}
	constexpr View(const char *name, Base::Window &win) : win(&win), name_(name) {}

	virtual void deinit() = 0;
	virtual IG::WindowRect &viewRect() = 0;
	virtual void place() = 0;
	virtual void draw(Base::FrameTimeBase frameTime) = 0;
	virtual void inputEvent(const Input::Event &event) = 0;
	virtual void clearSelection() {} // de-select any items from previous input
	virtual void onShow() {}

	void placeRect(IG::WindowRect rect)
	{
		this->viewRect() = rect;
		place();
	}

	void postDraw()
	{
		assert(win);
		win->postDraw();
	}

	Base::Window &window()
	{
		assert(win);
		return *win;
	}

	const char *name() { return name_; }

	// Does the platform need an on-screen/pointer-based control to move to a previous view?
	static bool needsBackControl;
	static const bool needsBackControlDefault = !(Config::envIsPS3 || Config::envIsAndroid || (Config::envIsWebOS && !Config::envIsWebOS3));
	static const bool needsBackControlIsConst = Config::envIsPS3 || Config::envIsIOS || Config::envIsWebOS3;

	static void setNeedsBackControl(bool on)
	{
		if(!needsBackControlIsConst) // only modify on environments that make sense
		{
			needsBackControl = on;
		}
	}
	static bool compileGfxPrograms();

	void dismiss();
	void pushAndShow(View &v, StackAllocator *allocator, bool needsNavView = true);

	void show(bool animated = 1)
	{
		onShow();
		//logMsg("showed view");
		postDraw();
	}

	void init()
	{

	}

	/*bool updateAnimation()
	{
		if(animation && animation->update())
		{
			// still animating
			postDraw();
			return 1;
		}

		if(displayState == HIDE)
		{
			// hide animation done, trigger view to dismiss
			doDismiss();
			return 0;
		}

		if(displayState == SHOW)
		{
			// show animation complete
			displayState = ACTIVE;
		}

		// not animating, view is active
		return 1;
	}*/
};
