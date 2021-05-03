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

#include <imagine/config/defs.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/gfx/defs.hh>
#include <imagine/gui/ScrollView.hh>
#include <iterator>

namespace Base
{
class Window;
}

namespace Input
{
class Event;
}

namespace Gfx
{
class RendererCommands;
}

class MenuItem;

class TableView : public ScrollView
{
public:
	using ItemsDelegate = DelegateFunc<int (const TableView &view)>;
	using ItemDelegate = DelegateFunc<MenuItem& (const TableView &view, uint32_t idx)>;
	using SelectElementDelegate = DelegateFunc<void (Input::Event e, uint32_t i, MenuItem &item)>;

	TableView(ViewAttachParams attach, ItemsDelegate items, ItemDelegate item);
	TableView(NameString name, ViewAttachParams attach, ItemsDelegate items, ItemDelegate item);
	TableView(const char *name, ViewAttachParams attach, ItemsDelegate items, ItemDelegate item):
		TableView{makeNameString(name), attach, items, item} {}
	template <class Container>
	TableView(ViewAttachParams attach, Container &item):
		TableView{{}, attach, item} {}
	template <class Container>
	TableView(NameString name, ViewAttachParams attach, Container &item):
		TableView
		{
			std::move(name),
			attach,
			[&item](const TableView &) { return std::size(item); },
			[&item](const TableView &, uint32_t idx) -> MenuItem& { return derefMenuItem(std::data(item)[idx]); }
		} {}
	template <class Container>
	TableView(const char *name, ViewAttachParams attach, Container &item):
		TableView{makeNameString(name), attach, item} {}
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;
	void place() override;
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void resetScroll();
	bool inputEvent(Input::Event event) override;
	void clearSelection() override;
	void onShow() override;
	void onHide() override;
	void onAddedToController(ViewController *c, Input::Event e) override;
	void setFocus(bool focused) override;
	void setOnSelectElement(SelectElementDelegate del);
	uint32_t cells() const;
	IG::WP cellSize() const;
	void highlightCell(int idx);
	void setAlign(_2DOrigin align);
	static MenuItem& derefMenuItem(MenuItem *item)
	{
		return *item;
	}
	static MenuItem& derefMenuItem(MenuItem &item)
	{
		return item;
	}

protected:
	ItemsDelegate items{};
	ItemDelegate item{};
	SelectElementDelegate selectElementDel{};
	int yCellSize = 0;
	int selected = -1;
	int visibleCells = 0;
	_2DOrigin align{LC2DO};
	bool onlyScrollIfNeeded = false;
	bool selectedIsActivated = false;
	bool hasFocus = true;

	void setYCellSize(int s);
	IG::WindowRect focusRect();
	void onSelectElement(Input::Event e, uint32_t i, MenuItem &item);
	bool elementIsSelectable(MenuItem &item);
	int nextSelectableElement(int start, int items);
	int prevSelectableElement(int start, int items);
	bool handleTableInput(Input::Event e, bool &movedSelected);
	virtual void drawElement(Gfx::RendererCommands &cmds, uint32_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const;
};
