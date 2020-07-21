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
#include <imagine/logger/logger.h>

MenuItem::~MenuItem() {}

BaseTextMenuItem::BaseTextMenuItem() {}

BaseTextMenuItem::BaseTextMenuItem(const char *str): t{str, &View::defaultFace} {}

BaseTextMenuItem::BaseTextMenuItem(const char *str, Gfx::GlyphTextureSet *face): t{str, face} {}

BaseTextMenuItem::BaseTextMenuItem(const char *str, bool isSelectable):
	MenuItem(isSelectable),
	t{str, &View::defaultFace}
{}
BaseTextMenuItem::BaseTextMenuItem(const char *str, bool isSelectable, Gfx::GlyphTextureSet *face):
	MenuItem(isSelectable),
	t{str, face}
{}

void BaseTextMenuItem::prepareDraw(Gfx::Renderer &r)
{
	t.makeGlyphs(r);
}

void BaseTextMenuItem::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	if(!active_)
	{
		// half-bright color
		auto col = cmds.color();
		cmds.setColor(col[0]/2.f, col[1]/2.f, col[2]/2.f, col[3]);
	}

	if(cmds.color()[3] == 1.f)
	{
		//logMsg("using replace program for non-alpha modulated text");
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA_REPLACE);
	}
	else
		cmds.setCommonProgram(CommonProgram::TEX_ALPHA);

	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += TableView::globalXIndent;
	t.draw(cmds, xPos, yPos, align, projP);
}

void BaseTextMenuItem::compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t.compile(r, projP);
}

void BaseTextMenuItem::compile(const char *str, Gfx::Renderer &r, const Gfx::ProjectionPlane &projP)
{
	t.setString(str);
	compile(r, projP);
}

int BaseTextMenuItem::ySize()
{
	return t.face()->nominalHeight();
}

Gfx::GC BaseTextMenuItem::xSize()
{
	return t.width();
}

void BaseTextMenuItem::setName(const char *name, Gfx::GlyphTextureSet *face)
{
	t.setString(name);
	if(face)
	{
		t.setFace(face);
	}
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

TextMenuItem::TextMenuItem() {}

TextMenuItem::TextMenuItem(const char *str, SelectDelegate selectDel):
	BaseTextMenuItem{str},
	selectD{selectDel}
{}

TextMenuItem::TextMenuItem(const char *str, Gfx::GlyphTextureSet *face, SelectDelegate selectDel):
	BaseTextMenuItem{str, face},
	selectD{selectDel}
{}

bool TextMenuItem::select(View &parent, Input::Event e)
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

TextHeadingMenuItem::TextHeadingMenuItem() {}

TextHeadingMenuItem::TextHeadingMenuItem(const char *str): BaseTextMenuItem{str, false, &View::defaultBoldFace} {}

bool TextHeadingMenuItem::select(View &parent, Input::Event e) { return true; };

BaseDualTextMenuItem::BaseDualTextMenuItem() {}

BaseDualTextMenuItem::BaseDualTextMenuItem(const char *str, const char *str2):
	BaseTextMenuItem{str},
	t2{str2, &View::defaultFace}
{}

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

void BaseDualTextMenuItem::draw2ndText(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	cmds.setCommonProgram(Gfx::CommonProgram::TEX_ALPHA);
	t2.draw(cmds, (xPos + xSize) - TableView::globalXIndent, yPos, RC2DO, projP);
}

void BaseDualTextMenuItem::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, align, projP);
	BaseDualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, align, projP);
}

void BaseDualTextMenuItem::set2ndName(const char *name)
{
	t2.setString(name);
}

DualTextMenuItem::DualTextMenuItem() {}

DualTextMenuItem::DualTextMenuItem(const char *str, const char *str2): BaseDualTextMenuItem{str, str2} {}

DualTextMenuItem::DualTextMenuItem(const char *str, const char *str2, SelectDelegate selectDel):
	BaseDualTextMenuItem{str, str2},
	selectD{selectDel}
{}

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

BoolMenuItem::BoolMenuItem() {}

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

void BoolMenuItem::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, align, projP);
	if(!onOffStyle) // custom strings
		cmds.setColor(0., .8, 1.);
	else if(on)
		cmds.setColor(.27, 1., .27);
	else
		cmds.setColor(1., .27, .27);
	draw2ndText(cmds, xPos, yPos, xSize, ySize, align, projP);
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

	MenuItemTableView(NameString name, ViewAttachParams attach, int active, ItemsDelegate items, ItemDelegate item, MultiChoiceMenuItem &src):
		TableView{std::move(name), attach, items, item},
		activeItem{active},
		src{src}
	{
		setOnSelectElement(
			[this](Input::Event e, uint32_t i, MenuItem &item)
			{
				if(item.select(*this, e))
				{
					this->src.setSelected(i, *this);
					dismiss();
				}
			});
	}

	void onAddedToController(ViewController *, Input::Event e) final
	{
		if(!e.isPointer())
		{
			selected = activeItem;
		}
	}

	void drawElement(Gfx::RendererCommands &cmds, uint32_t i, MenuItem &item, Gfx::GCRect rect) const final
	{
		if((int)i == activeItem)
			cmds.setColor(0., .8, 1.);
		else
			cmds.setColor(Gfx::COLOR_WHITE);
		item.draw(cmds, rect.x, rect.pos(C2DO).y, rect.xSize(), rect.ySize(), TableView::align, projP);
	}
};

MultiChoiceMenuItem::MultiChoiceMenuItem() {}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr,
	int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
	BaseDualTextMenuItem
	{
		str,
		nullptr
	},
	selectD
	{
		selectDel ? selectDel :
			[this](MultiChoiceMenuItem &item, View &view, Input::Event e)
			{
				item.defaultOnSelect(view, e);
			}
	},
	items_{items},
	item_{item},
	onSetDisplayString{onDisplayStr},
	selected_{selected}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, int selected,
	ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
	MultiChoiceMenuItem{str, SetDisplayStringDelegate{}, selected, items, item, selectDel}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr,
	int selected, ItemsDelegate items, ItemDelegate item):
	MultiChoiceMenuItem{str, onDisplayStr, selected, items, item, {}}
{}

MultiChoiceMenuItem::MultiChoiceMenuItem(const char *str, int selected,
	ItemsDelegate items, ItemDelegate item):
	MultiChoiceMenuItem{str, SetDisplayStringDelegate{}, selected, items, item, {}}
{}

void MultiChoiceMenuItem::draw(Gfx::RendererCommands &cmds, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(cmds, xPos, yPos, xSize, ySize, align, projP);
	//r.setColor(0., 1., 1.); // aqua
	cmds.setColor(0., .8, 1.);
	BaseDualTextMenuItem::draw2ndText(cmds, xPos, yPos, xSize, ySize, align, projP);
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

uint32_t MultiChoiceMenuItem::items() const
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

void MultiChoiceMenuItem::setDisplayString(int idx)
{
	if(onSetDisplayString.callSafe(idx, t2))
	{
		return;
	}
	else if((uint32_t)idx < items_(*this))
	{
		t2.setString(item_(*this, idx).text());
	}
	else
	{
		t2.setString(nullptr);
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

std::unique_ptr<TableView> MultiChoiceMenuItem::makeTableView(ViewAttachParams attach)
{
	return std::make_unique<MenuItemTableView>
	(
		View::NameString{t.stringView()},
		attach,
		(uint32_t)selected_ < items_(*this) ? selected_ : -1,
		[this](const TableView &)
		{
			return items_(*this);
		},
		[this](const TableView &, uint32_t idx) -> MenuItem&
		{
			return item_(*this, idx);
		},
		*this
	);
}

void MultiChoiceMenuItem::defaultOnSelect(View &view, Input::Event e)
{
	view.pushAndShow(makeTableView(view.attachParams()), e);
}
