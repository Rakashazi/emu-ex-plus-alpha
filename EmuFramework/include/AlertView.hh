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
#include <util/Delegate.hh>

void removeModalView();

class AlertView : public View
{
public:
	Area labelFrame;
	GfxText text;
	BaseMenuView menu;
	Rect2<int> rect;

	Rect2<int> &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit();

	void place(Rect2<int> rect)
	{
		View::place(rect);
	}

	void place();
	void inputEvent(const InputEvent &e);
	void draw();
};

class YesNoAlertView : public AlertView
{
public:
	typedef Delegate<void (const InputEvent &e)> OnInputDelegate;

	TextMenuItem yes, no;

	void selectYes(TextMenuItem &, const InputEvent &e)
	{
		removeModalView();
		onYes.invoke(e);
	}

	static void selectNo(TextMenuItem &, const InputEvent &e)
	{
		removeModalView();
	}

	MenuItem *menuItem[2];

	// Required delegates
	OnInputDelegate onYes;
	OnInputDelegate &onYesDelegate() { return onYes; }

	void init(const char *label, bool highlightFirst)
	{
		yes.init("Yes"); menuItem[0] = &yes;
		yes.selectDelegate().bind<YesNoAlertView, &YesNoAlertView::selectYes>(this);
		no.init("No"); menuItem[1] = &no;
		no.selectDelegate().bind<&selectNo>();
		AlertView::init(label, menuItem, highlightFirst);
	}
};
