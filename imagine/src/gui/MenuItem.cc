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
#include <imagine/logger/logger.h>

namespace IG
{

void BaseTextMenuItem::prepareDraw(Gfx::Renderer &r)
{
	t.makeGlyphs(r);
}

void BaseTextMenuItem::draw(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	if(!active_)
	{
		// half-bright color
		cmds.setColor(color[0]/2.f, color[1]/2.f, color[2]/2.f, color[3]);
	}
	else
	{
		cmds.setColor(color);
	}
	cmds.setCommonProgram(Gfx::CommonProgram::TEX_ALPHA);
	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += xIndent;
	t.draw(cmds, xPos, yPos, align, projP);
}

void BaseTextMenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t.compile(r, projP);
}

void BaseTextMenuItem::compile(IG::utf16String name, Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t.setString(std::move(name));
	compile(r, projP);
}

void BaseTextMenuItem::setName(IG::utf16String name, Gfx::GlyphTextureSet *face)
{
	t.setString(std::move(name));
	if(face)
	{
		t.setFace(face);
	}
}

int BaseTextMenuItem::ySize()
{
	return t.face()->nominalHeight();
}

float BaseTextMenuItem::xSize()
{
	return t.width();
}

const Gfx::Text &BaseTextMenuItem::text() const
{
	return t;
}

void BaseTextMenuItem::setActive(bool on)
{
	active_ = on;
}

bool BaseTextMenuItem::active()
{
	return active_;
}

TextMenuItem::TextMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SelectDelegate selectDel):
	BaseTextMenuItem{std::move(name), face},
	selectD{selectDel} {}

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

BaseDualTextMenuItem::BaseDualTextMenuItem(IG::utf16String name, IG::utf16String name2, Gfx::GlyphTextureSet *face):
	BaseTextMenuItem{std::move(name), face},
	t2{std::move(name2), face} {}

void BaseDualTextMenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	BaseTextMenuItem::compile(r, projP);
	compile2nd(r, projP);
}

void BaseDualTextMenuItem::compile2nd(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t2.compile(r, projP);
}

void BaseDualTextMenuItem::prepareDraw(Gfx::Renderer &r)
{
	BaseTextMenuItem::prepareDraw(r);
	t2.makeGlyphs(r);
}

void BaseDualTextMenuItem::draw2ndText(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	cmds.setCommonProgram(Gfx::CommonProgram::TEX_ALPHA);
	cmds.setColor(color);
	t2.draw(cmds, (xPos + xSize) - xIndent, yPos, RC2DO, projP);
}

void BaseDualTextMenuItem::draw(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	BaseDualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
}

DualTextMenuItem::DualTextMenuItem(IG::utf16String name, IG::utf16String name2,
	Gfx::GlyphTextureSet *face, SelectDelegate selectDel):
	BaseDualTextMenuItem{std::move(name), std::move(name2), face},
	selectD{selectDel} {}

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
	if(val != on)
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

void BoolMenuItem::draw(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
	Gfx::Color color2;
	if(!onOffStyle) // custom strings
		color2 = Gfx::color(0.f, .8f, 1.f);
	else if(on)
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

	MenuItemTableView(IG::utf16String name, ViewAttachParams attach, int active, ItemsDelegate items, ItemDelegate item, MultiChoiceMenuItem &src):
		TableView{std::move(name), attach, items, item},
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

	void drawElement(Gfx::RendererCommands &cmds, size_t i, MenuItem &item, Gfx::GCRect rect, float xIndent) const final
	{
		item.draw(cmds, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), xIndent, TableView::align, projP, menuTextColor((int)i == activeItem));
	}
};

MultiChoiceMenuItem::MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr,
	int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
	BaseDualTextMenuItem{std::move(name), {}, face},
	selectD
	{
		selectDel ? selectDel :
			[this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
			{
				item.defaultOnSelect(view, e);
			}
	},
	items_{items},
	item_{item},
	onSetDisplayString{onDisplayStr},
	selected_{selected} {}

void MultiChoiceMenuItem::draw(Gfx::RendererCommands &cmds, float xPos, float yPos, float xSize, float ySize,
	float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &projP, Gfx::Color color) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, xIndent, align, projP, color);
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

void MultiChoiceMenuItem::setDisplayString(size_t idx)
{
	if(onSetDisplayString.callSafe(idx, t2))
	{
		return;
	}
	else if(idx < items_(*this))
	{
		t2.setString(std::u16string{item_(*this, idx).text().stringView()});
	}
	else
	{
		t2.setString({});
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
		selected_ < (bool)items_(*this) ? selected_ : -1,
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

}
