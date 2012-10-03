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

class AlertView : public View
{
public:
	constexpr AlertView() { }
	Rect2<GC> labelFrame;
	GfxText text;
	BaseMenuView menu;
	Rect2<int> rect;

	Rect2<int> &viewRect() { return rect; }

	void init(const char *label, MenuItem **menuItem, bool highlightFirst);
	void deinit() override;
	void place() override;
	void inputEvent(const InputEvent &e) override;
	void draw() override;
};

class YesNoAlertView : public AlertView
{
public:
	constexpr YesNoAlertView() { }
	typedef Delegate<void (const InputEvent &e)> OnInputDelegate;

	TextMenuItem yes {TextMenuItem::SelectDelegate::create<YesNoAlertView, &YesNoAlertView::selectYes>(this)},
		no {TextMenuItem::SelectDelegate::create<YesNoAlertView, &YesNoAlertView::selectNo>(this)};

	void selectYes(TextMenuItem &, const InputEvent &e)
	{
		removeModalView();
		onYes.invoke(e);
	}

	void selectNo(TextMenuItem &, const InputEvent &e)
	{
		removeModalView();
		onNo.invokeSafe(e);
	}

	MenuItem *menuItem[2] = {nullptr};

	// Required delegates
	OnInputDelegate onYes;
	OnInputDelegate &onYesDelegate() { return onYes; }

	// Optional delegates
	OnInputDelegate onNo;
	OnInputDelegate &onNoDelegate() { return onNo; }

	void init(const char *label, bool highlightFirst)
	{
		yes.init("Yes"); menuItem[0] = &yes;
		no.init("No"); menuItem[1] = &no;
		onYes.clear();
		onNo.clear();
		AlertView::init(label, menuItem, highlightFirst);
	}
};
