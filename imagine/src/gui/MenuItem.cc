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

#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/logger/logger.h>

void BaseTextMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	if(!active_)
	{
		// half-bright color
		uint col = color();
		setColor(ColorFormat.r(col)/2, ColorFormat.g(col)/2, ColorFormat.b(col)/2, ColorFormat.a(col));
	}

	if(ColorFormat.a(color()) == 0xFF)
	{
		//logMsg("using replace program for non-alpha modulated text");
		texAlphaReplaceProgram.use();
	}
	else
		texAlphaProgram.use();

	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += TableView::globalXIndent;
	t.draw(xPos, yPos, align, projP);
}

void BaseTextMenuItem::compile(const Gfx::ProjectionPlane &projP)
{
	t.compile(projP);
}

int BaseTextMenuItem::ySize()
{
	return t.face->nominalHeight();
}

Gfx::GC BaseTextMenuItem::xSize()
{
	return t.xSize;
}

void BaseTextMenuItem::setActive(bool on)
{
	active_ = on;
}

bool BaseTextMenuItem::active()
{
	return active_;
}

bool TextMenuItem::select(View &parent, Input::Event e)
{
	//logMsg("calling delegate");
	return selectD.callCopySafe(*this, parent, e);
}

void TextMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

bool DualTextMenuItem::select(View &parent, Input::Event e)
{
	//logMsg("calling delegate");
	selectD.callCopySafe(*this, parent, e);
	return true;
}

void DualTextMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

void BaseDualTextMenuItem::compile(const Gfx::ProjectionPlane &projP)
{
	BaseTextMenuItem::compile(projP);
	if(t2.str)
	{
		t2.compile(projP);
	}
}

void BaseDualTextMenuItem::draw2ndText(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	Gfx::texAlphaProgram.use();
	t2.draw((xPos + xSize) - TableView::globalXIndent, yPos, RC2DO, projP);
}

void BaseDualTextMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	if(t2.str)
		BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

BoolMenuItem::BoolMenuItem(const char *str, bool val, SelectDelegate selectDel):
	BaseDualTextMenuItem{str, val ? "On" : "Off"},
	selectD{selectDel},
	on{val}
{}

BoolMenuItem::BoolMenuItem(const char *str, bool val, const char *offStr, const char *onStr, SelectDelegate selectDel):
	BaseDualTextMenuItem{str, val ? onStr : offStr},
	selectD{selectDel},
	offStr{offStr},
	onStr{onStr},
	on{val},
	onOffStyle{false}
{}

bool BoolMenuItem::select(View &parent, Input::Event e)
{
	selectD.callCopySafe(*this, parent, e);
	return true;
}

bool BoolMenuItem::setBoolValue(bool val, View &view)
{
	if(val != on)
	{
		setBoolValue(val);
		t2.compile(view.projection());
		view.postDraw();
		return true;
	}
	return false;
}

bool BoolMenuItem::setBoolValue(bool val)
{
	if(val != on)
	{
		//logMsg("setting bool: %d", val);
		on = val;
		t2.setString(val ? onStr : offStr);
		return true;
	}
	return false;
}

bool BoolMenuItem::boolValue() const
{
	return on;
}

bool BoolMenuItem::flipBoolValue(View &view)
{
	setBoolValue(on ^ true, view);
	return on;
}

bool BoolMenuItem::flipBoolValue()
{
	setBoolValue(on ^ true);
	return on;
}

void BoolMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	if(!onOffStyle) // custom strings
		setColor(0., .8, 1.);
	else if(on)
		setColor(.27, 1., .27);
	else
		setColor(1., .27, .27);
	draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

void BoolMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

class MenuItemTableView : public TableView
{
public:
	int activeItem;
	MultiChoiceMenuItem &src;

	MenuItemTableView(const char *name, Base::Window &win, int active, ItemsDelegate items, ItemDelegate item, MultiChoiceMenuItem &src):
		TableView{name, win, items, item},
		activeItem{active},
		src{src}
	{}

	void onAddedToController(Input::Event e) override
	{
		if(!e.isPointer())
		{
			selected = activeItem;
		}
	}

	void drawElement(uint i, MenuItem &item, Gfx::GCRect rect) const override
	{
		using namespace Gfx;
		if((int)i == activeItem)
			setColor(0., .8, 1.);
		else
			setColor(COLOR_WHITE);
		item.draw(rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), TableView::align, projP);
	}

	void onSelectElement(Input::Event e, uint i, MenuItem &item) override
	{
		if(item.select(*this, e))
		{
			src.setSelected(i, *this);
			dismiss();
		}
	}
};

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, const char *displayStr,
	uint selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
	BaseDualTextMenuItem
	{
		str,
		displayStr
	},
	selected_{selected},
	selectD
	{
		selectDel ? selectDel :
			[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				item.defaultOnSelect(view, e);
			}
	},
	items_{items},
	item_{item}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, uint selected,
	ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
	MultiChoiceMenuItem{str, nullptr, selected, items, item, selectDel}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, const char *displayStr,
	uint selected, ItemsDelegate items, ItemDelegate item):
	MultiChoiceMenuItem{str, displayStr, selected, items, item, {}}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, uint selected,
	ItemsDelegate items, ItemDelegate item):
	MultiChoiceMenuItem{str, nullptr, selected, items, item, {}}
{}

void MultiChoiceMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	//setColor(0., 1., 1.); // aqua
	setColor(0., .8, 1.);
	BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

void MultiChoiceMenuItem::compile(const Gfx::ProjectionPlane &projP)
{
	if(selected_ < items_(*this))
	{
		setDisplayString(item_(*this, selected_).t.str);
	}
	BaseDualTextMenuItem::compile(projP);
}

uint MultiChoiceMenuItem::selected() const
{
	return selected_;
}

uint MultiChoiceMenuItem::items() const
{
	return items_(*this);
}

bool MultiChoiceMenuItem::setSelected(uint idx, View &view)
{
	bool selectChanged = setSelected(idx);
	t2.compile(view.projection());
	view.postDraw();
	return selectChanged;
}

bool MultiChoiceMenuItem::setSelected(uint idx)
{
	bool selectChanged = selected_ != idx;
	selected_ = idx;
	if(selected_ < items_(*this))
	{
		setDisplayString(item_(*this, idx).t.str);
	}
	return selectChanged;
}

void MultiChoiceMenuItem::setDisplayString(const char *str)
{
	t2.setString(str);
}

int MultiChoiceMenuItem::cycleSelected(int offset, View &view)
{
	setSelected(IG::wrapMax(selected_ + offset, items()), view);
	return selected_;
}

int MultiChoiceMenuItem::cycleSelected(int offset)
{
	setSelected(IG::wrapMax(selected_ + offset, items()));
	return selected_;
}

bool MultiChoiceMenuItem::select(View &parent, Input::Event e)
{
	//logMsg("calling delegate");
	selectD.callCopySafe(*this, parent, e);
	return true;
}

void MultiChoiceMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

TableView *MultiChoiceMenuItem::makeTableView(Base::Window &window)
{
	return new MenuItemTableView
	{
		t.str,
		window,
		selected_ < items_(*this) ? (int)selected_ : -1,
		[this](const TableView &)
		{
			return items_(*this);
		},
		[this](const TableView &, uint idx) -> MenuItem&
		{
			return item_(*this, idx);
		},
		*this
	};
}

void MultiChoiceMenuItem::defaultOnSelect(View &view, Input::Event e)
{
	auto &multiChoiceView = *makeTableView(view.window());
	view.pushAndShow(multiChoiceView, e);
}
