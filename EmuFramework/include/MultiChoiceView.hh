#pragma once

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

#include <gui/MenuItem/MenuItem.hh>
#include <util/gui/BaseMenuView.hh>
#include <EmuSystem.hh>

void startGameFromMenu();
bool isMenuDismissKey(const Input::Event &e);

class BaseMultiChoiceView : public BaseMenuView
{
public:
	constexpr BaseMultiChoiceView() { }
	Rect2<int> viewFrame;

	void inputEvent(const Input::Event &e)
	{
		if(e.state == Input::PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				removeModalView();
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					removeModalView();
					startGameFromMenu();
					return;
				}
			}
		}

		BaseMenuView::inputEvent(e);
	}

	void place()
	{
		GC maxWidth = 0;
		iterateTimes(items, i)
		{
			item[i]->compile();
			maxWidth = IG::max(maxWidth, item[i]->xSize());
		}

		tbl.setYCellSize(item[0]->ySize()*2);

		viewFrame.setPosRel(Gfx::viewPixelWidth()/2, Gfx::viewPixelHeight()/2,
				Gfx::viewPixelWidth(), Gfx::viewPixelHeight(), C2DO);
		tbl.place(&viewFrame);
	}

	void draw(Gfx::FrameTimeBase frameTime)
	{
		using namespace Gfx;
		resetTransforms();
		setBlendMode(0);
		setColor(.2, .2, .2, 1.);
		GeomRect::draw(viewFrame);
		BaseMenuView::draw(frameTime);
	}
};

class MultiChoiceView : public BaseMultiChoiceView
{
public:
	constexpr MultiChoiceView() { }

	typedef Delegate<bool (int i, const Input::Event &e)> OnInputDelegate;
	OnInputDelegate onSelect;
	TextMenuItem choiceEntry[18];
	MenuItem *choiceEntryItem[18] = {nullptr};

	// Required delegates
	OnInputDelegate &onSelectDelegate() { return onSelect; }

	void init(const char **choice, uint choices, bool highlightCurrent)
	{
		assert(choices <= sizeofArray(choiceEntry));
		iterateTimes(choices, i)
		{
			choiceEntry[i].init(choice[i]);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, choices, highlightCurrent, C2DO);
	}

	template <size_t S, size_t S2>
	void init(const char (&choice)[S][S2], uint choices, bool highlightCurrent)
	{
		assert(choices <= sizeofArray(choiceEntry));
		iterateTimes(choices, i)
		{
			choiceEntry[i].init(choice[i]);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, choices, highlightCurrent, C2DO);
	}

	void init(MultiChoiceMenuItem *src, bool highlightCurrent)
	{
		assert((uint)src->choices <= sizeofArray(choiceEntry));
		iterateTimes(src->choices, i)
		{
			choiceEntry[i].init(src->choiceStr[i], src->t2.face);
			choiceEntryItem[i] = &choiceEntry[i];
		}
		BaseMenuView::init(choiceEntryItem, src->choices, 0, C2DO);
		if(highlightCurrent)
		{
			tbl.selected = src->choice;
		}
		onSelect.bind<MultiChoiceMenuItem, &MultiChoiceMenuItem::set>(src);
	}

	void onSelectElement(const GuiTable1D *table, const Input::Event &e, uint i)
	{
		logMsg("set choice %d", i);
		if(onSelect.invoke((int)i, e)) // TODO: Delegate should handle removeModalView()
			removeModalView();
	}
};

extern MultiChoiceView multiChoiceView;

struct MultiChoiceSelectMenuItem : public MultiChoiceMenuItem
{
	constexpr MultiChoiceSelectMenuItem() { }
	constexpr MultiChoiceSelectMenuItem(const char *str): MultiChoiceMenuItem(str) { }
	constexpr MultiChoiceSelectMenuItem(ValueDelegate valueDel): MultiChoiceMenuItem(valueDel) { }
	constexpr MultiChoiceSelectMenuItem(const char *str, ValueDelegate valueDel): MultiChoiceMenuItem(str, valueDel) { }
	void init(const char *str, const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace)
	{
		selectDel.bind<MultiChoiceSelectMenuItem, &MultiChoiceSelectMenuItem::handleChoices>(this);
		MultiChoiceMenuItem::init(str, choiceStr, val, max, baseVal, active, initialDisplayStr, face);
	}

	void init(const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace)
	{
		selectDel.bind<MultiChoiceSelectMenuItem, &MultiChoiceSelectMenuItem::handleChoices>(this);
		MultiChoiceMenuItem::init(choiceStr, val, max, baseVal, active, initialDisplayStr, face);
	}

	void handleChoices(DualTextMenuItem &, const Input::Event &e)
	{
		multiChoiceView.init(this, !e.isPointer());
		multiChoiceView.placeRect(Gfx::viewportRect());
		View::modalView = &multiChoiceView;
	}
};
