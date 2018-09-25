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
#include <imagine/input/Input.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/gui/ScrollView.hh>

class MenuItem;

class TableView : public ScrollView
{
public:
	using ItemsDelegate = DelegateFunc<int (const TableView &view)>;
	using ItemDelegate = DelegateFunc<MenuItem& (const TableView &view, uint idx)>;
	static Gfx::GC globalXIndent;

	TableView(ViewAttachParams attach, ItemsDelegate items, ItemDelegate item);
	TableView(const char *name, ViewAttachParams attach, ItemsDelegate items, ItemDelegate item);
	template <class CONTAINER>
	TableView(ViewAttachParams attach, CONTAINER &item):
		TableView{"", attach, item} {}
	template <class CONTAINER>
	TableView(const char *name, ViewAttachParams attach, CONTAINER &item):
		TableView
		{
			name,
			attach,
			[&item](const TableView &) { return IG::size(item); },
			[&item](const TableView &, uint idx) -> MenuItem& { return derefMenuItem(IG::data(item)[idx]); }
		} {}
	IG::WindowRect &viewRect() override { return viewFrame; }
	void prepareDraw() override;
	void draw(Gfx::RendererCommands &cmds) override;
	void place() override;
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void resetScroll();
	bool inputEvent(Input::Event event) override;
	void clearSelection() override;
	void onAddedToController(Input::Event e) override;
	void setFocus(bool focused) override;
	uint cells() { return items(*this); }
	IG::WP cellSize() const { return {viewFrame.x, yCellSize}; }
	void highlightCell(int idx);
	void setAlign(_2DOrigin align);
	static void setDefaultXIndent(const Gfx::ProjectionPlane &projP);
	static MenuItem& derefMenuItem(MenuItem *item)
	{
		return *item;
	}
	static MenuItem& derefMenuItem(MenuItem &item)
	{
		return item;
	}

protected:
	bool onlyScrollIfNeeded = false;
	bool selectedIsActivated = false;
	bool hasFocus = true;
	int yCellSize = 0;
	int selected = -1;
	int visibleCells = 0;
	IG::WindowRect viewFrame{};
	_2DOrigin align{LC2DO};
	ItemsDelegate items{};
	ItemDelegate item{};

	void setYCellSize(int s);
	IG::WindowRect focusRect();
	virtual void onSelectElement(Input::Event e, uint i, MenuItem &item);
	bool elementIsSelectable(MenuItem &item);
	int nextSelectableElement(int start, int items);
	int prevSelectableElement(int start, int items);
	bool handleTableInput(Input::Event e, bool &movedSelected);
	virtual void drawElement(Gfx::RendererCommands &cmds, uint i, MenuItem &item, Gfx::GCRect rect) const;
};
