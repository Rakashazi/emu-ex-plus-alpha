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
#include <imagine/util/math/int.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include <algorithm>

TextTableView::TextTableView(ViewAttachParams attach, unsigned itemsHint): TextTableView{{}, attach, itemsHint} {}

void TextTableView::appendItem(IG::utf16String name, TextMenuItem::SelectDelegate del)
{
	textItem.emplace_back(std::move(name), &defaultFace(), del);
}

void TextTableView::setItem(size_t idx, IG::utf16String name, TextMenuItem::SelectDelegate del)
{
	assert(idx < textItem.size());
	textItem[idx] = {std::move(name), &defaultFace(), del};
}

TextMenuItem &TextTableView::item(size_t idx)
{
	assert(idx < textItem.size());
	return textItem[idx];
}

void TextTableView::setItems(size_t items)
{
	textItem.resize(items);
}

void TextTableView::onAddedToController(ViewController *c, Input::Event e)
{
	if(activeItem != -1 && !e.isPointer())
	{
		selected = activeItem;
	}
	else TableView::onAddedToController(c, e);
}

void TextTableView::drawElement(Gfx::RendererCommands &cmds, size_t i, MenuItem &item, Gfx::GCRect rect, Gfx::GC xIndent) const
{
	item.draw(cmds, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(),
		xIndent, TableView::align, projP, menuTextColor((int)i == activeItem));
}

