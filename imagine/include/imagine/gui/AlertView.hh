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

#include <imagine/engine-globals.h>
#include <imagine/gui/View.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>

class AlertView : public View
{
public:
	AlertView(Base::Window &win): View{win}, menu{win} {}
	Gfx::GCRect labelFrame;
	Gfx::Text text;
	TableView menu;
	IG::WindowRect rect;

	IG::WindowRect &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw() override;
};

class YesNoAlertView : public AlertView
{
public:
	using InputDelegate = DelegateFunc<void (const Input::Event &e)>;
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
