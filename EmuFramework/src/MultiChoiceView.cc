/*  This file is part of EmuFramework.

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

#include <imagine/util/number.h>
#include <emuframework/MultiChoiceView.hh>
#include <algorithm>

void BaseMultiChoiceView::drawElement(uint i, Gfx::GCRect rect) const
{
	using namespace Gfx;
	if((int)i == activeItem)
		setColor(0., .8, 1.);
	else
		setColor(COLOR_WHITE);
	item[i]->draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), TableView::align, projP);
}

void MultiChoiceView::freeItems()
{
	if(choiceEntryItem)
	{
		logMsg("freeing menu choices");
		mem_free(choiceEntryItem);
		choiceEntry = nullptr;
		choiceEntryItem = nullptr;
	}
}

void MultiChoiceView::allocItems(int items)
{
	freeItems();
	uint menuItemPtrArrSize  = IG::alignRoundedUp(sizeof(MenuItem*) * items, 16);
	char *storage = (char*)mem_alloc(menuItemPtrArrSize + sizeof(TextMenuItem) * items);
	assert(storage);
	choiceEntryItem = (MenuItem**)storage;
	choiceEntry = (TextMenuItem*)(storage + menuItemPtrArrSize);
	iterateTimes(items, i)
	{
		new(&choiceEntry[i]) TextMenuItem();
	}
}

void MultiChoiceView::init(uint choices, bool highlightCurrent, _2DOrigin align)
{
	init(nullptr, choices, highlightCurrent, align);
}

void MultiChoiceView::init(const char **choice, uint choices, bool highlightCurrent, _2DOrigin align)
{
	allocItems(choices);
	iterateTimes(choices, i)
	{
		if(choice)
			choiceEntry[i].init(choice[i]);
		else
			choiceEntry[i].init();
		choiceEntryItem[i] = &choiceEntry[i];
	}
	TableView::init(choiceEntryItem, choices, highlightCurrent, align);
}

void MultiChoiceView::init(MultiChoiceMenuItem &src, bool highlightCurrent, _2DOrigin align)
{
	//assert((uint)src.choices <= sizeofArray(choiceEntry));
	allocItems(src.choices);
	iterateTimes(src.choices, i)
	{
		choiceEntry[i].init(src.choiceStr[i], src.t2.face);
		choiceEntry[i].onSelect() =
			[&src, i](TextMenuItem &, View &view, const Input::Event &e)
			{
				logMsg("set choice %d", i);
				if(src.set((int)i, e, view))
				{
					view.dismiss();
				}
			};
		choiceEntryItem[i] = &choiceEntry[i];
	}
	TableView::init(choiceEntryItem, src.choices, false, align);
	activeItem = src.choice;
	if(highlightCurrent)
	{
		selected = src.choice;
	}
}

void MultiChoiceView::deinit()
{
	TableView::deinit();
	freeItems();
}

void MultiChoiceView::setItem(int idx, TextMenuItem::SelectDelegate del)
{
	assert(idx < cells());
	choiceEntry[idx].onSelect() = del;
}

void MultiChoiceView::setItem(int idx, const char *name, TextMenuItem::SelectDelegate del)
{
	assert(idx < cells());
	choiceEntry[idx].t.setString(name);
	choiceEntry[idx].onSelect() = del;
}

void MultiChoiceSelectMenuItem::init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	MultiChoiceMenuItem::init(str, choiceStr, val, max, baseVal, active, initialDisplayStr, face);
}

void MultiChoiceSelectMenuItem::init(const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	MultiChoiceMenuItem::init(choiceStr, val, max, baseVal, active, initialDisplayStr, face);
}

void MultiChoiceSelectMenuItem::select(View &parent, const Input::Event &e)
{
	auto &multiChoiceView = *new MultiChoiceView{t.str, parent.window()};
	multiChoiceView.init(*this, !e.isPointer());
	parent.pushAndShow(multiChoiceView);
}
