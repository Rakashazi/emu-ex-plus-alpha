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
#include <imagine/util/math.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>
#include <algorithm>

namespace IG
{

TextMenuItem &TextTableView::item(size_t idx)
{
	assert(idx < textItem.size());
	return textItem[idx];
}

void TextTableView::onAddedToController(ViewController *c, const Input::Event &e)
{
	if(activeItem != -1 && e.keyEvent())
	{
		selected = activeItem;
	}
	else TableView::onAddedToController(c, e);
}

void TextTableView::drawElement(Gfx::RendererCommands &__restrict__ cmds, size_t i, MenuItem &item, WRect rect, int xIndent) const
{
	MenuItemDrawAttrs attrs{.rect = rect, .xIndent = xIndent,
		.color = menuTextColor((int)i == activeItem), .align = TableView::align};
	item.draw(cmds, attrs);
}

}
