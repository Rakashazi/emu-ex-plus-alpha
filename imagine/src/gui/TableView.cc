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

#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/logger/logger.h>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/input/Input.hh>
#include <imagine/base/Base.hh>
#include <algorithm>
#include <imagine/util/number.h>

Gfx::GC TableView::globalXIndent = 0;

void TableView::init(MenuItem **item, uint items, bool highlightFirst, _2DOrigin align)
{
	var_selfs(item);
	cells_ = items;
	var_selfs(align);
	selected = -1;
	if((!Config::Input::POINTING_DEVICES || highlightFirst) && items)
		selected = nextSelectableElement(0);
	ScrollView::init();
	onlyScrollIfNeeded = false;
	selectedIsActivated = false;
}

void TableView::deinit()
{
	iterateTimes(cells_, i)
	{
		item[i]->deinit();
		cells_ = 0;
	}
}

void TableView::highlightFirstCell()
{
	if(!cells_)
		return;
	selected = nextSelectableElement(0);
	postDraw();
}

void TableView::draw()
{
	if(!cells_)
		return;

	using namespace Gfx;
	ScrollView::updateGfx();
	ScrollView::drawScrollContent();

	auto y = viewRect().yPos(LT2DO);
	auto x = viewRect().xPos(LT2DO);
	int startYCell = std::min(scroll.offset / yCellSize, cells_ - 1);
	int endYCell = IG::clamp(startYCell + visibleCells, 0, cells_);
	if(startYCell < 0)
	{
		// skip non-existent cells
		y += -startYCell * yCellSize;
		startYCell = 0;
	}
	y -= scroll.offset % yCellSize;

	// draw separators
	int yStart = y;
	noTexProgram.use(projP.makeTranslate());
	int selectedCellY = INT_MAX;
	setBlendMode(0);
	for(int i = startYCell; i < endYCell; i++)
	{
		if(i == selected)
		{
			selectedCellY = y;
		}
		if(i != 0)
		{
			int ySize = 1;
			if(!elementIsSelectable(i - 1))
			{
				setColor(.4, .4, .4);
				ySize = 4;
			}
			else
			{
				setColor(.2, .2, .2);
			}
			auto rect = IG::makeWindowRectRel({x, y-1}, {viewRect().xSize(), ySize});
			GeomRect::draw(rect, projP);
		}
		y += yCellSize;
	}

	// draw selected rectangle
	if(selectedCellY != INT_MAX)
	{
		setBlendMode(BLEND_MODE_ALPHA);
		setColor(.2, .71, .9, 1./3.);
		auto rect = IG::makeWindowRectRel({x, selectedCellY}, {viewRect().xSize(), yCellSize-1});
		GeomRect::draw(rect, projP);
	}

	// draw elements
	y = yStart;
	for(int i = startYCell; i < endYCell; i++)
	{
		auto rect = IG::makeWindowRectRel({x, y}, {viewRect().xSize(), yCellSize});
		drawElement(i, projP.unProjectRect(rect));
		y += yCellSize;
	}
}

void TableView::place()
{
	iterateTimes(cells_, i)
	{
		//logMsg("compile item %d", i);
		item[i]->compile(projP);
	}
	if(cells_)
	{
		setYCellSize(IG::makeEvenRoundedUp(item[0]->ySize()*2));
		visibleCells = IG::divRoundUp(viewRect().ySize(), yCellSize) + 1;
		scrollToFocusRect();
	}
	else
		visibleCells = 0;
}

void TableView::setScrollableIfNeeded(bool on)
{
	onlyScrollIfNeeded = on;
}

void TableView::scrollToFocusRect()
{
	if(selected < 0)
		return;
	int topFocus = yCellSize * selected;
	int bottomFocus = topFocus + yCellSize;
	if(topFocus < scroll.offset)
	{
		scroll.setOffset(topFocus);
	}
	else if(bottomFocus > scroll.offset + viewRect().ySize())
	{
		scroll.setOffset(bottomFocus - viewRect().ySize());
	}
}

void TableView::inputEvent(const Input::Event &e)
{
	bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
	if(handleScroll && scrollInputEvent(e))
	{
		selected = -1;
		return;
	}
	if(handleTableInput(e) && handleScroll && !e.isPointer())
	{
		scrollToFocusRect();
	}
}

void TableView::setDefaultXIndent(const Gfx::ProjectionPlane &projP)
{
	TableView::globalXIndent =
		(Config::MACHINE_IS_OUYA) ? projP.xSMMSize(4) :
		(Config::MACHINE_IS_PANDORA) ? projP.xSMMSize(2) :
		(Config::envIsAndroid || Config::envIsIOS || Config::envIsWebOS) ? /*floor*/(projP.xSMMSize(1)) :
		(Config::envIsPS3) ? /*floor*/(projP.xSMMSize(16)) :
		/*floor*/(projP.xSMMSize(4));
}

void TableView::clearSelection()
{
	selected = -1;
}

void TableView::setYCellSize(int s)
{
	yCellSize = s;
	ScrollView::setContentSize({viewRect().xSize(), cells_ * s});
}

IG::WindowRect TableView::focusRect()
{
	if(selected >= 0)
		return IG::makeWindowRectRel(viewRect().pos(LT2DO) + IG::WP{0, yCellSize*selected}, {viewRect().xSize(), yCellSize});
	else
		return {};
}

int TableView::nextSelectableElement(int start)
{
	using namespace IG;
	int elem = wrapToBound(start, 0, cells_);
	iterateTimes(cells_, i)
	{
		if(elementIsSelectable(elem))
		{
			return elem;
		}
		elem = wrapToBound(elem+1, 0, cells_);
	}
	return -1;
}

int TableView::prevSelectableElement(int start)
{
	using namespace IG;
	int elem = wrapToBound(start, 0, cells_);
	iterateTimes(cells_, i)
	{
		if(elementIsSelectable(elem))
		{
			return elem;
		}
		elem = wrapToBound(elem-1, 0, cells_);
	}
	return -1;
}

bool TableView::handleTableInput(const Input::Event &e)
{
	using namespace IG;
	if(cells_ == 0)
		return false;

	bool movedSelected = false;

	if(e.isPointer())
	{
		if(!viewRect().overlaps({e.x, e.y}) || e.button != Input::Pointer::LBUTTON)
		{
			//logMsg("cursor not in table");
			return false;
		}

		if(e.state == Input::PUSHED)
		{
			int i = ((e.y + scroll.offset) - viewRect().y) / yCellSize;
			logMsg("input pushed on cell %d", i);
			if(i >= 0 && i < cells_ && elementIsSelectable(i))
			{
				selected = i;
				postDraw();
			}
		}
		else if(e.state == Input::RELEASED) // TODO, need to check that Input::PUSHED was sent on entry
		{
			int i = ((e.y + scroll.offset) - viewRect().y) / yCellSize;
			//logMsg("input released on cell %d", i);
			if(i >= 0 && i < cells_ && selected == i && elementIsSelectable(i))
			{
				logDMsg("entry %d pushed", i);
				selectedIsActivated = true;
				postDraw();
				selected = -1;
				onSelectElement(e, i);
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
				selected = prevSelectableElement(cells_ - 1);
			else
				selected = prevSelectableElement(selected - 1);
			logMsg("up, selected %d", selected);
			postDraw();
			movedSelected = true;
		}
		else if((e.pushed() && e.isDefaultDownButton())
			|| (e.isRelativePointer() && e.y > 0))
		{
			if(selected == -1)
				selected = nextSelectableElement(0);//0;
			else
				selected = nextSelectableElement(selected + 1);
			logMsg("down, selected %d", selected);
			postDraw();
			movedSelected = true;
		}
		else if(e.pushed() && e.isDefaultConfirmButton())
		{
			if(selected != -1)
			{
				logDMsg("entry %d pushed", selected);
				selectedIsActivated = true;
				onSelectElement(e, selected);
			}
		}
		else if(e.pushed() && e.isDefaultPageUpButton())
		{
			if(selected == -1)
				selected = cells_ - 1;
			else
				selected = clamp(selected - visibleCells, 0, cells_ - 1);
			logMsg("selected %d", selected);
			postDraw();
			movedSelected = true;
		}
		else if(e.pushed() && e.isDefaultPageDownButton())
		{
			if(selected == -1)
				selected = 0;
			else
				selected = clamp(selected + visibleCells, 0, cells_ - 1);
			logMsg("selected %d", selected);
			postDraw();
			movedSelected = true;
		}
	}

	return movedSelected;
}

void TableView::drawElement(uint i, Gfx::GCRect rect) const
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	item[i]->draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), align, projP);
}

void TableView::onSelectElement(const Input::Event &e, uint i)
{
	item[i]->select(*this, e);
}

bool TableView::elementIsSelectable(uint i)
{
	return item[i]->isSelectable;
}
