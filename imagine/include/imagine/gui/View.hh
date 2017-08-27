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

#include <imagine/config/defs.hh>
#include <imagine/base/Base.hh>
#include <imagine/base/Window.hh>
#include <imagine/input/Input.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/gfx/ProjectionPlane.hh>

class View;

class ViewController
{
public:
	constexpr ViewController() {}
	virtual void pushAndShow(View &v, Input::Event e, bool needsNavView) = 0;
	virtual void pop() = 0;
	virtual void popAndShow() { pop(); };
	virtual void dismissView(View &v) = 0;
};

struct ViewAttachParams
{
	Base::Window &win;
	Gfx::Renderer &renderer;
};

class View
{
public:
	static Gfx::GlyphTextureSet defaultFace;
	static Gfx::GlyphTextureSet defaultBoldFace;
	// Does the platform need an on-screen/pointer-based control to move to a previous view?
	static bool needsBackControl;
	static const bool needsBackControlDefault = !(Config::envIsPS3 || Config::envIsAndroid || (Config::envIsWebOS && !Config::envIsWebOS3));
	static const bool needsBackControlIsConst = Config::envIsPS3 || Config::envIsIOS || Config::envIsWebOS3;

	constexpr View() {}
	virtual ~View() {}
	constexpr View(ViewAttachParams attach):
		win{&attach.win}, renderer_{&attach.renderer} {}
	constexpr View(const char *name, ViewAttachParams attach):
		win(&attach.win), renderer_{&attach.renderer}, name_(name) {}

	virtual IG::WindowRect &viewRect() = 0;
	virtual void place() = 0;
	virtual void draw() = 0;
	virtual bool inputEvent(Input::Event event) = 0;
	virtual void clearSelection() {} // de-select any items from previous input
	virtual void onShow() {}
	virtual void onAddedToController(Input::Event e) = 0;

	void setViewRect(IG::WindowRect rect, Gfx::ProjectionPlane projP);
	void postDraw();
	Base::Window &window();
	Gfx::Renderer &renderer();
	ViewAttachParams attachParams();
	Base::Screen *screen();
	const char *name() { return name_; }
	void setName(const char *name) { name_ = name; }
	static void setNeedsBackControl(bool on);
	static bool compileGfxPrograms(Gfx::Renderer &r);
	void dismiss();
	void pushAndShow(View &v, Input::Event e, bool needsNavView = true);
	void pop();
	void popAndShow();
	void show();
	void setWindow(Base::Window *w) { win = w; }
	void setController(ViewController *c, Input::Event e);
	Gfx::ProjectionPlane projection() { return projP; }
	bool pointIsInView(IG::WP pos);

protected:
	Base::Window *win{};
	Gfx::Renderer *renderer_{};
	ViewController *controller{};
	Gfx::ProjectionPlane projP{};
	const char *name_ = "";
};
