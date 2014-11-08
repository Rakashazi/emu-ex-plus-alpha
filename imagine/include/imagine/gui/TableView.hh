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

#include <imagine/engine-globals.h>
#include <imagine/util/rectangle2.h>
#include <imagine/input/Input.hh>
#include <imagine/gfx/Gfx.hh>
#include <imagine/gui/ScrollView.hh>

class MenuItem;

class TableView : public ScrollView
{
public:
	static Gfx::GC globalXIndent;
	bool onlyScrollIfNeeded = false;

	TableView(Base::Window &win): ScrollView(win) {}
	TableView(const char *name, Base::Window &win) : ScrollView(name, win) {}
	IG::WindowRect &viewRect() override { return viewFrame; }
	void init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align = LC2DO);
	void deinit() override;
	void draw() override;
	void place() override;
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void inputEvent(const Input::Event &event) override;
	void clearSelection() override;
	static void setDefaultXIndent(const Gfx::ProjectionPlane &projP);
	int cells() const { return cells_; }
	IG::WP cellSize() const { return {viewFrame.x, yCellSize}; }
	void highlightFirstCell();

protected:
	bool selectedIsActivated = false;
	int yCellSize = 0;
	int cells_ = 0;
	int selected = -1;
	int visibleCells = 0;
	MenuItem **item{};
	IG::WindowRect viewFrame;
	_2DOrigin align;

	void setYCellSize(int s);
	IG::WindowRect focusRect();
	void onSelectElement(const Input::Event &e, uint i);
	bool elementIsSelectable(uint element);
	int nextSelectableElement(int start);
	int prevSelectableElement(int start);
	bool handleTableInput(const Input::Event &e);
	virtual void drawElement(uint i, Gfx::GCRect rect) const;
};
