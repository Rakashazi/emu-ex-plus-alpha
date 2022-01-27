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
#include <imagine/input/Input.hh>
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
class MenuItemSelectDelegate : public DelegateFunc<bool (Item &, View &, Input::Event)>
{
public:
	using DelegateFuncBase = DelegateFunc<bool (Item &, View &, Input::Event)>;
	using DelegateFuncBase::DelegateFuncBase;

	// Wraps different function signatures into a delegate function with signature:
	// bool (Item &, View &, Input::Event)

	constexpr MenuItemSelectDelegate(IG::Callable<void, Item &, View &, Input::Event> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, i, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable<Item &, Input::Event> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, i, e); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable<View &, Input::Event> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, v, e); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable<Item &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, i); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable<View &> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, v); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable<Input::Event> auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f, e); }
		} {}

	constexpr MenuItemSelectDelegate(IG::invocable auto &&f):
		DelegateFuncBase
		{
			[=](Item &i, View &v, Input::Event e) { return callAndReturnBool(f); }
		} {}

	constexpr static auto callAndReturnBool(auto &f, auto &&...args)
		requires IG::SameInvokeResult<decltype(f), bool, decltype(args)...>
	{
		return f(IG_forward(args)...);
	}

	// auto-return true if the supplied function doesn't return a value
	constexpr static auto callAndReturnBool(auto &f, auto &&...args)
		requires IG::SameInvokeResult<decltype(f), void, decltype(args)...>
	{
		f(IG_forward(args)...);
		return true;
	}
};

class MenuItem
{
public:
	bool isSelectable = true;

	constexpr MenuItem() = default;
	constexpr MenuItem(bool isSelectable):
		isSelectable{isSelectable} {}
	virtual ~MenuItem() = default;
	virtual void prepareDraw(Gfx::Renderer &r) = 0;
	virtual void draw(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const = 0;
	virtual void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) = 0;
	virtual int ySize() = 0;
	virtual float xSize() = 0;
	virtual bool select(View &view, Input::Event e) = 0;
};

class BaseTextMenuItem : public MenuItem
{
public:
	BaseTextMenuItem() = default;
	BaseTextMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face):
		t{std::move(name), face} {}
	BaseTextMenuItem(IG::utf16String name, bool isSelectable, Gfx::GlyphTextureSet *face):
		MenuItem(isSelectable),
		t{std::move(name), face} {}
	void prepareDraw(Gfx::Renderer &r) override;
	void draw(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void compile(IG::utf16String name, Gfx::Renderer &r, const Gfx::ProjectionPlane &projP);
	void setName(IG::utf16String name, Gfx::GlyphTextureSet *face = nullptr);
	int ySize() override;
	float xSize() override;
	const Gfx::Text &text() const;
	void setActive(bool on);
	bool active();

protected:
	bool active_ = true;
	Gfx::Text t{};
};

class TextMenuItem : public BaseTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<TextMenuItem>;

	TextMenuItem() = default;
	TextMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SelectDelegate selectDel);
	bool select(View &, Input::Event) override;
	void setOnSelect(SelectDelegate onSelect);
	SelectDelegate onSelect() const;

protected:
	SelectDelegate selectD{};
};

class TextHeadingMenuItem : public BaseTextMenuItem
{
public:
	TextHeadingMenuItem() = default;
	TextHeadingMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face):
		BaseTextMenuItem{std::move(name), false, face} {}
	bool select(View &view, Input::Event e) override;
};

class BaseDualTextMenuItem : public BaseTextMenuItem
{
public:
	BaseDualTextMenuItem() = default;
	BaseDualTextMenuItem(IG::utf16String name, IG::utf16String name2, Gfx::GlyphTextureSet *face);
	void set2ndName(IG::utf16String name) { t2.setString(std::move(name)); }
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void compile2nd(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP);
	void prepareDraw(Gfx::Renderer &r) override;
	void draw2ndText(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const;
	void draw(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;

protected:
	Gfx::Text t2{};
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<DualTextMenuItem>;

	DualTextMenuItem() = default;
	DualTextMenuItem(IG::utf16String name, IG::utf16String name2,
			Gfx::GlyphTextureSet *face, SelectDelegate selectDel);
	DualTextMenuItem(IG::utf16String name, IG::utf16String name2, Gfx::GlyphTextureSet *face):
		BaseDualTextMenuItem{std::move(name), std::move(name2), face} {}
	bool select(View &view, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD{};
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = MenuItemSelectDelegate<BoolMenuItem>;

	BoolMenuItem() = default;
	BoolMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, bool val, SelectDelegate selectDel):
		BaseDualTextMenuItem{std::move(name), val ? u"On" : u"Off", face},
		selectD{selectDel},
		on{val} {}
	BoolMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, bool val,
		IG::utf16String offStr, IG::utf16String onStr, SelectDelegate selectDel):
		BaseDualTextMenuItem{std::move(name), val ? onStr : offStr, face},
		selectD{selectDel},
		offStr{std::move(offStr)},
		onStr{std::move(onStr)},
		on{val},
		onOffStyle{false} {}
	bool boolValue() const;
	bool setBoolValue(bool val, View &view);
	bool setBoolValue(bool val);
	bool flipBoolValue(View &view);
	bool flipBoolValue();
	void draw(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	bool select(View &view, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD{};
	std::u16string offStr{u"Off"}, onStr{u"On"};
	bool on = false;
	bool onOffStyle = true;
};

class MultiChoiceMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (MultiChoiceMenuItem &item, View &view, Input::Event e)>;
	using ItemsDelegate = DelegateFunc<size_t (const MultiChoiceMenuItem &item)>;
	using ItemDelegate = DelegateFunc<TextMenuItem& (const MultiChoiceMenuItem &item, size_t idx)>;
	using SetDisplayStringDelegate = DelegateFunc<bool(size_t idx, Gfx::Text &text)>;

	MultiChoiceMenuItem() = default;
	MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr,
		int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel);
	MultiChoiceMenuItem(IG::utf16String name,
		Gfx::GlyphTextureSet *face, int selected,
		ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel):
		MultiChoiceMenuItem{std::move(name), face, SetDisplayStringDelegate{}, selected, items, item, selectDel} {}
	MultiChoiceMenuItem(IG::utf16String name,
		Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr,
		int selected, ItemsDelegate items, ItemDelegate item):
		MultiChoiceMenuItem{std::move(name), face, onDisplayStr, selected, items, item, {}} {}
	MultiChoiceMenuItem(IG::utf16String name,
		Gfx::GlyphTextureSet *face, int selected,
		ItemsDelegate items, ItemDelegate item):
		MultiChoiceMenuItem{std::move(name), face, SetDisplayStringDelegate{}, selected, items, item, {}} {}

	MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, int selected,
		IG::Container auto &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{std::move(name), face, onDisplayStr, selected,
		[&item](const MultiChoiceMenuItem &)
		{
			return std::size(item);
		},
		[&item](const MultiChoiceMenuItem &, size_t idx) -> TextMenuItem&
		{
			return std::data(item)[idx];
		},
		selectDel} {}

	MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, int selected, IG::Container auto &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{std::move(name), face, SetDisplayStringDelegate{}, selected, item, selectDel} {}

	MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, int selected, IG::Container auto &item):
		MultiChoiceMenuItem{std::move(name), face, onDisplayStr, selected, item, {}} {}

	MultiChoiceMenuItem(IG::utf16String name, Gfx::GlyphTextureSet *face, int selected, IG::Container auto &item):
		MultiChoiceMenuItem{std::move(name), face, SetDisplayStringDelegate{}, selected, item, {}} {}

	void draw(Gfx::RendererCommands &, float xPos, float yPos, float xSize, float ySize,
		float xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	int selected() const;
	size_t items() const;
	bool setSelected(int idx, View &view);
	bool setSelected(int idx);
	int cycleSelected(int offset, View &view);
	int cycleSelected(int offset);
	bool select(View &view, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);
	std::unique_ptr<TableView> makeTableView(ViewAttachParams attach);
	void defaultOnSelect(View &view, Input::Event e);
	void updateDisplayString();

protected:
	SelectDelegate selectD{};
	ItemsDelegate items_{};
	ItemDelegate item_{};
	SetDisplayStringDelegate onSetDisplayString{};
	int selected_ = 0;

	void setDisplayString(size_t idx);
};

}
