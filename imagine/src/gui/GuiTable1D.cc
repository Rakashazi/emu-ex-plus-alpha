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

#define LOGTAG "GuiTable1D"

#include <imagine/gui/GuiTable1D.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/input/Input.hh>
#include <imagine/base/Base.hh>
#include <algorithm>
#include <imagine/util/number.h>

Gfx::GC GuiTable1D::globalXIndent = 0;

void GuiTable1D::init(GuiTableSource *src, int cells)
{
	var_selfs(src);
	var_selfs(cells);
	selected = -1;
	if(!Input::SUPPORTS_POINTER && cells)
		selected = 0;
	selectedIsActivated = 0;
}

void GuiTable1D::setXCellSize(int s)
{
	viewRect.x2 = viewRect.x + (s-1);
}

void GuiTable1D::setYCellSize(int s)
{
	yCellSize = s;
	viewRect.y2 = viewRect.y + ((cells * s)-1);
}

int GuiTable1D::nextSelectableElement(int start)
{
	using namespace IG;
	int elem = wrapToBound(start, 0, cells);
	iterateTimes(cells, i)
	{
		if(src->elementIsSelectable(*this, elem))
		{
			return elem;
		}
		elem = wrapToBound(elem+1, 0, cells);
	}
	return -1;
}

int GuiTable1D::prevSelectableElement(int start)
{
	using namespace IG;
	int elem = wrapToBound(start, 0, cells);
	iterateTimes(cells, i)
	{
		if(src->elementIsSelectable(*this, elem))
		{
			return elem;
		}
		elem = wrapToBound(elem-1, 0, cells);
	}
	return -1;
}

bool GuiTable1D::inputEvent(const Input::Event &e, View &view)
{
	using namespace IG;
	if(cells == 0)
		return false;

	bool movedSelected = false;

	if(e.isPointer())
	{
		if(!viewRect.overlaps({e.x, e.y}) || e.button != Input::Pointer::LBUTTON)
		{
			//logMsg("cursor not in table");
			return false;
		}

		if(e.state == Input::PUSHED)
		{
			int i = (e.y - viewRect.y) / yCellSize;
			logMsg("input pushed on cell %d", i);
			if(i >= 0 && i < cells && src->elementIsSelectable(*this, i))
			{
				selected = i;
				view.postDraw();
			}
		}
		else if(e.state == Input::RELEASED) // TODO, need to check that Input::PUSHED was sent on entry
		{
			int i = (e.y - viewRect.y) / yCellSize;
			//logMsg("input released on cell %d", i);
			if(i >= 0 && i < cells && selected == i && src->elementIsSelectable(*this, i))
			{
				logDMsg("entry %d pushed", i);
				selectedIsActivated = 1;
				view.postDraw();
				selected = -1;
				src->onSelectElement(*this, e, i);
			}
		}
	}
	else // Key or Relative Pointer
	{
		if(e.isRelativePointer())
			logMsg("got rel pointer %d", e.y);

		if((e.pushed() && e.isDefaultUpButton())
			|| (e.isRelativePointer() && e.y < 0))
		{
			//logMsg("move up %d", selected);
			if(selected == -1)
				selected = prevSelectableElement(cells - 1);//cells - 1;
			else
			{
				//logMsg("wrapping %d-1 to 0-%d", selected, cells);
				selected = prevSelectableElement(selected - 1);//wrapToBound(selected-1, 0, cells);
			}
			logMsg("up, selected %d", selected);
			view.postDraw();
			movedSelected = true;
		}
		else if((e.pushed() && e.isDefaultDownButton())
			|| (e.isRelativePointer() && e.y > 0))
		{
			if(selected == -1)
				selected = nextSelectableElement(0);//0;
			else
				selected = nextSelectableElement(selected + 1);//wrapToBound(selected+1, 0, cells);
			logMsg("down, selected %d", selected);
			view.postDraw();
			movedSelected = true;
		}
		else if(e.pushed() && e.isDefaultConfirmButton())
		{
			if(selected != -1)
			{
				logDMsg("entry %d pushed", selected);
				selectedIsActivated = 1;
				src->onSelectElement(*this, e, selected);
			}
		}
		else if(e.pushed() && e.isDefaultPageUpButton())
		{
			if(selected == -1)
				selected = cells - 1;
			else
				selected = clipToBounds(selected-visibleCells(view.projP), 0, cells-1);
			logMsg("selected %d", selected);
			view.postDraw();
			movedSelected = true;
		}
		else if(e.pushed() && e.isDefaultPageDownButton())
		{
			if(selected == -1)
				selected = 0;
			else
				selected = clipToBounds(selected+visibleCells(view.projP), 0, cells-1);
			logMsg("selected %d", selected);
			view.postDraw();
			movedSelected = true;
		}
	}

	return movedSelected;
}

int GuiTable1D::visibleCells(const Gfx::ProjectionPlane &projP) const
{
	//int visYCells = ceil(View::projP.h/Gfx::unprojectYSize(yCellSize)) + 1;
	int visYCells = IG::divRoundUp(projP.viewport.height(), yCellSize) + 1;
	if(offscreenCells(projP) < 0)
		visYCells += offscreenCells(projP);
	visYCells = IG::clipToBounds(visYCells, 0, cells);
	return visYCells;
}

int GuiTable1D::offscreenCells(const Gfx::ProjectionPlane &projP) const
{
	auto y = viewRect.yPos(LT2DO);
	return (projP.unprojectY(y) - projP.hHalf())/projP.unprojectYSize(yCellSize);
}

void GuiTable1D::draw(const Gfx::ProjectionPlane &projP)
{
	using namespace Gfx;
	if(cells == 0)
		return;
	auto y = viewRect.yPos(LT2DO);
	auto x = viewRect.xPos(LT2DO);
	// TODO: fix calculations
	int visYCells = visibleCells(projP);
	//int visYCells = ceil(gfx_viewHeight()/gfx_iYSize(yCellSize)) + 1;
	int startYCell = 0;
	int yOffScreen = offscreenCells(projP);
	//logMsg("%d cells offscreen", yOffScreen);
	/*if(yOffScreen < 0)
		visYCells += yOffScreen;
	visYCells = clipToBounds(visYCells, 0, cells);*/
	//logMsg("%d cells visible", visYCells);
	if(yOffScreen > 0)
		startYCell += yOffScreen;
	y += yCellSize*startYCell;
	//Gfx::GC separatorYSize = unprojectYSize(1);
	int endYCell = std::min(startYCell+visYCells, cells);

	// draw separators
	int yStart = y;
	noTexProgram.use(projP.makeTranslate());
	int selectedCellY = INT_MAX;
	setBlendMode(0);
	for(int i = startYCell; i < endYCell; i++)
	{
		/*if(i >= cells)
			break;*/
		if(i == selected)
		{
			selectedCellY = y;
		}
		if(i != 0)
		{
			int ySize = 1;
			if(!src->elementIsSelectable(*this, i - 1))
			{
				setColor(.4, .4, .4);
				ySize = 4;
			}
			else
			{
				setColor(.2, .2, .2);
			}
			auto rect = IG::makeWindowRectRel({x, y-1}, {viewRect.xSize(), ySize});
			GeomRect::draw(rect, projP);
		}
		y += yCellSize;
	}

	// draw selected rectangle
	if(selectedCellY != INT_MAX)
	{
		setBlendMode(BLEND_MODE_ALPHA);
		/*if(selectedIsActivated)
			gfx_setColor(0, 0, 1., .8);
		else*/
			setColor(.2, .71, .9, 1./3.);
		auto rect = IG::makeWindowRectRel({x, selectedCellY}, {viewRect.xSize(), yCellSize-1});
		GeomRect::draw(rect, projP);
	}

	// draw elements
	y = yStart;
	for(int i = startYCell; i < endYCell; i++)
	{
		/*logMsg("drawing entry %d %d %f %f, size %f %f", x, y,
				(float)gfx_iXPos(x), (float)gfx_iYPos(y) - gfx_iYSize(yCellSize/2),
				gfx_iXSize(viewRect.xSize()), gfx_iYSize(yCellSize));*/
		auto rect = IG::makeWindowRectRel({x, y}, {viewRect.xSize(), yCellSize});
		src->drawElement(*this, i, projP.unProjectRect(rect));
		y += yCellSize;
	}
}

IG::WindowRect GuiTable1D::focusRect()
{
	if(selected >= 0)
		return IG::makeWindowRectRel(viewRect.pos(LT2DO) + IG::Point2D<int>{0, yCellSize*selected}, {viewRect.xSize(), yCellSize});
	else
		return {};
}
