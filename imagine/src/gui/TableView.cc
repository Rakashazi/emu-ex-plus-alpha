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

#include <imagine/gui/TableView.hh>
#include <imagine/gui/ViewManager.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/input/Event.hh>
#include <imagine/base/Window.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/variant.hh>
#include <imagine/util/math.hh>
#include <imagine/logger/logger.h>

namespace IG
{

constexpr SystemLogger log{"TableView"};

size_t TableView::cells() const
{
	return getAs<size_t>(itemSrc(ItemsMessage{*this}));
}

WSize TableView::cellSize() const
{
	return {viewRect().x, yCellSize};
}

void TableView::highlightCell(int idx)
{
	auto cells_ = cells();
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
	auto src = itemSrc;
	for(auto i : iotaCount(cells()))
	{
		item(src, i).prepareDraw();
	}
}

void TableView::draw(Gfx::RendererCommands &__restrict__ cmds, ViewDrawParams) const
{
	ssize_t cells_ = cells();
	if(!cells_)
		return;
	auto src = itemSrc;
	using namespace IG::Gfx;
	auto visibleRect = viewRect() + WindowRect{{}, {0, displayRect().y2 - viewRect().y2}};
	cmds.setClipRect(renderer().makeClipRect(window(), visibleRect));
	cmds.setClipTest(true);
	auto y = viewRect().yPos(LT2DO);
	auto x = viewRect().xPos(LT2DO);
	ssize_t startYCell = std::min(ssize_t(scrollOffset() / yCellSize), cells_);
	ssize_t endYCell = std::clamp(startYCell + visibleCells, 0z, cells_);
	if(startYCell < 0)
	{
		// skip non-existent cells
		y += -startYCell * yCellSize;
		startYCell = 0;
	}
	//log.debug("draw cells:[{},{})", startYCell, endYCell);
	y -= scrollOffset() % yCellSize;

	// draw separators
	int yStart = y;
	cmds.basicEffect().disableTexture(cmds);
	int selectedCellY = selected == 0 ? y : INT_MAX;
	if(cells_ > 1)
	{
		cmds.basicEffect().setModelView(cmds, Gfx::Mat4::ident());
		cmds.set(BlendMode::OFF);
		cmds.setColor(ColorName::WHITE);
		cmds.setVertexArray(separatorQuads);
		cmds.setVertexBuffer(separatorQuads);
		StaticArrayList<IColQuad, maxSeparators> vRect;
		const auto headingColor = PackedColor::format.build(.4, .4, .4, 1.);
		const auto regularColor = PackedColor::format.build(.2, .2, .2, 1.);
		auto regularYSize = std::max(1, window().heightMMInPixels(.2));
		auto headingYSize = std::max(2, window().heightMMInPixels(.4));
		auto drawSeparators = [](Gfx::RendererCommands &__restrict__ cmds, const auto &vRect)
		{
			cmds.vertexBufferData(0, vRect.data(), vRect.size() * sizeof(IColQuad));
			cmds.drawQuads(0, vRect.size());
		};
		for(ssize_t i = startYCell; i < endYCell; i++)
		{
			if(i == selected)
			{
				selectedCellY = y;
			}
			if(i != 0)
			{
				int ySize = regularYSize;
				auto color = regularColor;
				if(!item(src, i - 1).selectable())
				{
					ySize = headingYSize;
					color = headingColor;
				}
				auto rect = makeWindowRectRel({x, y-1}, {viewRect().xSize(), ySize}).as<int16_t>();
				vRect.emplace_back(IColQuad::InitParams{.bounds = rect, .color = color});
			}
			y += yCellSize;
			if(vRect.size() == vRect.capacity())
			{
				drawSeparators(cmds, vRect);
				vRect.clear();
			}
		}
		if(vRect.size())
		{
			drawSeparators(cmds, vRect);
		}
	}

	// draw scroll bar
	ScrollView::drawScrollContent(cmds);

	// draw selected rectangle
	if(selectedCellY != INT_MAX)
	{
		cmds.set(BlendMode::ALPHA);
		if(hasFocus)
			cmds.setColor({.2, .71, .9, 1./3.});
		else
			cmds.setColor({.2 / 3., .71 / 3., .9 / 3., 1./3.});
		cmds.basicEffect().setModelView(cmds, Mat4::makeTranslate({x, selectedCellY}));
		cmds.drawQuad(selectQuads, 0);
	}

	// draw elements
	y = yStart;
	auto xIndent = manager().tableXIndentPx;
	for(ssize_t i = startYCell; i < endYCell; i++)
	{
		auto rect = IG::makeWindowRectRel({x, y}, {viewRect().xSize(), yCellSize});
		drawElement(cmds, i, item(src, i), rect, xIndent);
		y += yCellSize;
	}
	cmds.setClipTest(false);
}

void TableView::place()
{
	auto cells_ = cells();
	auto src = itemSrc;
	for(auto i : iotaCount(cells_))
	{
		//log.debug("place item:{}", i);
		item(src, i).place();
	}
	if(cells_)
	{
		setYCellSize(IG::makeEvenRoundedUp(item(0).ySize()*2));
		visibleCells = IG::divRoundUp(displayRect().ySize(), yCellSize) + 1;
		scrollToFocusRect();
		selectQuads.write(0, {.bounds = WRect{{}, {viewRect().xSize(), yCellSize-1}}.as<int16_t>()});
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

void TableView::onAddedToController(ViewController *, const Input::Event &e)
{
	if(e.keyEvent())
	{
		auto cells_ = cells();
		if(!cells_)
			return;
		selected = nextSelectableElement(0, cells_);
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

bool TableView::inputEvent(const Input::Event& e, ViewInputEventParams)
{
	bool handleScroll = !onlyScrollIfNeeded || contentIsBiggerThanView;
	auto motionEv = e.motionEvent();
	if(motionEv && handleScroll && scrollInputEvent(*motionEv))
	{
		selected = -1;
		return true;
	}
	bool movedSelected = false;
	if(handleTableInput(e, movedSelected))
	{
		if(movedSelected && handleScroll && !motionEv)
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
	ScrollView::setContentSize({viewRect().xSize(), (int)cells() * s});
}

IG::WindowRect TableView::focusRect()
{
	if(selected >= 0)
		return makeWindowRectRel(viewRect().pos(LT2DO) + WPt{0, yCellSize*selected}, {viewRect().xSize(), yCellSize});
	else
		return {};
}

int TableView::nextSelectableElement(int start, int items)
{
	int elem = wrapMinMax(start, 0, items);
	auto src = itemSrc;
	for([[maybe_unused]] auto i : iotaCount(items))
	{
		if(item(src, elem).selectable())
		{
			return elem;
		}
		elem = wrapMinMax(elem+1, 0, items);
	}
	return -1;
}

int TableView::prevSelectableElement(int start, int items)
{
	int elem = wrapMinMax(start, 0, items);
	auto src = itemSrc;
	for([[maybe_unused]] auto i : iotaCount(items))
	{
		if(item(src, elem).selectable())
		{
			return elem;
		}
		elem = wrapMinMax(elem-1, 0, items);
	}
	return -1;
}

bool TableView::handleTableInput(const Input::Event &e, bool &movedSelected)
{
	ssize_t cells_ = cells();
	return e.visit(overloaded
	{
		[&](const Input::KeyEvent &keyEv)
		{
			if(!cells_)
			{
				if(keyEv.pushed(Input::DefaultKey::UP) && moveFocusToNextView(e, CT2DO))
				{
					return true;
				}
				else if(keyEv.pushed(Input::DefaultKey::DOWN) && moveFocusToNextView(e, CB2DO))
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			if(keyEv.pushed(Input::DefaultKey::UP))
			{
				if(!hasFocus)
				{
					log.info("gained focus from key input");
					hasFocus = true;
					if(selected != -1)
					{
						postDraw();
						return true;
					}
				}
				//log.debug("move up:{}", selected);
				if(selected == -1)
					selected = prevSelectableElement(cells_ - 1, cells_);
				else
				{
					auto prevSelected = selected;
					selected = prevSelectableElement(selected - 1, cells_);
					if(selected > prevSelected || cells_ == 1)
					{
						if(keyEv.repeated())
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
				log.info("up, selected:{}", selected);
				postDraw();
				movedSelected = true;
				return true;
			}
			else if(keyEv.pushed(Input::DefaultKey::DOWN))
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
						if(keyEv.repeated())
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
				log.info("down, selected:{}", selected);
				postDraw();
				movedSelected = true;
				return true;
			}
			else if(keyEv.pushed(Input::DefaultKey::CONFIRM))
			{
				if(selected != -1)
				{
					//log.debug("entry:{} pushed", selected);
					selectedIsActivated = true;
					onSelectElement(e, selected, item(selected));
				}
				return true;
			}
			else if(keyEv.pushed(Input::DefaultKey::PAGE_UP))
			{
				if(selected == -1)
					selected = cells_ - 1;
				else
					selected = std::clamp(selected - visibleCells, 0, (int)cells_ - 1);
				log.info("selected:{}", selected);
				postDraw();
				movedSelected = true;
				return true;
			}
			else if(keyEv.pushed(Input::DefaultKey::PAGE_DOWN))
			{
				if(selected == -1)
					selected = 0;
				else
					selected = std::clamp(selected + visibleCells, 0, (int)cells_ - 1);
				log.info("selected:{}", selected);
				postDraw();
				movedSelected = true;
				return true;
			}
			return false;
		},
		[&](const Input::MotionEvent &motionEv)
		{
			if(!cells_ || !motionEv.isPointer())
				return false;
			if(!pointIsInView(motionEv.pos()) || !(motionEv.key() & Input::Pointer::LBUTTON))
			{
				//log.info("cursor not in table");
				return false;
			}
			int i = ((motionEv.pos().y + scrollOffset()) - viewRect().y) / yCellSize;
			if(i < 0 || i >= cells_)
			{
				//logMsg("pushed outside of item bounds");
				return false;
			}
			auto &it = item(i);
			if(motionEv.pushed())
			{
				//log.info("input pushed on cell:{}", i);
				hasFocus = true;
				if(i >= 0 && i < cells_ && it.selectable())
				{
					selected = i;
					postDraw();
				}
			}
			else if(motionEv.isOff()) // TODO, need to check that Input::PUSHED was sent on entry
			{
				//log.info("input released on cell:{}", i);
				if(i >= 0 && i < cells_ && selected == i && it.selectable())
				{
					postDraw();
					selected = -1;
					if(!motionEv.canceled())
					{
						//log.debug("entry:{} pushed", i);
						selectedIsActivated = true;
						onSelectElement(e, i, it);
					}
				}
			}
			return true;
		}
	});
}

void TableView::drawElement(Gfx::RendererCommands &__restrict__ cmds, size_t, MenuItem &item, WRect rect, int xIndent) const
{
	static constexpr Gfx::Color highlightColor{0.f, .8f, 1.f};
	MenuItemDrawAttrs attrs{.rect = rect, .xIndent = xIndent,
		.color = item.highlighted() ? highlightColor : Gfx::Color(Gfx::ColorName::WHITE), .align = align};
	item.draw(cmds, attrs);
}

void TableView::onSelectElement(const Input::Event &e, size_t i, MenuItem &item)
{
	if(selectElementDel)
		selectElementDel(e, i, item);
	else
		item.inputEvent(e, {.parentPtr = this});
}

std::u16string_view TableView::name() const
{
	return nameStr;
}

TableUIState TableView::saveUIState() const
{
	return {.highlightedCell = highlightedCell(), .scrollOffset = scrollOffset()};
}

void TableView::restoreUIState(TableUIState state)
{
	if(state.highlightedCell != -1)
		highlightCell(state.highlightedCell);
	setScrollOffset(state.scrollOffset);
}

}
