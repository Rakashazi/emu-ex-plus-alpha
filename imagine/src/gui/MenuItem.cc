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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/GlyphTextureSet.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/logger/logger.h>

namespace IG
{

void MenuItem::prepareDraw(Gfx::Renderer &r)
{
	t.makeGlyphs(r);
}

void MenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	if(!active())
	{
		// half-bright color
		cmds.setColor(color.r/2.f, color.g/2.f, color.b/2.f, color.a);
	}
	else
	{
		cmds.setColor(color);
	}
	cmds.basicEffect().enableAlphaTexture(cmds);
	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += xIndent;
	t.draw(cmds, {xPos, yPos}, align, projP);
}

void MenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t.compile(r, projP);
}

int MenuItem::ySize() const
{
	return t.face()->nominalHeight();
}

float MenuItem::xSize() const
{
	return t.width();
}

const Gfx::Text &MenuItem::text() const
{
	return t;
}

bool TextMenuItem::select(View &parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	return selectD.callCopySafe(*this, parent, e);
}

void TextMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

TextMenuItem::SelectDelegate TextMenuItem::onSelect() const
{
	return selectD;
}

bool TextHeadingMenuItem::select(View &parent, const Input::Event &e) { return true; };

void BaseDualTextMenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	MenuItem::compile(r, projP);
	compile2nd(r, projP);
}

void BaseDualTextMenuItem::compile2nd(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t2.compile(r, projP);
}

void BaseDualTextMenuItem::prepareDraw(Gfx::Renderer &r)
{
	MenuItem::prepareDraw(r);
	t2.makeGlyphs(r);
}

void BaseDualTextMenuItem::draw2ndText(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	cmds.basicEffect().enableAlphaTexture(cmds);
	cmds.setColor(color);
	t2.draw(cmds, {(xPos + xSize) - xIndent, yPos}, RC2DO, projP);
}

void BaseDualTextMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	MenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	BaseDualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
}

bool DualTextMenuItem::select(View &parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	selectD.callCopySafe(*this, parent, e);
	return true;
}

void DualTextMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

bool BoolMenuItem::select(View &parent, const Input::Event &e)
{
	selectD.callCopySafe(*this, parent, e);
	return true;
}

bool BoolMenuItem::setBoolValue(bool val, View &view)
{
	if(val != boolValue())
	{
		setBoolValue(val);
		t2.compile(view.renderer(), view.projection());
		view.postDraw();
		return true;
	}
	return false;
}

bool BoolMenuItem::setBoolValue(bool val)
{
	if(val != boolValue())
	{
		//logMsg("setting bool: %d", val);
		flags_ = setOrClearBits(flags_, ON_FLAG, val);
		t2.resetString(val ? onStr : offStr);
		return true;
	}
	return false;
}

bool BoolMenuItem::boolValue() const
{
	return flags_ & ON_FLAG;
}

bool BoolMenuItem::flipBoolValue(View &view)
{
	setBoolValue(boolValue() ^ true, view);
	return boolValue();
}

bool BoolMenuItem::flipBoolValue()
{
	setBoolValue(boolValue() ^ true);
	return boolValue();
}

void BoolMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	MenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	Gfx::Color color2;
	if(!(flags_ & ON_OFF_STYLE_FLAG)) // custom strings
		color2 = Gfx::color(0.f, .8f, 1.f);
	else if(boolValue())
		color2 = Gfx::color(.27f, 1.f, .27f);
	else
		color2 = Gfx::color(1.f, .27f, .27f);
	draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color2);
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

	MenuItemTableView(UTF16Convertible auto &&name, ViewAttachParams attach, int active, ItemsDelegate items, ItemDelegate item, MultiChoiceMenuItem &src):
		TableView{IG_forward(name), attach, items, item},
		activeItem{active},
		src{src}
	{
		setOnSelectElement(
			[this](const Input::Event &e, int i, MenuItem &item)
			{
				if(item.select(*this, e))
				{
					this->src.setSelected(i, *this);
					dismiss();
				}
			});
	}

	void onAddedToController(ViewController *, const Input::Event &e) final
	{
		if(e.keyEvent())
			selected = activeItem;
	}

	void drawElement(Gfx::RendererCommands &__restrict__ cmds, size_t i, MenuItem &item, Gfx::GCRect rect, float xIndent) const final
	{
		item.draw(cmds, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), xIndent, TableView::align, projP, menuTextColor((int)i == activeItem));
	}
};

void MultiChoiceMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	MenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	//auto color2 = Gfx::color(0.f, 1.f, 1.f); // aqua
	auto color2 = Gfx::color(0.f, .8f, 1.f);
	BaseDualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color2);
}

void MultiChoiceMenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	setDisplayString(selected_);
	BaseDualTextMenuItem::compile(r, projP);
}

int MultiChoiceMenuItem::selected() const
{
	return selected_;
}

size_t MultiChoiceMenuItem::items() const
{
	return items_(*this);
}

bool MultiChoiceMenuItem::setSelected(int idx, View &view)
{
	bool selectChanged = setSelected(idx);
	t2.compile(view.renderer(), view.projection());
	view.postDraw();
	return selectChanged;
}

bool MultiChoiceMenuItem::setSelected(int idx)
{
	bool selectChanged = selected_ != idx;
	selected_ = idx;
	setDisplayString(idx);
	return selectChanged;
}

bool MultiChoiceMenuItem::setSelected(Id id, View &view)
{
	return setSelected(idxOfId(id), view);
}

bool MultiChoiceMenuItem::setSelected(Id id)
{
	return setSelected(idxOfId(id));
}

void MultiChoiceMenuItem::setDisplayString(size_t idx)
{
	if(onSetDisplayString.callSafe(idx, t2))
	{
		return;
	}
	else if(idx < items_(*this))
	{
		t2.resetString(std::u16string{item_(*this, idx).text().stringView()});
	}
	else
	{
		t2.resetString();
	}
}

int MultiChoiceMenuItem::cycleSelected(int offset, View &view)
{
	setSelected(IG::wrapMax(selected_ + offset, (int)items()), view);
	return selected_;
}

int MultiChoiceMenuItem::cycleSelected(int offset)
{
	setSelected(IG::wrapMax(selected_ + offset, (int)items()));
	return selected_;
}

bool MultiChoiceMenuItem::select(View &parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	selectD.callCopySafe(*this, parent, e);
	return true;
}

void MultiChoiceMenuItem::setOnSelect(SelectDelegate onSelect)
{
	selectD = onSelect;
}

std::unique_ptr<TableView> MultiChoiceMenuItem::makeTableView(ViewAttachParams attach)
{
	return std::make_unique<MenuItemTableView>
	(
		std::u16string{t.stringView()},
		attach,
		selected_ < (int)items_(*this) ? selected_ : -1,
		[this](const TableView &)
		{
			return items_(*this);
		},
		[this](const TableView &, size_t idx) -> MenuItem&
		{
			return item_(*this, idx);
		},
		*this
	);
}

void MultiChoiceMenuItem::defaultOnSelect(View &view, const Input::Event &e)
{
	view.pushAndShow(makeTableView(view.attachParams()), e);
}

void MultiChoiceMenuItem::updateDisplayString()
{
	setDisplayString(selected_);
}

int MultiChoiceMenuItem::idxOfId(IdInt id)
{
	auto items = items_(*this);
	auto item = item_;
	Id lastId{};
	for(auto i : iotaCount(items))
	{
		lastId = item(*this, i).id();
		if(lastId == id)
			return (int)i;
	}
	if(lastId == DEFAULT_ID) // special case to simplify uses where the last menu item represents a custom value
		return items - 1;
	else
		return -1;
}

}
