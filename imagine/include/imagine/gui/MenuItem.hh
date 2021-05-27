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
#include <imagine/util/typeTraits.hh>
#include <imagine/util/FunctionTraits.hh>
#include <vector>
#include <iterator>
#include <utility>
#include <memory>

class View;
class TableView;

namespace Input
{
class Event;
}

class MenuItem
{
public:
	bool isSelectable = true;

	constexpr MenuItem() {}
	constexpr MenuItem(bool isSelectable):
		isSelectable{isSelectable} {}
	virtual ~MenuItem();
	virtual void prepareDraw(Gfx::Renderer &r) = 0;
	virtual void draw(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const = 0;
	virtual void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) = 0;
	virtual int ySize() = 0;
	virtual Gfx::GC xSize() = 0;
	virtual bool select(View &view, Input::Event e) = 0;

	// Wraps different function signatures into a delegate function with signature:
	// void|bool (T &, View &, Input::Event)
	template<class T, class Func>
	static typename T::SelectDelegate wrapSelectDelegateFunction(Func &&func)
	{
		if constexpr(std::is_null_pointer_v<Func>)
		{
			return {};
		}
		else
		{
			return
				[=](T &item, View &view, Input::Event e)
				{
					auto funcAdaptor =
						[&]<class ...Args>(Args&& ...args)
						{
							constexpr auto delegateReturnTypeIsBool = std::is_same_v<bool, IG::FunctionTraitsR<typename T::SelectDelegate>>;
							constexpr auto delegateReturnsBoolFromFunc = delegateReturnTypeIsBool && std::is_same_v<bool, IG::FunctionTraitsR<Func>>;
							if constexpr(delegateReturnsBoolFromFunc)
							{
								return func(args...);
							}
							else
							{
								func(args...);
								if constexpr(delegateReturnTypeIsBool)
									return true;
							}
						};
					if constexpr(std::is_invocable_v<Func, T&, View&, Input::Event>)
					{
						return funcAdaptor(item, view, e);
					}
					else if constexpr(std::is_invocable_v<Func, T&, Input::Event>)
					{
						return funcAdaptor(item, e);
					}
					else if constexpr(std::is_invocable_v<Func, View&, Input::Event>)
					{
						return funcAdaptor(view, e);
					}
					else if constexpr(std::is_invocable_v<Func, T&>)
					{
						return funcAdaptor(item);
					}
					else if constexpr(std::is_invocable_v<Func, View&>)
					{
						return funcAdaptor(view);
					}
					else if constexpr(std::is_invocable_v<Func, Input::Event>)
					{
						return funcAdaptor(e);
					}
					else if constexpr(std::is_invocable_v<Func>)
					{
						return funcAdaptor();
					}
					else
					{
						static_assert(IG::dependentFalseValue<Func>, "incompatible function object");
					}
				};
		}
	}
};

class BaseTextMenuItem : public MenuItem
{
public:
	BaseTextMenuItem();
	BaseTextMenuItem(const char *str, Gfx::GlyphTextureSet *face);
	BaseTextMenuItem(const char *str, bool isSelectable, Gfx::GlyphTextureSet *face);
	void prepareDraw(Gfx::Renderer &r) override;
	void draw(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void compile(const char *str, Gfx::Renderer &r, const Gfx::ProjectionPlane &projP);
	int ySize() override;
	Gfx::GC xSize() override;
	void setName(const char *name, Gfx::GlyphTextureSet *face = nullptr);
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
	using SelectDelegate = DelegateFunc<bool (TextMenuItem &item, View &view, Input::Event e)>;

	TextMenuItem();
	TextMenuItem(const char *str, Gfx::GlyphTextureSet *face, SelectDelegate);
	bool select(View &, Input::Event) override;
	void setOnSelect(SelectDelegate onSelect);
	SelectDelegate onSelect() const;

	// SelectDelegate wrappers
	template<class Func>
	static SelectDelegate makeSelectDelegate(Func &&func)
	{
		return wrapSelectDelegateFunction<TextMenuItem>(std::forward<Func>(func));
	}

	template<class Func>
	TextMenuItem(const char *str, Gfx::GlyphTextureSet *face, Func &&func): TextMenuItem{str, face, makeSelectDelegate(std::forward<Func>(func))} {}

	template<class Func>
	void setOnSelect(Func &&func) { setOnSelect(makeSelectDelegate(std::forward<Func>(func))); }

protected:
	SelectDelegate selectD{};
};

class TextHeadingMenuItem : public BaseTextMenuItem
{
public:
	TextHeadingMenuItem();
	TextHeadingMenuItem(const char *str, Gfx::GlyphTextureSet *face);
	bool select(View &view, Input::Event e) override;
};

class BaseDualTextMenuItem : public BaseTextMenuItem
{
public:
	BaseDualTextMenuItem();
	BaseDualTextMenuItem(const char *str, const char *str2, Gfx::GlyphTextureSet *face);
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void compile2nd(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP);
	void prepareDraw(Gfx::Renderer &r) override;
	void draw2ndText(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const;
	void draw(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	void set2ndName(const char *name);

protected:
	Gfx::Text t2{};
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (DualTextMenuItem &item, View &view, Input::Event e)>;

	DualTextMenuItem();
	DualTextMenuItem(const char *str, const char *str2, Gfx::GlyphTextureSet *face);
	DualTextMenuItem(const char *str, const char *str2, Gfx::GlyphTextureSet *face, SelectDelegate selectDel);
	bool select(View &view, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

	// SelectDelegate wrappers
	template<class Func>
	static SelectDelegate makeSelectDelegate(Func &&func)
	{
		return wrapSelectDelegateFunction<DualTextMenuItem>(std::forward<Func>(func));
	}

	template<class Func>
	DualTextMenuItem(const char *str, const char *str2, Gfx::GlyphTextureSet *face, Func &&func):
		DualTextMenuItem{str, str2, face, makeSelectDelegate(std::forward<Func>(func))} {}

	template<class Func>
	void setOnSelect(Func &&func)
	{
		setOnSelect(makeSelectDelegate(std::forward<Func>(func)));
	}

protected:
	SelectDelegate selectD{};
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (BoolMenuItem &item, View &view, Input::Event e)>;

	BoolMenuItem();
	BoolMenuItem(const char *str, Gfx::GlyphTextureSet *face, bool val, SelectDelegate);
	BoolMenuItem(const char *str, Gfx::GlyphTextureSet *face, bool val, const char *offStr, const char *onStr, SelectDelegate);
	bool boolValue() const;
	bool setBoolValue(bool val, View &view);
	bool setBoolValue(bool val);
	bool flipBoolValue(View &view);
	bool flipBoolValue();
	void draw(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	bool select(View &view, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

	// SelectDelegate wrappers
	template<class Func>
	static SelectDelegate makeSelectDelegate(Func &&func)
	{
		return wrapSelectDelegateFunction<BoolMenuItem>(std::forward<Func>(func));
	}

	template<class Func>
	BoolMenuItem(const char *str, Gfx::GlyphTextureSet *face, bool val, Func &&func):
		BoolMenuItem{str, face, val, makeSelectDelegate(std::forward<Func>(func))} {}

	template<class Func>
	BoolMenuItem(const char *str, Gfx::GlyphTextureSet *face, bool val, const char *offStr, const char *onStr, Func &&func):
		BoolMenuItem{str, face, val, offStr, onStr, makeSelectDelegate(std::forward<Func>(func))} {}

	template<class Func>
	void setOnSelect(Func &&func)
	{
		setOnSelect(makeSelectDelegate(std::forward<Func>(func)));
	}

protected:
	SelectDelegate selectD{};
	const char *offStr = "Off", *onStr = "On";
	bool on = false;
	bool onOffStyle = true;
};

class MultiChoiceMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (MultiChoiceMenuItem &item, View &view, Input::Event e)>;
	using ItemsDelegate = DelegateFunc<uint32_t (const MultiChoiceMenuItem &item)>;
	using ItemDelegate = DelegateFunc<TextMenuItem& (const MultiChoiceMenuItem &item, uint32_t idx)>;
	using SetDisplayStringDelegate = DelegateFunc<bool(uint32_t idx, Gfx::Text &text)>;

	MultiChoiceMenuItem();
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate, int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate);
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate);
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate, int selected, ItemsDelegate items, ItemDelegate item);
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, int selected, ItemsDelegate items, ItemDelegate item);
	template <class C>
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, int selected, C &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{str, face, onDisplayStr, selected,
		[&item](const MultiChoiceMenuItem &) -> int
		{
			return std::size(item);
		},
		[&item](const MultiChoiceMenuItem &, uint32_t idx) -> TextMenuItem&
		{
			return std::data(item)[idx];
		},
		selectDel}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, int selected, C &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{str, face, SetDisplayStringDelegate{}, selected, item, selectDel}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, SetDisplayStringDelegate onDisplayStr, int selected, C &item):
		MultiChoiceMenuItem{str, face, onDisplayStr, selected, item, {}}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, Gfx::GlyphTextureSet *face, int selected, C &item):
		MultiChoiceMenuItem{str, face, SetDisplayStringDelegate{}, selected, item, {}}
	{}
	void draw(Gfx::RendererCommands &, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize,
		Gfx::GC xIndent, _2DOrigin align, const Gfx::ProjectionPlane &, Gfx::Color) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	int selected() const;
	uint32_t items() const;
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

	void setDisplayString(int idx);
};
