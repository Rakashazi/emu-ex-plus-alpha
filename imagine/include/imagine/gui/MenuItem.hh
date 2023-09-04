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

#include <imagine/config/defs.hh>
#include <imagine/gui/ViewAttachParams.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <vector>
#include <iterator>
#include <memory>
#include <type_traits>

namespace IG::Input
{
class Event;
}

namespace IG
{

class View;
class TableView;

template <class Item>
class MenuItemSelectDelegate : public DelegateFunc<bool (Item &, View &, const Input::Event &)>
{
public:
	using DelegateFuncBase = DelegateFunc<bool (Item &, View &, const Input::Event &)>;
	using DelegateFuncBase::DelegateFuncBase;

	// Wraps different function signatures into a delegate function with signature:
	// bool (Item &, View &, const Input::Event &)

	constexpr MenuItemSelectDelegate(Callable<void, Item &, View &, const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, i, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<Item &, const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, i, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<View &, const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<Item &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, i); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<View &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, v); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, const Input::Event &e) { return callAndReturnBool(f); }
		} {}

	constexpr static bool callAndReturnBool(auto &f, auto &&...args)
	{
		if constexpr(VoidInvokeResult<decltype(f), decltype(args)...>)
		{
			// auto-return true if the supplied function doesn't return a value
			f(IG_forward(args)...);
			return true;
		}
		else
		{
			return f(IG_forward(args)...);
		}
	}
};

struct MenuItemFlags
{
	uint32_t
	selectable:1{},
	active:1{},
	highlight:1{},
	impl:4{},
	user:4{};
};

class MenuItem
{
public:
	using IdInt = int32_t;
	enum Id : IdInt{};

	static constexpr Id DEFAULT_ID = static_cast<Id>(std::numeric_limits<IdInt>::min());
	MenuItemFlags flags{.selectable = true, .active = true};

	constexpr MenuItem() = default;
	MenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, IdInt id = {}):
		id_{id},
		t{IG_forward(name), face} {}
	virtual ~MenuItem() = default;
	virtual void prepareDraw(Gfx::Renderer &r);
	virtual void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const;
	virtual void compile(Gfx::Renderer &r);
	int ySize() const;
	int xSize() const;
	virtual bool select(View &, const Input::Event &) = 0;
	constexpr bool selectable() const { return flags.selectable; }
	constexpr void setSelectable(bool on) { flags.selectable = on; }
	constexpr bool active() const { return flags.active; }
	constexpr void setActive(bool on) { flags.active = on; }
	constexpr bool highlighted() const { return flags.highlight; }
	constexpr void setHighlighted(bool on) { flags.highlight = on; }
	constexpr Id id() const { return (Id)id_; }
	constexpr void setId(IdInt id) { id_ = id; }

	void compile(UTF16Convertible auto &&name, Gfx::Renderer &r)
	{
		t.resetString(IG_forward(name));
		compile(r);
	}

	void setName(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face = nullptr)
	{
		t.resetString(IG_forward(name));
		if(face)
			t.setFace(face);
	}

	const Gfx::Text &text() const;

protected:
	IdInt id_{};
	Gfx::Text t;
};

class TextMenuItem : public MenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<TextMenuItem>;

	SelectDelegate onSelect;

	constexpr TextMenuItem() = default;

	TextMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SelectDelegate onSelect, IdInt id = {}):
		MenuItem{IG_forward(name), face, id},
		onSelect{onSelect} {}

	TextMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, IdInt id):
		MenuItem{IG_forward(name), face, id} {}

	bool select(View &parent, const Input::Event &e) override { return onSelect.callCopySafe(*this, parent, e); }
};

class TextHeadingMenuItem : public MenuItem
{
public:
	constexpr TextHeadingMenuItem() = default;

	TextHeadingMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, IdInt id = {}):
		MenuItem{IG_forward(name), face, id}
	{
		setSelectable(false);
	}

	bool select(View &, const Input::Event &) override { return true; }
};

class BaseDualTextMenuItem : public MenuItem
{
public:
	constexpr BaseDualTextMenuItem() = default;

	BaseDualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, Gfx::GlyphTextureSet *face, IdInt id = {}):
		MenuItem{IG_forward(name), face, id},
		t2{IG_forward(name2), face} {}

	void set2ndName(UTF16Convertible auto &&name) { t2.resetString(IG_forward(name)); }
	void set2ndName() { t2.resetString(); }
	void compile(Gfx::Renderer &r) override;
	void compile2nd(Gfx::Renderer &r);
	void prepareDraw(Gfx::Renderer &r) override;
	void draw2ndText(Gfx::RendererCommands &, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const;
	void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const override;

protected:
	Gfx::Text t2;
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<DualTextMenuItem>;

	SelectDelegate onSelect;
	Gfx::Color text2Color;

	constexpr DualTextMenuItem() = default;

	DualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, Gfx::GlyphTextureSet *face,
		SelectDelegate onSelect = {}, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), IG_forward(name2), face, id},
		onSelect{onSelect} {}

	void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const override;
	bool select(View &, const Input::Event &) override;
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<BoolMenuItem>;
	static constexpr uint32_t onFlag = bit(0);
	static constexpr uint32_t onOffStyleFlag = bit(1);

	SelectDelegate onSelect;

	constexpr BoolMenuItem() = default;

	BoolMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, bool val, SelectDelegate onSelect, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), val ? u"On" : u"Off", face, id},
		onSelect{onSelect}
	{
		if(val)
			flags.impl |= onFlag;
		flags.impl |= onOffStyleFlag;
	}

	BoolMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, bool val,
		UTF16Convertible auto &&offStr, UTF16Convertible auto &&onStr, SelectDelegate onSelect, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), val ? onStr : offStr, face, id},
		onSelect{onSelect},
		offStr{IG_forward(offStr)},
		onStr{IG_forward(onStr)}
	{
		if(val)
			flags.impl |= onFlag;
	}

	bool boolValue() const;
	bool setBoolValue(bool val, View &view);
	bool setBoolValue(bool val);
	bool flipBoolValue(View &view);
	bool flipBoolValue();
	void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const override;
	bool select(View &, const Input::Event &) override;

protected:
	UTF16String offStr{u"Off"}, onStr{u"On"};
};

class MultiChoiceMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (MultiChoiceMenuItem &, View &, const Input::Event &)>;
	using ItemsDelegate = DelegateFunc<size_t (const MultiChoiceMenuItem &item)>;
	using ItemDelegate = DelegateFunc<TextMenuItem& (const MultiChoiceMenuItem &item, size_t idx)>;
	using SetDisplayStringDelegate = DelegateFunc<bool(size_t idx, Gfx::Text &text)>;

	struct SelectedInit
	{
		int val{};
		bool isId{};

		constexpr SelectedInit(int i): val{i} {}
		constexpr SelectedInit(Id i): val{i}, isId{true} {}
	};

	struct Delegates
	{
		SetDisplayStringDelegate onSetDisplayString{};
		SelectDelegate onSelect{};
		TextMenuItem::SelectDelegate defaultItemOnSelect{};
	};

	SelectDelegate onSelect;

	constexpr MultiChoiceMenuItem() = default;

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, Delegates delegates,
		SelectedInit selected, ItemsDelegate items, ItemDelegate item, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), UTF16String{}, face, id},
		onSelect
		{
			delegates.onSelect ? delegates.onSelect :
				[this](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
				{
					item.defaultOnSelect(view, e);
				}
		},
		items_{items},
		item_{item},
		onSetDisplayString{delegates.onSetDisplayString},
		selected_{selected.isId ? idxOfId((Id)selected.val) : selected.val} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face,
		SelectedInit selected, ItemsDelegate items, ItemDelegate item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, {}, selected, items, item, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, Delegates delegates,
		SelectedInit selected, Container auto &&item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, delegates, selected,
		itemsDelegate(IG_forward(item)),
		itemDelegate(IG_forward(item)),
		id}
	{
		if(delegates.defaultItemOnSelect)
		{
			for(auto &i : item)
			{
				if(!i.onSelect)
					i.onSelect = delegates.defaultItemOnSelect;
			}
		}
	}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face,
		SelectedInit selected, Container auto &&item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, {}, selected, IG_forward(item), id} {}

	void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const override;
	void compile(Gfx::Renderer &r) override;
	int selected() const;
	size_t items() const;
	bool setSelected(int idx, View &view);
	bool setSelected(int idx);
	bool setSelected(Id, View &view);
	bool setSelected(Id);
	int cycleSelected(int offset, View &view);
	int cycleSelected(int offset);
	bool select(View &, const Input::Event &) override;
	std::unique_ptr<TableView> makeTableView(ViewAttachParams attach);
	void defaultOnSelect(View &, const Input::Event &);
	void updateDisplayString();
	int idxOfId(IdInt);

protected:
	ItemsDelegate items_;
	ItemDelegate item_;
	SetDisplayStringDelegate onSetDisplayString;
	int selected_{};

	void setDisplayString(size_t idx);

	static constexpr ItemsDelegate itemsDelegate(Container auto &&item)
	{
		if constexpr(std::is_rvalue_reference_v<decltype(item)>)
			return [size = std::size(item)](const MultiChoiceMenuItem &) { return size; };
		else
			return [&item](const MultiChoiceMenuItem &) { return std::size(item); };
	}

	static constexpr ItemDelegate itemDelegate(Container auto &&item)
	{
		if constexpr(std::is_rvalue_reference_v<decltype(item)>)
			return [item](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem& { return std::data(item)[idx]; };
		else
			return [&item](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem& { return std::data(item)[idx]; };
	}
};

}
