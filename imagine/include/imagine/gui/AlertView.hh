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
#include <imagine/util/typeTraits.hh>
#include <utility>

namespace IG
{

class BaseAlertView : public View
{
public:
	BaseAlertView(ViewAttachParams attach, IG::utf16String label, TableView::ItemsDelegate items, TableView::ItemDelegate item);
	BaseAlertView(ViewAttachParams attach, IG::utf16String label, IG::Container auto &item):
		BaseAlertView
		{
			attach,
			std::move(label),
			[&item](const TableView &) { return std::size(item); },
			[&item](const TableView &, size_t idx) -> MenuItem& { return IG::deref(std::data(item)[idx]); }
		} {}
	void place() override;
	bool inputEvent(const Input::Event &) override;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &) override;
	void onAddedToController(ViewController *, const Input::Event &) override;
	void setLabel(IG::utf16String label);

protected:
	Gfx::GCRect labelFrame{};
	Gfx::Text text{};
	TableView menu;
};

class AlertView : public BaseAlertView
{
public:
	AlertView(ViewAttachParams attach, IG::utf16String label, size_t menuItems);
	void setItem(size_t idx, IG::utf16String name, TextMenuItem::SelectDelegate del);

protected:
	std::vector<TextMenuItem> item;
};

class YesNoAlertView : public BaseAlertView
{
public:
	YesNoAlertView(ViewAttachParams attach, IG::utf16String label, IG::utf16String yesStr, IG::utf16String noStr,
		TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);
	YesNoAlertView(ViewAttachParams attach, IG::utf16String label,
		IG::utf16String yesStr = {}, IG::utf16String noStr = {}):
		YesNoAlertView{attach, std::move(label), std::move(yesStr), std::move(noStr), {}, {}} {}
	void setOnYes(TextMenuItem::SelectDelegate del);
	void setOnNo(TextMenuItem::SelectDelegate del);

protected:
	TextMenuItem yes, no;
	TextMenuItem::SelectDelegate makeDefaultSelectDelegate();
};

}
