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

#pragma once

#include <gui/View.hh>
#include <gui/MenuItem/MenuItem.hh>
#include <util/gui/BaseMenuView.hh>
#include <util/rectangle2.h>
#include <util/DelegateFunc.hh>

class AlertView : public View
{
public:
	constexpr AlertView(Base::Window &win): View{win}, menu{win} {}
	IG::Rect2<GC> labelFrame;
	Gfx::Text text;
	BaseMenuView menu;
	IG::Rect2<int> rect;

	IG::Rect2<int> &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
};

class YesNoAlertView : public AlertView
{
public:

	typedef DelegateFunc<void (const Input::Event &e)> InputDelegate;
	InputDelegate onYesD;
	InputDelegate onNoD;
	MenuItem *menuItem[2] {nullptr};

	YesNoAlertView(Base::Window &win);
	void init(const char *label, bool highlightFirst, const char *choice1 = nullptr, const char *choice2 = nullptr);
	void deinit() override;
	// Optional delegates
	InputDelegate &onYes() { return onYesD; }
	InputDelegate &onNo() { return onNoD; }

private:
	TextMenuItem yes, no;
};
