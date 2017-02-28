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
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/TextTableView.hh>
#include <imagine/logger/logger.h>
#include <imagine/mem/mem.h>
#include <imagine/util/math/int.hh>
#include <algorithm>

TextTableView::TextTableView(ViewAttachParams attach, uint itemsHint): TextTableView{"", attach, itemsHint} {}

TextTableView::TextTableView(const char *name, ViewAttachParams attach, uint itemsHint):
	TableView{name, attach, textItem}
{
	textItem.reserve(itemsHint);
}

void TextTableView::appendItem(const char *name, TextMenuItem::SelectDelegate del)
{
	textItem.emplace_back(name, del);
}

void TextTableView::setItem(uint idx, const char *name, TextMenuItem::SelectDelegate del)
{
	assert(idx < textItem.size());
	textItem[idx] = {name, del};
}

TextMenuItem &TextTableView::item(uint idx)
{
	assert(idx < textItem.size());
	return textItem[idx];
}

void TextTableView::setItems(uint items)
{
	textItem.resize(items);
}

void TextTableView::onAddedToController(Input::Event e)
{
	if(!e.isPointer())
	{
		selected = activeItem;
	}
}

void TextTableView::drawElement(Gfx::Renderer &r, uint i, MenuItem &item, Gfx::GCRect rect) const
{
	using namespace Gfx;
	if((int)i == activeItem)
		r.setColor(0., .8, 1.);
	else
		r.setColor(COLOR_WHITE);
	item.draw(r, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), TableView::align, projP);
}

