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

#include <util/rectangle2.h>
#include <input/Input.hh>
#include <gfx/Gfx.hh>
#include <config/env.hh>
#include <config/machine.hh>
#include <gui/ScrollView1D/ScrollView1D.hh>

class GuiTable1D;

class GuiTableSource
{
public:
	constexpr GuiTableSource() { }
	virtual void drawElement(const GuiTable1D *table, uint element, GC xPos, GC yPos, GC xSize, GC ySize, _2DOrigin align) const = 0;
	virtual void onSelectElement(const GuiTable1D *table, const Input::Event &event, uint element) = 0;
};

class GuiTable1D
{
public:
	constexpr GuiTable1D() {}
	int yCellSize = 0;
	int cells = 0, selected = -1, selectedIsActivated = 0;
	GuiTableSource *src = nullptr;
	Rect2<int> viewRect;
	_2DOrigin align;

	void init(GuiTableSource *src, int cells, _2DOrigin align = LC2DO);
	void setXCellSize(int s);
	void setYCellSize(int s);
	bool inputEvent(const Input::Event &event);
	void draw();
	Rect2<int> focusRect();
	static GC globalXIndent;

	static void setDefaultXIndent()
	{
		GuiTable1D::globalXIndent =
			(Config::MACHINE_IS_OUYA) ? Gfx::xSMMSize(4) :
			(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS) ? /*floor*/(Gfx::xSMMSize(1)) :
			(Config::envIsPS3) ? /*floor*/(Gfx::xSMMSize(16)) :
			/*floor*/(Gfx::xSMMSize(2));
	}

	void clearSelection()
	{
		selected = -1;
	}

private:
	int visibleCells() const;
	int offscreenCells() const;
};

class ScrollableGuiTable1D : public GuiTable1D, public ScrollView1D
{
public:
	constexpr ScrollableGuiTable1D() {}
	bool onlyScrollIfNeeded = 0;

	void init(GuiTableSource *src, int cells, _2DOrigin align = LC2DO);
	void deinit();
	void draw();
	void place(Rect2<int> *frame);
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void updateView(); // move content frame in position along view frame
	void inputEvent(const Input::Event &e);
};
