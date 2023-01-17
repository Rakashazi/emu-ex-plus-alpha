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

	constexpr MenuItemSelectDelegate(IG::Callable<void, Item &, View &, const Input::Event &> auto &&f):
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
		return f(IG_forward(args)...);
	}

	// auto-return true if the supplied function doesn't return a value
	constexpr static bool callAndReturnBool(auto &f, auto &&...args)
		requires IG::VoidInvokeResult<decltype(f), decltype(args)...>
	{
		f(IG_forward(args)...);
		return true;
	}
};

class MenuItem
{
public:
	using IdInt = int32_t;
	enum Id : IdInt{};
	static constexpr uint32_t SELECTABLE_FLAG = bit(0);
	static constexpr uint32_t ACTIVE_FLAG = bit(1);
	static constexpr uint32_t HIGHLIGHT_FLAG = bit(2);
	static constexpr uint32_t IMPL_FLAG_START = bit(3);
	static constexpr uint32_t USER_FLAG_START = bit(16);
	static constexpr uint32_t DEFAULT_FLAGS = SELECTABLE_FLAG | ACTIVE_FLAG;
	static constexpr Id DEFAULT_ID = static_cast<Id>(std::numeric_limits<IdInt>::min());

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
	constexpr auto flags() const { return flags_; }
	constexpr void setFlags(uint32_t flags) { flags_ = flags; }
	constexpr bool selectable() const { return flags_ & SELECTABLE_FLAG; }
	constexpr void setSelectable(bool on) { flags_ = setOrClearBits(flags_, SELECTABLE_FLAG, on); }
	constexpr bool active() const { return flags_ & ACTIVE_FLAG; }
	constexpr void setActive(bool on) { flags_ = setOrClearBits(flags_, ACTIVE_FLAG, on); }
	constexpr bool highlighted() const { return flags_ & HIGHLIGHT_FLAG; }
	constexpr void setHighlighted(bool on) { flags_ = setOrClearBits(flags_, HIGHLIGHT_FLAG, on); }
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
	uint32_t flags_{DEFAULT_FLAGS};
	IdInt id_{};
	Gfx::Text t;
};

class TextMenuItem : public MenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<TextMenuItem>;

	constexpr TextMenuItem() = default;

	TextMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SelectDelegate selectDel, IdInt id = {}):
		MenuItem{IG_forward(name), face, id},
		selectD{selectDel} {}

	bool select(View &parent, const Input::Event &e) override { return selectD.callCopySafe(*this, parent, e); }
	void setOnSelect(SelectDelegate onSelect) { selectD = onSelect; }
	const SelectDelegate &onSelect() const { return selectD; }

protected:
	SelectDelegate selectD;
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

	constexpr DualTextMenuItem() = default;

	DualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, Gfx::GlyphTextureSet *face,
		SelectDelegate selectDel, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), IG_forward(name2), face, id},
		selectD{selectDel} {}

	DualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, Gfx::GlyphTextureSet *face, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), IG_forward(name2), face, id} {}

	bool select(View &, const Input::Event &) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD;
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<BoolMenuItem>;
	static constexpr uint32_t ON_FLAG = IMPL_FLAG_START;
	static constexpr uint32_t ON_OFF_STYLE_FLAG = IMPL_FLAG_START << 1;

	constexpr BoolMenuItem() = default;

	BoolMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, bool val, SelectDelegate selectDel, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), val ? u"On" : u"Off", face, id},
		selectD{selectDel}
	{
		if(val)
			flags_ |= ON_FLAG;
		flags_ |= ON_OFF_STYLE_FLAG;
	}

	BoolMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, bool val,
		UTF16Convertible auto &&offStr, UTF16Convertible auto &&onStr, SelectDelegate selectDel, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), val ? onStr : offStr, face, id},
		selectD{selectDel},
		offStr{IG_forward(offStr)},
		onStr{IG_forward(onStr)}
	{
		if(val)
			flags_ |= ON_FLAG;
	}

	bool boolValue() const;
	bool setBoolValue(bool val, View &view);
	bool setBoolValue(bool val);
	bool flipBoolValue(View &view);
	bool flipBoolValue();
	void draw(Gfx::RendererCommands &__restrict__, int xPos, int yPos, int xSize, int ySize,
		int xIndent, _2DOrigin align, Gfx::Color) const override;
	bool select(View &, const Input::Event &) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD;
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

	constexpr MultiChoiceMenuItem() = default;

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr,
		SelectedInit selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel, IdInt id = {}):
		BaseDualTextMenuItem{IG_forward(name), UTF16String{}, face},
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
		selected_{selected.isId ? idxOfId((Id)selected.val) : selected.val} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name,
		Gfx::GlyphTextureSet *face, SelectedInit selected,
		ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, SetDisplayStringDelegate{}, selected, items, item, selectDel, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name,
		Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr,
		SelectedInit selected, ItemsDelegate items, ItemDelegate item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, onDisplayStr, selected, items, item, {}, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name,
		Gfx::GlyphTextureSet *face, SelectedInit selected,
		ItemsDelegate items, ItemDelegate item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, SetDisplayStringDelegate{}, selected, items, item, {}, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, SelectedInit selected,
		IG::Container auto &item, SelectDelegate selectDel, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, onDisplayStr, selected,
		[&item](const MultiChoiceMenuItem &)
		{
			return std::size(item);
		},
		[&item](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return std::data(item)[idx];
		},
		selectDel, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SelectedInit selected, IG::Container auto &item, SelectDelegate selectDel, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, SetDisplayStringDelegate{}, selected, item, selectDel, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, SelectedInit selected, IG::Container auto &item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, onDisplayStr, selected, item, {}, id} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face, SelectedInit selected, IG::Container auto &item, IdInt id = {}):
		MultiChoiceMenuItem{IG_forward(name), face, SetDisplayStringDelegate{}, selected, item, {}, id} {}

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
	void setOnSelect(SelectDelegate onSelect);
	std::unique_ptr<TableView> makeTableView(ViewAttachParams attach);
	void defaultOnSelect(View &, const Input::Event &);
	void updateDisplayString();
	int idxOfId(IdInt);

protected:
	SelectDelegate selectD;
	ItemsDelegate items_;
	ItemDelegate item_;
	SetDisplayStringDelegate onSetDisplayString;
	int selected_{};

	void setDisplayString(size_t idx);
};

}
