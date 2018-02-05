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

#include <initializer_list>
#include <vector>
#include <imagine/config/defs.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/DelegateFunc.hh>

class MenuItem
{
public:
	bool isSelectable = true;

	constexpr MenuItem() {}
	constexpr MenuItem(bool isSelectable):
		isSelectable{isSelectable} {}
	virtual ~MenuItem() {};
	virtual void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const = 0;
	virtual void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) = 0;
	virtual int ySize() = 0;
	virtual Gfx::GC xSize() = 0;
	virtual bool select(View &parent, Input::Event e) = 0;
};

class BaseTextMenuItem : public MenuItem
{
public:
	Gfx::Text t{};

	BaseTextMenuItem() {}
	BaseTextMenuItem(const char *str):
		t{str, &View::defaultFace} {}
	BaseTextMenuItem(const char *str, Gfx::GlyphTextureSet *face):
		t{str, face} {}
	BaseTextMenuItem(const char *str, bool isSelectable):
		MenuItem(isSelectable),
		t{str, &View::defaultFace} {}
	BaseTextMenuItem(const char *str, bool isSelectable, Gfx::GlyphTextureSet *face):
		MenuItem(isSelectable),
		t{str, face} {}
	void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	int ySize() override;
	Gfx::GC xSize() override;
	void setActive(bool on);
	bool active();

protected:
	bool active_ = true;
};

class TextMenuItem : public BaseTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<bool (TextMenuItem &item, View &parent, Input::Event e)>;

	TextMenuItem() {}
	TextMenuItem(const char *str, SelectDelegate selectDel): BaseTextMenuItem{str}, selectD{selectDel} {}
	template<class FUNC>
	TextMenuItem(const char *str, FUNC &&selectDel): TextMenuItem{str, wrapSelectDelegate(selectDel)} {}
	bool select(View &parent, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);
	template<class FUNC>
	void setOnSelect(FUNC &&onSelect) { setOnSelect(wrapSelectDelegate(onSelect)); }
	SelectDelegate onSelect() const { return selectD; }
	template<class FUNC = SelectDelegate>
	static SelectDelegate wrapSelectDelegate(FUNC selectDel)
	{
		constexpr auto args = IG::functionTraitsArity<FUNC>;
		constexpr auto returnsVoid = std::is_same<void, IG::functionTraitsRType<FUNC>>::value;
		constexpr auto returnsBool = std::is_same<bool, IG::functionTraitsRType<FUNC>>::value;
		if constexpr(returnsVoid && args == 3)
		{
			// for void (TextMenuItem &, View &, Input::Event)
			return
				[=](TextMenuItem &item, View &parent, Input::Event e)
				{
					selectDel(item, parent, e);
					return true;
				};
		}
		else if constexpr(returnsBool && args == 0)
		{
			// for bool ()
			return
				[=](TextMenuItem &item, View &parent, Input::Event e)
				{
					return selectDel();
				};
		}
		else if constexpr(returnsVoid && args == 0)
		{
			// for void ()
			return
				[=](TextMenuItem &item, View &parent, Input::Event e)
				{
					selectDel();
					return true;
				};
		}
		else
		{
			// for bool (TextMenuItem &, View &, Input::Event)
			return selectDel;
		}
	}

protected:
	SelectDelegate selectD{};
};

class TextHeadingMenuItem : public BaseTextMenuItem
{
public:
	TextHeadingMenuItem() {}
	TextHeadingMenuItem(const char *str): BaseTextMenuItem{str, false, &View::defaultBoldFace} {}
	bool select(View &parent, Input::Event e) override { return true; };
};

class BaseDualTextMenuItem : public BaseTextMenuItem
{
public:
	Gfx::Text t2{};

	BaseDualTextMenuItem() {}
	BaseDualTextMenuItem(const char *str, const char *str2):
		BaseTextMenuItem{str},
		t2{str2, &View::defaultFace} {}
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	void draw2ndText(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const;
	void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (DualTextMenuItem &item, View &parent, Input::Event e)>;

	DualTextMenuItem() {}
	DualTextMenuItem(const char *str, const char *str2): BaseDualTextMenuItem{str, str2} {}
	DualTextMenuItem(const char *str, const char *str2, SelectDelegate selectDel): BaseDualTextMenuItem{str, str2}, selectD{selectDel} {}
	bool select(View &parent, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD{};
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (BoolMenuItem &item, View &parent, Input::Event e)>;

	BoolMenuItem() {}
	BoolMenuItem(const char *str, bool val, SelectDelegate selectDel);
	BoolMenuItem(const char *str, bool val, const char *offStr, const char *onStr, SelectDelegate selectDel);
	bool boolValue() const;
	bool setBoolValue(bool val, View &view);
	bool setBoolValue(bool val);
	bool flipBoolValue(View &view);
	bool flipBoolValue();
	void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	bool select(View &parent, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);

protected:
	SelectDelegate selectD{};
	const char *offStr = "Off", *onStr = "On";
	bool on = false;
	bool onOffStyle = true;
};

class MultiChoiceMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (MultiChoiceMenuItem &item, View &parent, Input::Event e)>;
	using ItemsDelegate = DelegateFunc<uint (const MultiChoiceMenuItem &item)>;
	using ItemDelegate = DelegateFunc<TextMenuItem& (const MultiChoiceMenuItem &item, uint idx)>;
	using SetDisplayStringDelegate = DelegateFunc<const char *(uint idx)>;

	MultiChoiceMenuItem() {}
	MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr, int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel);
	MultiChoiceMenuItem(const char *str, int selected, ItemsDelegate items, ItemDelegate item, SelectDelegate selectDel);
	MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr, int selected, ItemsDelegate items, ItemDelegate item);
	MultiChoiceMenuItem(const char *str, int selected, ItemsDelegate items, ItemDelegate item);
	template <class C>
	MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr, int selected, C &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{str, onDisplayStr, selected,
		[&item](const MultiChoiceMenuItem &) -> int
		{
			return IG::size(item);
		},
		[&item](const MultiChoiceMenuItem &, uint idx) -> TextMenuItem&
		{
			return IG::data(item)[idx];
		},
		selectDel}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, int selected, C &item, SelectDelegate selectDel):
		MultiChoiceMenuItem{str, SetDisplayStringDelegate{}, selected, item, selectDel}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, SetDisplayStringDelegate onDisplayStr, int selected, C &item):
		MultiChoiceMenuItem{str, onDisplayStr, selected, item, {}}
	{}
	template <class C>
	MultiChoiceMenuItem(const char *str, int selected, C &item):
		MultiChoiceMenuItem{str, SetDisplayStringDelegate{}, selected, item, {}}
	{}
	void draw(Gfx::Renderer &r, Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	void compile(Gfx::Renderer &r, const Gfx::ProjectionPlane &projP) override;
	int selected() const;
	uint items() const;
	bool setSelected(int idx, View &view);
	bool setSelected(int idx);
	int cycleSelected(int offset, View &view);
	int cycleSelected(int offset);
	bool select(View &parent, Input::Event e) override;
	void setOnSelect(SelectDelegate onSelect);
	TableView *makeTableView(ViewAttachParams attach);
	void defaultOnSelect(View &view, Input::Event e);

protected:
	int selected_ = 0;
	SelectDelegate selectD{};
	ItemsDelegate items_{};
	ItemDelegate item_{};
	SetDisplayStringDelegate onSetDisplayString{};

	void setDisplayString(int idx);
};
