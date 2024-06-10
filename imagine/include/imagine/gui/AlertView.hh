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

#include <imagine/config/defs.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/util/concepts.hh>
#include <vector>
#include <array>

namespace IG
{

class BaseAlertView : public View
{
public:
	BaseAlertView(ViewAttachParams attach, UTF16Convertible auto &&label, TableView::ItemSourceDelegate items):
		View{attach},
		bgQuads{attach.rendererTask, {.size = 2}},
		text{attach.rendererTask, IG_forward(label), &defaultFace()},
		menu
		{
			attach,
			items
		} { init(); }

	void place() override;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &__restrict__, ViewDrawParams p = {}) const override;
	void onAddedToController(ViewController *, const Input::Event &) override;
	void setLabel(UTF16Convertible auto &&label) { text.resetString(IG_forward(label)); }

protected:
	WRect labelFrame;
	Gfx::IQuads bgQuads;
	Gfx::Text text;
	TableView menu;

	void init();
};

class AlertView : public BaseAlertView
{
public:
	AlertView(ViewAttachParams attach, UTF16Convertible auto &&label, size_t menuItems):
		BaseAlertView{attach, IG_forward(label), item}
	{
		item.reserve(menuItems);
	}

protected:
	std::vector<TextMenuItem> item;
};

class YesNoAlertView : public BaseAlertView
{
public:
	struct Delegates
	{
		TextMenuItem::SelectDelegate onYes{[]{}};
		TextMenuItem::SelectDelegate onNo{[]{}};
	};

	YesNoAlertView(ViewAttachParams attach, UTF16Convertible auto &&label,
		UTF16Convertible auto &&yesStr, UTF16Convertible auto &&noStr,
		Delegates delegates):
		BaseAlertView{attach, IG_forward(label), yesNo},
		yesNo
		{
			TextMenuItem{IG_forward(yesStr), attach, delegates.onYes},
			TextMenuItem{IG_forward(noStr), attach, delegates.onNo}
		} {}

	YesNoAlertView(ViewAttachParams attach, UTF16Convertible auto &&label, Delegates delegates):
		YesNoAlertView{attach, IG_forward(label), u"Yes", u"No", delegates} {}

	void setOnYes(TextMenuItem::SelectDelegate del);
	void setOnNo(TextMenuItem::SelectDelegate del);

protected:
	std::array<TextMenuItem, 2> yesNo;
};

}
