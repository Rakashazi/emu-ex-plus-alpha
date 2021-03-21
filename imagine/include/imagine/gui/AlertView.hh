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
#include <utility>

class BaseAlertView : public View
{
public:
	BaseAlertView(ViewAttachParams attach, const char *label, TableView::ItemsDelegate items, TableView::ItemDelegate item);
	template <class Container>
	BaseAlertView(ViewAttachParams attach, const char *label, Container &item):
		BaseAlertView
		{
			attach,
			label,
			[&item](const ::TableView &) { return std::size(item); },
			[&item](const ::TableView &, uint32_t idx) -> MenuItem& { return ::TableView::derefMenuItem(std::data(item)[idx]); }
		} {}
	void place() override;
	bool inputEvent(Input::Event e) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;
	void onAddedToController(ViewController *c, Input::Event e) override;
	void setLabel(const char *label);

protected:
	Gfx::GCRect labelFrame{};
	Gfx::Text text{};
	TableView menu;
};

class AlertView : public BaseAlertView
{
public:
	AlertView(ViewAttachParams attach, const char *label, uint32_t menuItems);
	void setItem(uint32_t idx, const char *name, TextMenuItem::SelectDelegate del);
	template<class Func>
	void setItem(uint32_t idx, const char *name, Func &&func)
	{
		setItem(idx, name, TextMenuItem::makeSelectDelegate(std::forward<Func>(func)));
	}

protected:
	std::vector<TextMenuItem> item;
};

class YesNoAlertView : public BaseAlertView
{
public:
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	template<class Func1, class Func2>
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr, const char *noStr,
		Func1 &&onYes, Func2 &&onNo):
			YesNoAlertView
			{
				attach,
				label,
				yesStr,
				noStr,
				TextMenuItem::makeSelectDelegate(std::forward<Func1>(onYes)),
				TextMenuItem::makeSelectDelegate(std::forward<Func2>(onNo))
			} {}
	YesNoAlertView(ViewAttachParams attach, const char *label, const char *yesStr = {}, const char *noStr = {}):
		YesNoAlertView{attach, label, yesStr, noStr, {}, {}} {}
	void setOnYes(TextMenuItem::SelectDelegate del);
	template<class Func>
	void setOnYes(Func &&func)
	{
		setOnYes(TextMenuItem::makeSelectDelegate(std::forward<Func>(func)));
	}
	void setOnNo(TextMenuItem::SelectDelegate del);
	template<class Func>
	void setOnNo(Func &&func)
	{
		setOnNo(TextMenuItem::makeSelectDelegate(std::forward<Func>(func)));
	}

protected:
	TextMenuItem yes, no;
	TextMenuItem::SelectDelegate makeDefaultSelectDelegate();
};
