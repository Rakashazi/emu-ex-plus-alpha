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

#define thisModuleName "GuiTable1D"

#include "GuiTable1D.hh"
#include <gfx/GeomRect.hh>
#include <input/Input.hh>
#include <base/Base.hh>
#include <algorithm>

GC GuiTable1D::globalXIndent = 0;

void GuiTable1D::init(GuiTableSource *src, int cells, _2DOrigin align)
{
	this->src = src;
	this->cells = cells;
	selected = -1;
	if(!Input::SUPPORTS_POINTER && cells)
		selected = 0;
	this->align = align;
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

bool GuiTable1D::inputEvent(const Input::Event &e)
{
	using namespace IG;
	if(cells == 0)
		return false;

	bool usedInput = false;

	if(e.isPointer())
	{
		if(!viewRect.overlaps(e.x, e.y) || e.button != Input::Pointer::LBUTTON)
		{
			//logMsg("cursor not in table");
			return false;
		}

		if(e.state == Input::PUSHED)
		{
			int i = (e.y - viewRect.y) / yCellSize;
			logMsg("input pushed on cell %d", i);
			if(i >= 0 && i < cells)
			{
				selected = i;
				Base::displayNeedsUpdate();
				usedInput = true;
			}
		}
		else if(e.state == Input::RELEASED) // TODO, need to check that Input::PUSHED was sent on entry
		{
			int i = (e.y - viewRect.y) / yCellSize;
			//logMsg("input released on cell %d", i);
			if(i >= 0 && i < cells && selected == i)
			{
				logDMsg("entry %d pushed", i);
				selectedIsActivated = 1;
				Base::displayNeedsUpdate();
				usedInput = true;
				src->onSelectElement(this, e, i);
				selected = -1;
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
				selected = cells - 1;
			else
			{
				//logMsg("wrapping %d-1 to 0-%d", selected, cells);
				selected = wrapToBound(selected-1, 0, cells);
			}
			logMsg("up, selected %d", selected);
			Base::displayNeedsUpdate();
			usedInput = true;
		}
		else if((e.pushed() && e.isDefaultDownButton())
			|| (e.isRelativePointer() && e.y > 0))
		{
			if(selected == -1)
				selected = 0;
			else
				selected = wrapToBound(selected+1, 0, cells);
			logMsg("down, selected %d", selected);
			Base::displayNeedsUpdate();
			usedInput = true;
		}
		else if(e.pushed() && e.isDefaultConfirmButton())
		{
			if(selected != -1)
			{
				logDMsg("entry %d pushed", selected);
				selectedIsActivated = 1;
				src->onSelectElement(this, e, selected);
				Base::displayNeedsUpdate();
				usedInput = true;
			}
		}
		else if(e.pushed() && e.isDefaultPageUpButton())
		{
			if(selected == -1)
				selected = cells - 1;
			else
				selected = clipToBounds(selected-visibleCells(), 0, cells-1);
			logMsg("selected %d", selected);
			Base::displayNeedsUpdate();
			usedInput = true;
		}
		else if(e.pushed() && e.isDefaultPageDownButton())
		{
			if(selected == -1)
				selected = 0;
			else
				selected = clipToBounds(selected+visibleCells(), 0, cells-1);
			logMsg("selected %d", selected);
			Base::displayNeedsUpdate();
			usedInput = true;
		}
	}

	return usedInput;
}

int GuiTable1D::visibleCells() const
{
	int visYCells = ceil(Gfx::proj.h/Gfx::iYSize(yCellSize)) + 1;
	if(offscreenCells() < 0)
		visYCells += offscreenCells();
	visYCells = IG::clipToBounds(visYCells, 0, cells);
	return visYCells;
}

int GuiTable1D::offscreenCells() const
{
	auto y = viewRect.yPos(LT2DO);
	return (Gfx::iYPos(y) - Gfx::proj.hHalf())/Gfx::iYSize(yCellSize);
}

void GuiTable1D::draw()
{
	using namespace Gfx;
	if(cells == 0)
		return;
	auto y = viewRect.yPos(LT2DO);
	auto x = viewRect.xPos(LT2DO);
	// TODO: fix calculations
	int visYCells = visibleCells();
	//int visYCells = ceil(gfx_viewHeight()/gfx_iYSize(yCellSize)) + 1;
	int startYCell = 0;
	int yOffScreen = offscreenCells();
	//logMsg("%d cells offscreen", yOffScreen);
	/*if(yOffScreen < 0)
		visYCells += yOffScreen;
	visYCells = clipToBounds(visYCells, 0, cells);*/
	//logMsg("%d cells visible", visYCells);
	if(yOffScreen > 0)
		startYCell += yOffScreen;
	y += yCellSize*startYCell;
	GC separatorYSize = iYSize(1);

	int yStart = y;
	int endYCell = std::min(startYCell+visYCells, cells);
	// draw separators
	resetTransforms();
	for(int i = startYCell; i < endYCell; i++)
	{
		/*if(i >= cells)
			break;*/
		if(i == selected)
		{
			//resetTransforms();
			setBlendMode(BLEND_MODE_ALPHA);
			/*if(selectedIsActivated)
				gfx_setColor(0, 0, 1., .8);
			else*/
				setColor(0, 0, 1., .5);

			Rect2<GC> d(iXPos(x), iYPos(y) - iYSize(yCellSize),
				iXPos(x) + iXSize(viewRect.xSize()), iYPos(y));
			GeomRect::draw(d);
		}
		if(i != 0)
		{
			//resetTransforms();
			setBlendMode(0);
			setColor(.5, .5, .5);
			Rect2<GC> d(iXPos(x), iYPos(y),
				iXPos(x) + iXSize(viewRect.xSize()), iYPos(y) + separatorYSize);
			GeomRect::draw(d);
		}
		y += yCellSize;
	}

	y = yStart;
	for(int i = startYCell; i < endYCell; i++)
	{
		/*logMsg("drawing entry %d %d %f %f, size %f %f", x, y,
				(float)gfx_iXPos(x), (float)gfx_iYPos(y) - gfx_iYSize(yCellSize/2),
				gfx_iXSize(viewRect.xSize()), gfx_iYSize(yCellSize));*/
		src->drawElement(this, i, iXPos(x), iYPos(y) - iYSize(yCellSize/2), iXSize(viewRect.xSize()), iYSize(yCellSize), align);

		y += yCellSize;
	}
}

Rect2<int> GuiTable1D::focusRect()
{
	if(selected >= 0)
		return Rect2<int>::createRel(viewRect.xPos(LT2DO), viewRect.yPos(LT2DO) + (yCellSize*selected), viewRect.xSize(), yCellSize);
	else
		return Rect2<int>();
}
