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
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <vector>
#include <iterator>

class BaseAlertView : public View
{
public:
	BaseAlertView(ViewAttachParams attach, const char *label, TableView::ItemsDelegate items, TableView::ItemDelegate item);
	template <class CONTAINER>
	BaseAlertView(ViewAttachParams attach, const char *label, CONTAINER &item):
		BaseAlertView
		{
			attach,
			label,
			[&item](const TableView &) { return std::size(item); },
			[&item](const TableView &, uint32_t idx) -> MenuItem& { return TableView::derefMenuItem(std::data(item)[idx]); }
		} {}
	IG::WindowRect &viewRect() override { return rect; }
	void place() override;
	bool inputEvent(Input::Event e) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;
	void onAddedToController(Input::Event e) override;
	void setLabel(const char *label);

protected:
	Gfx::GCRect labelFrame{};
	Gfx::Text text{};
	TableView menu;
	IG::WindowRect rect{};
};

class AlertView : public BaseAlertView
{
public:
	AlertView(ViewAttachParams attach, const char *label, uint32_t menuItems);
	void setItem(uint32_t idx, const char *name, TextMenuItem::SelectDelegate del);
	template<class C>
	void setItem(uint32_t idx, const char *name, C &&del)
	{
		setItem(idx, name, TextMenuItem::wrapSelectDelegate(del));
	}

protected:
	std::vector<TextMenuItem> item;
};

class YesNoAlertView : public BaseAlertView
{
public:
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	template<class C = TextMenuItem::SelectDelegate, class C2 = TextMenuItem::SelectDelegate>
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
		C &&onYes, C2 &&onNo):
			YesNoAlertView
			{
				attach,
				label,
				yesStr,
				noStr,
				TextMenuItem::wrapSelectDelegate(onYes),
				TextMenuItem::wrapSelectDelegate(onNo)
			} {}
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr = {}, const char *noStr = {}):
		YesNoAlertView{attach, label, yesStr, noStr, {}, {}} {}
	void setOnYes(TextMenuItem::SelectDelegate del);
	template<class C>
	void setOnYes(C &&del)
	{
		setOnYes(TextMenuItem::wrapSelectDelegate(del));
	}
	void setOnNo(TextMenuItem::SelectDelegate del);
	template<class C>
	void setOnNo(C &&del)
	{
		setOnNo(TextMenuItem::wrapSelectDelegate(del));
	}

protected:
	TextMenuItem yes, no;
	TextMenuItem::SelectDelegate makeDefaultSelectDelegate();
};
