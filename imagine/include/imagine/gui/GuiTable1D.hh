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
#include <imagine/gui/ScrollView1D.hh>

class GuiTable1D;

class GuiTableSource
{
public:
	constexpr GuiTableSource() { }
	virtual void drawElement(const GuiTable1D &table, uint element, Gfx::GCRect rect) const = 0;
	virtual void onSelectElement(const GuiTable1D *table, const Input::Event &event, uint element) = 0;
};

class GuiTable1D
{
public:
	int yCellSize = 0;
	int cells = 0, selected = -1, selectedIsActivated = 0;
	GuiTableSource *src = nullptr;
	IG::WindowRect viewRect;
	static Gfx::GC globalXIndent;

	constexpr GuiTable1D() {}
	void init(GuiTableSource *src, int cells);
	void setXCellSize(int s);
	void setYCellSize(int s);
	bool inputEvent(const Input::Event &event, View &view);
	void draw(const Gfx::ProjectionPlane &projP);
	IG::WindowRect focusRect();

	static void setDefaultXIndent(const Gfx::ProjectionPlane &projP)
	{
		GuiTable1D::globalXIndent =
			(Config::MACHINE_IS_OUYA) ? projP.xSMMSize(4) :
			(Config::MACHINE_IS_PANDORA) ? projP.xSMMSize(2) :
			(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS) ? /*floor*/(projP.xSMMSize(1)) :
			(Config::envIsPS3) ? /*floor*/(projP.xSMMSize(16)) :
			/*floor*/(projP.xSMMSize(4));
	}

	void clearSelection()
	{
		selected = -1;
	}

private:
	int visibleCells(const Gfx::ProjectionPlane &projP) const;
	int offscreenCells(const Gfx::ProjectionPlane &projP) const;
};

class ScrollableGuiTable1D : public GuiTable1D, public ScrollView1D
{
public:
	bool onlyScrollIfNeeded = 0;

	constexpr ScrollableGuiTable1D() {}
	void init(GuiTableSource *src, int cells, View &view);
	void deinit();
	void draw(View &view);
	void place(IG::WindowRect *frame, View &view);
	void setScrollableIfNeeded(bool yes);
	void scrollToFocusRect();
	void updateView(); // move content frame in position along view frame
	void inputEvent(const Input::Event &e, View &view);
};
