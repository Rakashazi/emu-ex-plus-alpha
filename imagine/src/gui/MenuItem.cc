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

bool MenuItemI::inputEvent(const Input::Event&, ViewInputEventParams) { return false; }

void MenuItem::prepareDraw()
{
	t.makeGlyphs();
}

void MenuItem::draw(Gfx::RendererCommands&__restrict__ cmds, MenuItemDrawAttrs attrs) const
{
	if(!active())
	{
		// half-bright color
		attrs.color.r /= 2.f;
		attrs.color.g /= 2.f;
		attrs.color.b /= 2.f;
	}
	if(!attrs.align.isXCentered())
		attrs.rect += WPt{attrs.xIndent, 0};
	cmds.basicEffect().enableAlphaTexture(cmds);
	t.draw(cmds, attrs.rect.pos(attrs.align), attrs.align, attrs.color);
}

void MenuItem::place()
{
	t.compile();
}

int MenuItem::ySize() const
{
	return t.face() ? t.face()->nominalHeight() : 0;
}

int MenuItem::xSize() const
{
	return t.width();
}

const Gfx::Text &MenuItem::text() const
{
	return t;
}

bool TextMenuItem::inputEvent(const Input::Event& e, ViewInputEventParams p) { return onSelect.callCopySafe(*this, *p.parentPtr, e); }

void BaseDualTextMenuItem::place()
{
	MenuItem::place();
	place2nd();
}

void BaseDualTextMenuItem::place2nd()
{
	t2.compile();
}

void BaseDualTextMenuItem::prepareDraw()
{
	MenuItem::prepareDraw();
	t2.makeGlyphs();
}

void BaseDualTextMenuItem::draw2ndText(Gfx::RendererCommands &cmds, MenuItemDrawAttrs attrs) const
{
	cmds.basicEffect().enableAlphaTexture(cmds);
	attrs.rect -= WPt{attrs.xIndent, 0};
	t2.draw(cmds, attrs.rect.pos(RC2DO), RC2DO, attrs.color);
}

void BaseDualTextMenuItem::draw(Gfx::RendererCommands&__restrict__ cmds, MenuItemDrawAttrs attrs) const
{
	MenuItem::draw(cmds, attrs);
	BaseDualTextMenuItem::draw2ndText(cmds, attrs);
}

void DualTextMenuItem::draw(Gfx::RendererCommands&__restrict__ cmds, MenuItemDrawAttrs attrs) const
{
	MenuItem::draw(cmds, attrs);
	if(text2Color != Gfx::Color{})
		attrs.color = text2Color;
	draw2ndText(cmds, attrs);
}

bool DualTextMenuItem::inputEvent(const Input::Event& e, ViewInputEventParams p)
{
	//logMsg("calling delegate");
	onSelect.callCopySafe(*this, *p.parentPtr, e);
	return true;
}

bool BoolMenuItem::inputEvent(const Input::Event& e, ViewInputEventParams p)
{
	onSelect.callCopySafe(*this, *p.parentPtr, e);
	return true;
}

bool BoolMenuItem::setBoolValue(bool val, View &view)
{
	if(val != boolValue())
	{
		setBoolValue(val);
		t2.compile();
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
		flags.impl = setOrClearBits(flags.impl, onFlag, val);
		t2.resetString(val ? onStr : offStr);
		return true;
	}
	return false;
}

bool BoolMenuItem::boolValue() const
{
	return flags.impl & onFlag;
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

void BoolMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, MenuItemDrawAttrs attrs) const
{
	MenuItem::draw(cmds, attrs);
	if(!(flags.impl & onOffStyleFlag)) // custom strings
		attrs.color = Gfx::Color{0.f, .8f, 1.f};
	else if(boolValue())
		attrs.color = Gfx::Color{.27f, 1.f, .27f};
	else
		attrs.color = Gfx::Color{1.f, .27f, .27f};
	draw2ndText(cmds, attrs);
}

class MenuItemTableView : public TableView
{
public:
	int activeItem;
	MultiChoiceMenuItem &src;

	MenuItemTableView(UTF16Convertible auto &&name, ViewAttachParams attach, int active, ItemSourceDelegate itemSrc, MultiChoiceMenuItem &src):
		TableView{IG_forward(name), attach, itemSrc},
		activeItem{active},
		src{src}
	{
		setOnSelectElement([this](const Input::Event& e, int i, MenuItem& item)
		{
			if(item.inputEvent(e, {.parentPtr = this}))
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

	void drawElement(Gfx::RendererCommands &__restrict__ cmds, size_t i, MenuItem &item, WRect rect, int xIndent) const final
	{
		MenuItemDrawAttrs attrs{.rect = rect, .xIndent = xIndent,
			.color = menuTextColor((int)i == activeItem), .align = TableView::align};
		item.draw(cmds, attrs);
	}
};

void MultiChoiceMenuItem::draw(Gfx::RendererCommands &__restrict__ cmds, MenuItemDrawAttrs attrs) const
{
	MenuItem::draw(cmds, attrs);
	attrs.color = Gfx::Color{0.f, .8f, 1.f};
	BaseDualTextMenuItem::draw2ndText(cmds, attrs);
}

void MultiChoiceMenuItem::place()
{
	setDisplayString(selected_);
	BaseDualTextMenuItem::place();
}

int MultiChoiceMenuItem::selected() const
{
	return selected_;
}

size_t MultiChoiceMenuItem::items() const
{
	return getAs<size_t>(itemSrc(ItemsMessage{*this}));
}

TextMenuItem& MultiChoiceMenuItem::item(ItemSourceDelegate src, size_t idx)
{
	return *getAs<TextMenuItem*>(src(GetItemMessage{*this, idx}));
}

bool MultiChoiceMenuItem::setSelected(int idx, View &view)
{
	bool selectChanged = setSelected(idx);
	t2.compile();
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

bool MultiChoiceMenuItem::setSelected(MenuId id, View &view)
{
	return setSelected(idxOfId(id), view);
}

bool MultiChoiceMenuItem::setSelected(MenuId id)
{
	return setSelected(idxOfId(id));
}

void MultiChoiceMenuItem::setDisplayString(size_t idx)
{
	if(onSetDisplayString.callSafe(idx, t2))
	{
		return;
	}
	else if(idx < items())
	{
		t2.resetString(std::u16string{item(idx).text().stringView()});
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

bool MultiChoiceMenuItem::inputEvent(const Input::Event& e, ViewInputEventParams p)
{
	//logMsg("calling delegate");
	onSelect.callCopySafe(*this, *p.parentPtr, e);
	return true;
}

std::unique_ptr<TableView> MultiChoiceMenuItem::makeTableView(ViewAttachParams attach)
{
	return std::make_unique<MenuItemTableView>
	(
		std::u16string{t.stringView()},
		attach,
		selected_ < (ssize_t)items() ? selected_ : -1,
		[this](TableView::ItemMessage msg) -> TableView::ItemReply
		{
			return msg.visit(overloaded
			{
				[&](const TableView::ItemsMessage&) -> TableView::ItemReply { return items(); },
				[&](const TableView::GetItemMessage& m) -> TableView::ItemReply { return &item(m.idx); },
			});
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

int MultiChoiceMenuItem::idxOfId(MenuId id)
{
	auto count = items();
	auto src = itemSrc;
	MenuId lastId{};
	for(auto i : iotaCount(count))
	{
		lastId = item(src, i).id;
		if(lastId == id)
			return (int)i;
	}
	if(lastId == defaultMenuId) // special case to simplify uses where the last menu item represents a custom value
		return count - 1;
	else
		return -1;
}

}
