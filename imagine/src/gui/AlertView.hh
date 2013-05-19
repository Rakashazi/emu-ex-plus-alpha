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
	constexpr AlertView() { }
	Rect2<GC> labelFrame;
	Gfx::Text text;
	BaseMenuView menu;
	Rect2<int> rect;

	Rect2<int> &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
};

class YesNoAlertView : public AlertView
{
public:
	YesNoAlertView() { }
	typedef DelegateFunc<void (const Input::Event &e)> InputDelegate;

	MenuItem *menuItem[2] = {nullptr};

	// Optional delegates
	InputDelegate &onYes() { return onYesD; }
	InputDelegate &onNo() { return onNoD; }

	void init(const char *label, bool highlightFirst, const char *choice1 = nullptr, const char *choice2 = nullptr)
	{
		yes.init(choice1 ? choice1 : "Yes"); menuItem[0] = &yes;
		no.init(choice2 ? choice2 : "No"); menuItem[1] = &no;
		assert(!onYesD);
		assert(!onNoD);
		AlertView::init(label, menuItem, highlightFirst);
	}

	void deinit() override
	{
		logMsg("deinit alert");
		AlertView::deinit();
		onYesD = {};
		onNoD = {};
	}

	InputDelegate onYesD;
	InputDelegate onNoD;
private:
	TextMenuItem yes
	{
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto callback = onYesD;
			removeModalView();
			if(callback) callback(e);
		}
	};
	TextMenuItem no
	{
		[this](TextMenuItem &, const Input::Event &e)
		{
			auto callback = onNoD;
			removeModalView();
			if(callback) callback(e);
		}
	};
};
