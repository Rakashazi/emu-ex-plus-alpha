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

namespace IG::Input
{
class Event;
}

namespace IG::Gfx
{
class RendererCommands;
}

namespace IG
{

class Window;
class MenuItem;

class TableView : public ScrollView
{
public:
	using ItemsDelegate = DelegateFunc<size_t (const TableView &view)>;
	using ItemDelegate = DelegateFunc<MenuItem& (const TableView &view, size_t idx)>;
	using SelectElementDelegate = DelegateFunc<void (Input::Event e, int i, MenuItem &item)>;

	TableView(IG::utf16String name, ViewAttachParams attach, ItemsDelegate items, ItemDelegate item):
		ScrollView{attach}, items{items}, item{item}, nameStr{std::move(name)} {}

	TableView(ViewAttachParams attach, IG::Container auto &item):
		TableView{{}, attach, item} {}

	TableView(IG::utf16String name, ViewAttachParams attach, IG::Container auto &item):
		TableView
		{
			std::move(name),
			attach,
			[&item](const TableView &) { return std::size(item); },
			[&item](const TableView &, size_t idx) -> MenuItem& { return IG::deref(std::data(item)[idx]); }
		} {}

	TableView(ViewAttachParams attach, ItemsDelegate items, ItemDelegate item);
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
	size_t cells() const;
	IG::WP cellSize() const;
	void highlightCell(int idx);
	void setAlign(_2DOrigin align);
	std::u16string_view name() const override;
	void setName(IG::utf16String name) { nameStr = std::move(name); }

protected:
	ItemsDelegate items{};
	ItemDelegate item{};
	SelectElementDelegate selectElementDel{};
	std::u16string nameStr{};
	int yCellSize = 0;
	int selected = -1;
	int visibleCells = 0;
	_2DOrigin align{LC2DO};
	bool onlyScrollIfNeeded = false;
	bool selectedIsActivated = false;
	bool hasFocus = true;

	void setYCellSize(int s);
	IG::WindowRect focusRect();
	void onSelectElement(Input::Event e, size_t i, MenuItem &item);
	bool elementIsSelectable(MenuItem &item);
	int nextSelectableElement(int start, int items);
	int prevSelectableElement(int start, int items);
	bool handleTableInput(Input::Event e, bool &movedSelected);
	virtual void drawElement(Gfx::RendererCommands &cmds, size_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const;
};

}
