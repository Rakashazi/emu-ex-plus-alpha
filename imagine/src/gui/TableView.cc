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

#define LOGTAG "TableView"

#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gfx/GeomRect.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/input/Input.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/math/int.hh>
#include <imagine/logger/logger.h>

TableView::TableView(ViewAttachParams attach, ItemsDelegate items, ItemDelegate item):
	ScrollView{attach}, items{items}, item{item}
{}

TableView::TableView(NameString name, ViewAttachParams attach, ItemsDelegate items, ItemDelegate item):
	ScrollView{std::move(name), attach}, items{items}, item{item}
{}

uint32_t TableView::cells() const
{
	return items(*this);
}

IG::WP TableView::cellSize() const
{
	return {viewRect().x, yCellSize};
}

void TableView::highlightCell(int idx)
{
	auto cells_ = items(*this);
	if(!cells_)
		return;
	if(idx >= 0)
		selected = nextSelectableElement(idx, cells_);
	else
		selected = -1;
	postDraw();
}

void TableView::setAlign(_2DOrigin align)
{
	this->align = align;
}

void TableView::prepareDraw()
{
	if(!yCellSize)
		return;
	auto cells_ = items(*this);
	if(!cells_)
		return;
	int startYCell = std::min(scrollOffset() / yCellSize, cells_);
	int endYCell = std::clamp(startYCell + visibleCells, 0, cells_);
	if(startYCell < 0)
	{
		// skip non-existent cells
		startYCell = 0;
	}
	for(int i = startYCell; i < endYCell; i++)
	{
		item(*this, i).prepareDraw(renderer());
	}
}

void TableView::draw(Gfx::RendererCommands &cmds)
{
	auto cells_ = items(*this);
	if(!cells_)
		return;
	using namespace Gfx;
	auto y = viewRect().yPos(LT2DO);
	auto x = viewRect().xPos(LT2DO);
	int startYCell = std::min(scrollOffset() / yCellSize, cells_);
	int endYCell = std::clamp(startYCell + visibleCells, 0, cells_);
	if(startYCell < 0)
	{
		// skip non-existent cells
		y += -startYCell * yCellSize;
		startYCell = 0;
	}
	//logMsg("draw cells [%d,%d)", startYCell, endYCell);
	y -= scrollOffset() % yCellSize;

	// draw separators
	int yStart = y;
	cmds.setCommonProgram(CommonProgram::NO_TEX, projP.makeTranslate());
	int selectedCellY = INT_MAX;
	{
		StaticArrayList<std::array<ColVertex, 4>, 80> vRect;
		StaticArrayList<std::array<VertexIndex, 6>, vRect.maxSize()> vRectIdx;
		auto headingColor = VertexColorPixelFormat.build(.4, .4, .4, 1.);
		auto regularColor = VertexColorPixelFormat.build(.2, .2, .2, 1.);
		auto regularYSize = std::max(1, window().heightSMMInPixels(.2));
		auto headingYSize = std::max(2, window().heightSMMInPixels(.4));
		for(int i = startYCell; i < endYCell; i++)
		{
			if(i == selected)
			{
				selectedCellY = y;
			}
			if(i != 0)
			{
				int ySize = regularYSize;
				auto color = regularColor;
				if(!elementIsSelectable(item(*this, i - 1)))
				{
					ySize = headingYSize;
					color = headingColor;
				}
				vRectIdx.emplace_back(makeRectIndexArray(vRect.size()));
				auto rect = IG::makeWindowRectRel({x, y-1}, {viewRect().xSize(), ySize});
				vRect.emplace_back(makeColVertArray(projP.unProjectRect(rect), color));
			}
			y += yCellSize;
			if(vRect.size() == vRect.maxSize()) [[unlikely]]
				break;
		}
		if(vRect.size())
		{
			cmds.setBlendMode(0);
			cmds.set(ColorName::WHITE);
			drawQuads(cmds, &vRect[0], vRect.size(), &vRectIdx[0], vRectIdx.size());
		}
	}

	// draw scroll bar
	ScrollView::drawScrollContent(cmds);

	// draw selected rectangle
	if(selectedCellY != INT_MAX)
	{
		cmds.setBlendMode(BLEND_MODE_ALPHA);
		if(hasFocus)
			cmds.setColor(.2, .71, .9, 1./3.);
		else
			cmds.setColor(.2 / 3., .71 / 3., .9 / 3., 1./3.);
		auto rect = IG::makeWindowRectRel({x, selectedCellY}, {viewRect().xSize(), yCellSize-1});
		GeomRect::draw(cmds, rect, projP);
	}

	// draw elements
	y = yStart;
	auto xIndent = manager().tableXIndent();
	for(int i = startYCell; i < endYCell; i++)
	{
		auto rect = IG::makeWindowRectRel({x, y}, {viewRect().xSize(), yCellSize});
		drawElement(cmds, i, item(*this, i), projP.unProjectRect(rect), xIndent);
		y += yCellSize;
	}
}

void TableView::place()
{
	auto cells_ = items(*this);
	iterateTimes(cells_, i)
	{
		//logMsg("compile item %d", i);
		item(*this, i).compile(renderer(), projP);
	}
	if(cells_)
	{
		setYCellSize(IG::makeEvenRoundedUp(item(*this, 0).ySize()*2));
		visibleCells = IG::divRoundUp(viewRect().ySize(), yCellSize) + 1;
		scrollToFocusRect();
	}
	else
		visibleCells = 0;
}

void TableView::onShow()
{
	ScrollView::onShow();
}

void TableView::onHide()
{
	ScrollView::onHide();
}

void TableView::onAddedToController(ViewController *, Input::Event e)
{
	if((!Config::Input::POINTING_DEVICES || !e.isPointer()))
	{
		auto cells = items(*this);
		if(!cells)
			return;
		selected = nextSelectableElement(0, cells);
	}
}

void TableView::setFocus(bool focused)
{
	hasFocus = focused;
}

void TableView::setOnSelectElement(SelectElementDelegate del)
{
	selectElementDel = del;
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
	if(topFocus < scrollOffset())
	{
		setScrollOffset(topFocus);
	}
	else if(bottomFocus > scrollOffset() + viewRect().ySize())
	{
		setScrollOffset(bottomFocus - viewRect().ySize());
	}
}

void TableView::resetScroll()
{
	setScrollOffset(0);
}

bool TableView::inputEvent(Input::Event e)
{
	bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
	if(handleScroll && scrollInputEvent(e))
	{
		selected = -1;
		return true;
	}
	bool movedSelected = false;
	if(handleTableInput(e, movedSelected))
	{
		if(movedSelected && handleScroll && !e.isPointer())
		{
			scrollToFocusRect();
		}
		return true;
	}
	return false;
}

void TableView::clearSelection()
{
	selected = -1;
}

void TableView::setYCellSize(int s)
{
	yCellSize = s;
	ScrollView::setContentSize({viewRect().xSize(), items(*this) * s});
}

IG::WindowRect TableView::focusRect()
{
	if(selected >= 0)
		return IG::makeWindowRectRel(viewRect().pos(LT2DO) + IG::WP{0, yCellSize*selected}, {viewRect().xSize(), yCellSize});
	else
		return {};
}

int TableView::nextSelectableElement(int start, int items)
{
	using namespace IG;
	int elem = wrapMinMax(start, 0, items);
	iterateTimes(items, i)
	{
		if(elementIsSelectable(item(*this, elem)))
		{
			return elem;
		}
		elem = wrapMinMax(elem+1, 0, items);
	}
	return -1;
}

int TableView::prevSelectableElement(int start, int items)
{
	using namespace IG;
	int elem = wrapMinMax(start, 0, items);
	iterateTimes(items, i)
	{
		if(elementIsSelectable(item(*this, elem)))
		{
			return elem;
		}
		elem = wrapMinMax(elem-1, 0, items);
	}
	return -1;
}

bool TableView::handleTableInput(Input::Event e, bool &movedSelected)
{
	using namespace IG;
	auto cells_ = items(*this);
	if(!cells_)
	{
		if(e.pushed() && e.isDefaultUpButton() && moveFocusToNextView(e, CT2DO))
		{
			return true;
		}
		else if(e.pushed() && e.isDefaultDownButton() && moveFocusToNextView(e, CB2DO))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	if(e.isPointer())
	{
		if(!pointIsInView(e.pos()) || e.mapKey() != Input::Pointer::LBUTTON)
		{
			//logMsg("cursor not in table");
			return false;
		}
		int i = ((e.pos().y + scrollOffset()) - viewRect().y) / yCellSize;
		if(i < 0 || i >= cells_)
		{
			//logMsg("pushed outside of item bounds");
			return false;
		}
		auto &it = item(*this, i);
		if(e.pushed())
		{
			logMsg("input pushed on cell %d", i);
			hasFocus = true;
			if(i >= 0 && i < cells_ && elementIsSelectable(it))
			{
				selected = i;
				postDraw();
			}
		}
		else if(e.isOff()) // TODO, need to check that Input::PUSHED was sent on entry
		{
			//logMsg("input released on cell %d", i);
			if(i >= 0 && i < cells_ && selected == i && elementIsSelectable(it))
			{
				postDraw();
				selected = -1;
				if(!e.canceled())
				{
					logDMsg("entry %d pushed", i);
					selectedIsActivated = true;
					onSelectElement(e, i, it);
				}
			}
		}
		return true;
	}
	else // Key or Relative Pointer
	{
		if(e.isRelativePointer())
			logMsg("got rel pointer %d", e.pos().y);

		if((e.pushed() && e.isDefaultUpButton())
			|| (e.isRelativePointer() && e.pos().y < 0))
		{
			if(!hasFocus)
			{
				logMsg("gained focus from key input");
				hasFocus = true;
				if(selected != -1)
				{
					postDraw();
					return true;
				}
			}
			//logMsg("move up %d", selected);
			if(selected == -1)
				selected = prevSelectableElement(cells_ - 1, cells_);
			else
			{
				auto prevSelected = selected;
				selected = prevSelectableElement(selected - 1, cells_);
				if(selected > prevSelected || cells_ == 1)
				{
					if(e.repeated())
					{
						selected = prevSelected;
						return true;
					}
					else if(moveFocusToNextView(e, CT2DO))
					{
						selected = -1;
						return true;
					}
				}
			}
			logMsg("up, selected %d", selected);
			postDraw();
			movedSelected = true;
			return true;
		}
		else if((e.pushed() && e.isDefaultDownButton())
			|| (e.isRelativePointer() && e.pos().y > 0))
		{
			if(!hasFocus)
			{
				logMsg("gained focus from key input");
				hasFocus = true;
				if(selected != -1)
				{
					postDraw();
					return true;
				}
			}
			if(selected == -1)
				selected = nextSelectableElement(0, cells_);
			else
			{
				auto prevSelected = selected;
				selected = nextSelectableElement(selected + 1, cells_);
				if(selected < prevSelected || cells_ == 1)
				{
					if(e.repeated())
					{
						selected = prevSelected;
						return true;
					}
					else if(moveFocusToNextView(e, CB2DO))
					{
						selected = -1;
						return true;
					}
				}
			}
			logMsg("down, selected %d", selected);
			postDraw();
			movedSelected = true;
			return true;
		}
		else if(e.pushed() && e.isDefaultConfirmButton())
		{
			if(selected != -1)
			{
				logDMsg("entry %d pushed", selected);
				selectedIsActivated = true;
				onSelectElement(e, selected, item(*this, selected));
			}
			return true;
		}
		else if(e.pushed() && e.isDefaultPageUpButton())
		{
			if(selected == -1)
				selected = cells_ - 1;
			else
				selected = std::clamp(selected - visibleCells, 0, cells_ - 1);
			logMsg("selected %d", selected);
			postDraw();
			movedSelected = true;
			return true;
		}
		else if(e.pushed() && e.isDefaultPageDownButton())
		{
			if(selected == -1)
				selected = 0;
			else
				selected = std::clamp(selected + visibleCells, 0, cells_ - 1);
			logMsg("selected %d", selected);
			postDraw();
			movedSelected = true;
			return true;
		}
	}
	return false;
}

void TableView::drawElement(Gfx::RendererCommands &cmds, uint32_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const
{
	item.draw(cmds, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), xIndent, align, projP, Gfx::color(Gfx::ColorName::WHITE));
}

void TableView::onSelectElement(Input::Event e, uint32_t i, MenuItem &item)
{
	if(selectElementDel)
		selectElementDel(e, i, item);
	else
		item.select(*this, e);
}

bool TableView::elementIsSelectable(MenuItem &item)
{
	return item.isSelectable;
}
