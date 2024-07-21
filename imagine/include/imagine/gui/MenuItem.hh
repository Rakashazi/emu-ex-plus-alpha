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
#include <imagine/gui/ViewManager.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
#include <imagine/util/variant.hh>
#include <iterator>
#include <memory>
#include <type_traits>

namespace IG::Input
{
class Event;
}

namespace IG
{

template<class Func, class... Args>
constexpr bool callAndAutoReturnTrue(Func& f, Args&&... args)
{
	if constexpr(VoidInvokeResult<Func, Args...>)
	{
		// auto-return true if the supplied function doesn't return a value
		f(std::forward<Args>(args)...);
		return true;
	}
	else
	{
		return f(std::forward<Args>(args)...);
	}
}

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
			[=](Item& i, View& v, const Input::Event& e) { return callAndAutoReturnTrue(f, i, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<Item &, const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item& i, View&, const Input::Event& e) { return callAndAutoReturnTrue(f, i, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<View &, const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item&, View& v, const Input::Event& e) { return callAndAutoReturnTrue(f, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<Item &> auto &&f):
		DelegateFuncBase
		{
			[=](Item& i, View&, const Input::Event&) { return callAndAutoReturnTrue(f, i); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<View &> auto &&f):
		DelegateFuncBase
		{
			[=](Item&, View& v, const Input::Event&) { return callAndAutoReturnTrue(f, v); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable<const Input::Event &> auto &&f):
		DelegateFuncBase
		{
			[=](Item&, View&, const Input::Event& e) { return callAndAutoReturnTrue(f, e); }
		} {}

	constexpr MenuItemSelectDelegate(std::invocable auto &&f):
		DelegateFuncBase
		{
			[=](Item&, View&, const Input::Event&) { return callAndAutoReturnTrue(f); }
		} {}
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

struct MenuId
{
	using Type = int32_t;
	Type val{};

	constexpr MenuId() = default;
	constexpr MenuId(auto &&i):val{static_cast<Type>(i)} {}
	constexpr operator Type() const { return val; }
};

struct MenuItemDrawAttrs
{
	WindowRect rect{};
	int xIndent{};
	Gfx::Color color{};
	_2DOrigin align{};
};

struct MenuItemI
{
	virtual ~MenuItemI() = default;
	virtual void place() = 0;
	virtual void prepareDraw() = 0;
	virtual void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs a = {}) const = 0;
	virtual bool inputEvent(const Input::Event&, ViewInputEventParams p = {});
};

constexpr MenuId defaultMenuId{std::numeric_limits<MenuId::Type>::min()};

class MenuItem: public MenuItemI
{
public:
	struct Config
	{
		Gfx::GlyphTextureSet *face{};
		MenuId id{};
	};

	MenuItemFlags flags{.selectable = true, .active = true};
	MenuId id{};

	MenuItem() = default;

	MenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, Config conf):
		id{conf.id},
		t{attach.rendererTask, IG_forward(name), conf.face ?: &attach.viewManager.defaultFace} {}

	MenuItem(MenuItem&&) = default;
	MenuItem &operator=(MenuItem&&) = default;
	void prepareDraw() override;
	void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const override;
	void place() override;
	int ySize() const;
	int xSize() const;
	constexpr bool selectable() const { return flags.selectable; }
	constexpr void setSelectable(bool on) { flags.selectable = on; }
	constexpr bool active() const { return flags.active; }
	constexpr void setActive(bool on) { flags.active = on; }
	constexpr bool highlighted() const { return flags.highlight; }
	constexpr void setHighlighted(bool on) { flags.highlight = on; }

	void compile(UTF16Convertible auto &&name)
	{
		t.resetString(IG_forward(name));
		place();
	}

	void setName(UTF16Convertible auto &&name, Gfx::GlyphTextureSet *face = {})
	{
		t.resetString(IG_forward(name));
		if(face)
			t.setFace(face);
	}

	static constexpr Config toBaseConfig(auto &&conf) { return {.face = conf.face, .id = conf.id}; }

	const Gfx::Text &text() const;

protected:
	Gfx::Text t;
};

class TextMenuItem : public MenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<TextMenuItem>;

	SelectDelegate onSelect;

	TextMenuItem() = default;

	TextMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, SelectDelegate onSelect = {}, Config conf = Config{}):
		MenuItem{IG_forward(name), attach, conf},
		onSelect{onSelect} {}

	TextMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, Config conf):
		MenuItem{IG_forward(name), attach, conf} {}

	bool inputEvent(const Input::Event& e, ViewInputEventParams p) override;
};

class TextHeadingMenuItem : public MenuItem
{
public:
	static constexpr Config applyBoldFont(ViewAttachParams attach, Config conf) { conf.face = &attach.viewManager.defaultBoldFace; return conf; }

	TextHeadingMenuItem() = default;

	TextHeadingMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, Config conf = Config{}):
		MenuItem{IG_forward(name), attach, conf.face ? conf : applyBoldFont(attach, conf)}
	{
		setSelectable(false);
	}
};

class BaseDualTextMenuItem : public MenuItem
{
public:
	BaseDualTextMenuItem() = default;

	BaseDualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, ViewAttachParams attach, Config conf = Config{}):
		MenuItem{IG_forward(name), attach, conf},
		t2{attach.rendererTask, IG_forward(name2), t.face()} {}

	void set2ndName(UTF16Convertible auto &&name) { t2.resetString(IG_forward(name)); }
	void set2ndName() { t2.resetString(); }
	void place() override;
	void place2nd();
	void prepareDraw() override;
	void draw2ndText(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const;
	void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const override;

protected:
	Gfx::Text t2;
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<DualTextMenuItem>;

	SelectDelegate onSelect;
	Gfx::Color text2Color;

	DualTextMenuItem() = default;

	DualTextMenuItem(UTF16Convertible auto &&name, UTF16Convertible auto &&name2, ViewAttachParams attach,
		SelectDelegate onSelect, Config conf = Config{}):
		BaseDualTextMenuItem{IG_forward(name), IG_forward(name2), attach, conf},
		onSelect{onSelect} {}

	void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const override;
	bool inputEvent(const Input::Event&, ViewInputEventParams) override;
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<BoolMenuItem>;
	static constexpr uint32_t onFlag = bit(0);
	static constexpr uint32_t onOffStyleFlag = bit(1);

	SelectDelegate onSelect;

	BoolMenuItem() = default;

	BoolMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, bool val,
		SelectDelegate onSelect, Config conf = Config{}):
		BaseDualTextMenuItem{IG_forward(name), val ? u"On" : u"Off", attach, conf},
		onSelect{onSelect}
	{
		if(val)
			flags.impl |= onFlag;
		flags.impl |= onOffStyleFlag;
	}

	BoolMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach, bool val,
		UTF16Convertible auto &&offStr, UTF16Convertible auto &&onStr, SelectDelegate onSelect, Config conf = Config{}):
		BaseDualTextMenuItem{IG_forward(name), val ? onStr : offStr, attach, conf},
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
	void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const override;
	bool inputEvent(const Input::Event&, ViewInputEventParams) override;

protected:
	UTF16String offStr{u"Off"}, onStr{u"On"};
};

class MultiChoiceMenuItem : public BaseDualTextMenuItem
{
public:
	struct ItemsMessage {const MultiChoiceMenuItem& item;};
	struct GetItemMessage {const MultiChoiceMenuItem& item; size_t idx;};
	using ItemMessageVariant = std::variant<GetItemMessage, ItemsMessage>;
	class ItemMessage: public ItemMessageVariant, public AddVisit
	{
	public:
		using ItemMessageVariant::ItemMessageVariant;
		using AddVisit::visit;
	};
	using ItemReply = std::variant<TextMenuItem*, size_t>;
	using ItemSourceDelegate = MenuItemSourceDelegate<ItemMessage, ItemReply, ItemsMessage, GetItemMessage>;
	using SelectDelegate = DelegateFunc<void (MultiChoiceMenuItem &, View &, const Input::Event &)>;
	using SetDisplayStringDelegate = DelegateFunc<bool(size_t idx, Gfx::Text &text)>;

	struct SelectedInit
	{
		int val{};
		bool isId{};

		constexpr SelectedInit(int i): val{i} {}
		constexpr SelectedInit(MenuId i): val{i.val}, isId{true} {}
	};

	struct Config
	{
		Gfx::GlyphTextureSet *face{};
		MenuId id{};
		SetDisplayStringDelegate onSetDisplayString{};
		SelectDelegate onSelect{};
		TextMenuItem::SelectDelegate defaultItemOnSelect{};

		static Config defaultConfig() { return {}; }
	};

	SelectDelegate onSelect;

	MultiChoiceMenuItem() = default;

	MultiChoiceMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach,
		SelectedInit selected, ItemSourceDelegate itemSrc, Config conf = Config::defaultConfig()):
		BaseDualTextMenuItem{IG_forward(name), UTF16String{}, attach, toBaseConfig(conf)},
		onSelect
		{
			conf.onSelect ? conf.onSelect :
				[](MultiChoiceMenuItem &item, View &view, const Input::Event &e)
				{
					item.defaultOnSelect(view, e);
				}
		},
		itemSrc{itemSrc},
		onSetDisplayString{conf.onSetDisplayString},
		selected_{selected.isId ? idxOfId(MenuId{selected.val}) : selected.val} {}

	MultiChoiceMenuItem(UTF16Convertible auto &&name, ViewAttachParams attach,
		SelectedInit selected, Container auto &&item, Config conf = Config::defaultConfig()):
		MultiChoiceMenuItem
		{
			IG_forward(name), attach, selected,
			ItemSourceDelegate{IG_forward(item)}, conf
		}
	{
		if(conf.defaultItemOnSelect)
		{
			for(auto &i : item)
			{
				if(!i.onSelect)
					i.onSelect = conf.defaultItemOnSelect;
			}
		}
	}

	void draw(Gfx::RendererCommands&__restrict__, MenuItemDrawAttrs) const override;
	void place() override;
	int selected() const;
	size_t items() const;
	TextMenuItem& item(size_t idx) { return item(itemSrc, idx); }
	bool setSelected(int idx, View &view);
	bool setSelected(int idx);
	bool setSelected(MenuId, View &view);
	bool setSelected(MenuId);
	int cycleSelected(int offset, View &view);
	int cycleSelected(int offset);
	bool inputEvent(const Input::Event&, ViewInputEventParams) override;
	std::unique_ptr<TableView> makeTableView(ViewAttachParams attach);
	void defaultOnSelect(View &, const Input::Event &);
	void updateDisplayString();
	int idxOfId(MenuId);

protected:
	ItemSourceDelegate itemSrc;
	SetDisplayStringDelegate onSetDisplayString;
	int selected_{};

	void setDisplayString(size_t idx);
	TextMenuItem& item(ItemSourceDelegate, size_t idx);
};

}
