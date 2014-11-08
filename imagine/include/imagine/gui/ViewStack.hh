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

#include <imagine/engine-globals.h>
#include <imagine/gui/View.hh>
#include <imagine/gui/NavView.hh>
#include <utility>
#include <memory>

class BasicViewController : public ViewController
{
	View *view{};
	IG::WindowRect viewRect;
	Gfx::ProjectionPlane projP;
	typedef DelegateFunc<void ()> RemoveViewDelegate;
	RemoveViewDelegate removeViewDel;

public:
	constexpr BasicViewController() {}
	RemoveViewDelegate &onRemoveView() { return removeViewDel; }
	void push(View &v);
	void pushAndShow(View &v, bool needsNavView) override;
	void pushAndShow(View &v);
	void pop() override;
	void dismissView(View &v) override;
	void place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP);
	void place();
	bool hasView() { return view; }
	void inputEvent(const Input::Event &e);
	void draw();
	void init(const Base::Window &win);
};

class ViewStack : public ViewController
{
private:
	View *view[5]{};
	NavView *nav{};
	//ViewController *nextController{};
	IG::WindowRect viewRect, customViewRect;
	Gfx::ProjectionPlane projP;

public:
	uint size = 0;
	bool useNavView = 0;

	constexpr ViewStack() {}
	constexpr ViewStack(View &root): view{&root}, size{1} {}
	void init(const Base::Window &win);
	void setNavView(NavView *nav);
	NavView *navView() const;
	void place(const IG::WindowRect &rect, const Gfx::ProjectionPlane &projP);
	void place();
	void inputEvent(const Input::Event &e);
	void draw();
	void push(View &v);
	void pushAndShow(View &v, bool needsNavView) override;
	void pushAndShow(View &v);
	void pop() override;
	void popAndShow() override;
	void popToRoot();
	void popTo(View &v);
	void show();
	View &top() const;
	View &viewAtIdx(uint idx) const;
	int viewIdx(View &v) const;
	bool contains(View &v) const;
	void dismissView(View &v) override;
};
