#pragma once

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

#include <gui/MenuItem/MenuItem.hh>
#include <gui/BaseMenuView.hh>

class BaseMultiChoiceView : public BaseMenuView
{
public:
	int activeItem = -1;

	constexpr BaseMultiChoiceView(Base::Window &win): BaseMenuView(win) {}
	constexpr BaseMultiChoiceView(const char *name, Base::Window &win): BaseMenuView(name, win) {}
	void draw(Base::FrameTimeBase frameTime) override;
	void drawElement(const GuiTable1D &table, uint i, Gfx::GCRect rect) const override;
};

class MultiChoiceView : public BaseMultiChoiceView
{
public:
	typedef DelegateFunc<bool (int i, const Input::Event &e)> OnInputDelegate;
	OnInputDelegate onSelectD;
	TextMenuItem *choiceEntry = nullptr;
	MenuItem **choiceEntryItem = nullptr;

	// Required delegates
	OnInputDelegate &onSelect() { return onSelectD; }

	constexpr MultiChoiceView(Base::Window &win): BaseMultiChoiceView(win) {}
	constexpr MultiChoiceView(const char *name, Base::Window &win): BaseMultiChoiceView(name, win) {}
	void freeItems();
	void allocItems(int items);
	void init(const char **choice, uint choices, bool highlightCurrent, _2DOrigin align = LC2DO);
	void init(MultiChoiceMenuItem &src, bool highlightCurrent, _2DOrigin align = LC2DO);
	void deinit() override;
	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i) override;

	template <size_t S, size_t S2>
	void init(const char (&choice)[S][S2], uint choices, bool highlightCurrent, _2DOrigin align = LC2DO)
	{
		//assert(choices <= sizeofArray(choiceEntry));
		allocItems(choices);
		iterateTimes(choices, i)
		{
			choiceEntry[i].init(choice[i]);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, choices, highlightCurrent, align);
	}
};

struct MultiChoiceSelectMenuItem : public MultiChoiceMenuItem
{
	constexpr MultiChoiceSelectMenuItem() {}
	constexpr MultiChoiceSelectMenuItem(const char *str): MultiChoiceMenuItem(str) {}
	constexpr MultiChoiceSelectMenuItem(ValueDelegate valueDel): MultiChoiceMenuItem(valueDel) {}
	constexpr MultiChoiceSelectMenuItem(const char *str, ValueDelegate valueDel): MultiChoiceMenuItem(str, valueDel) {}
	void init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face);
	void init(const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face);
	void handleChoices(DualTextMenuItem &, const Input::Event &e);
	void select(View *parent, const Input::Event &e) override;

	void init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr)
	{
		init(str, choiceStr, val, max, baseVal, active, initialDisplayStr, View::defaultFace);
	}

	void init(const char *str, const char **choiceStr, int val, int max)
	{
		init(str, choiceStr, val, max, 0, true, nullptr, View::defaultFace);
	}

	void init(const char **choiceStr, int val, int max)
	{
		init(choiceStr, val, max, 0, true, nullptr, View::defaultFace);
	}

	void init(const char **choiceStr, int val, int max, int baseVal)
	{
		init(choiceStr, val, max, baseVal, true, nullptr, View::defaultFace);
	}

	template <size_t S>
	void init(const char *(&choiceStrArr)[S], int val)
	{
		init(choiceStrArr, val, S, 0, true, nullptr, View::defaultFace);
	}
};
