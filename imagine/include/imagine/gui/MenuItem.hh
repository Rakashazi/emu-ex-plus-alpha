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

#include <imagine/engine-globals.h>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gui/View.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/DelegateFunc.hh>

class MenuItem
{
public:
	constexpr MenuItem() {}
	constexpr MenuItem(bool isSelectable):
		isSelectable{isSelectable}
		{}
	bool isSelectable = true;
	virtual void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const = 0;
	virtual void compile(const Gfx::ProjectionPlane &projP) = 0;
	virtual int ySize() = 0;
	virtual Gfx::GC xSize() = 0;
	virtual void deinit() = 0;
	virtual void select(View &parent, const Input::Event &e) { bug_exit("unimplemented select()"); };
};

class BaseTextMenuItem : public MenuItem
{
public:
	Gfx::Text t;
	bool active = true;

	constexpr BaseTextMenuItem() {}
	constexpr BaseTextMenuItem(const char *str): t(str) {}
	constexpr BaseTextMenuItem(const char *str, bool isSelectable):
		MenuItem(isSelectable),
		t(str)
		{}
	void init(const char *str, bool active, ResourceFace *face = View::defaultFace);
	void init(const char *str, ResourceFace *face = View::defaultFace);
	void init(bool active, ResourceFace *face = View::defaultFace);
	void init();
	void deinit() override;
	void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	void compile(const Gfx::ProjectionPlane &projP) override;
	int ySize() override;
	Gfx::GC xSize() override;
};

class TextMenuItem : public BaseTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (TextMenuItem &item, View &parent, const Input::Event &e)>;
	SelectDelegate selectD;

	constexpr TextMenuItem() {}
	constexpr TextMenuItem(const char *str): BaseTextMenuItem(str) {}
	constexpr TextMenuItem(SelectDelegate selectDel): selectD(selectDel) {}
	constexpr TextMenuItem(const char *str, SelectDelegate selectDel): BaseTextMenuItem(str), selectD(selectDel) {}
	void select(View &parent, const Input::Event &e) override;
	SelectDelegate &onSelect() { return selectD; }
};

class TextHeadingMenuItem : public BaseTextMenuItem
{
public:
	constexpr TextHeadingMenuItem() {}
	constexpr TextHeadingMenuItem(const char *str): BaseTextMenuItem(str, false) {}
	void select(View &parent, const Input::Event &e) override {};
};

class BaseDualTextMenuItem : public BaseTextMenuItem
{
public:
	Gfx::Text t2;

	constexpr BaseDualTextMenuItem() {}
	constexpr BaseDualTextMenuItem(const char *str): BaseTextMenuItem(str) {}
	void init(const char *str, const char *str2, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *str2, bool active = 1, ResourceFace *face = View::defaultFace);
	void deinit() override;
	void compile(const Gfx::ProjectionPlane &projP) override;
	void draw2ndText(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const;
	void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	using SelectDelegate = DelegateFunc<void (DualTextMenuItem &item, View &parent, const Input::Event &e)>;
	SelectDelegate selectD;

	constexpr DualTextMenuItem() {}
	constexpr DualTextMenuItem(const char *str): BaseDualTextMenuItem(str) {}
	constexpr DualTextMenuItem(SelectDelegate selectDel): selectD(selectDel) {}
	constexpr DualTextMenuItem(const char *str, SelectDelegate selectDel): BaseDualTextMenuItem(str), selectD(selectDel) {}
	void select(View &parent, const Input::Event &e) override;
	SelectDelegate &onSelect() { return selectD; }
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	typedef DelegateFunc<void (BoolMenuItem &item, View &parent, const Input::Event &e)> SelectDelegate;
	SelectDelegate selectD;
	SelectDelegate &onSelect() { return selectD; }
	const char *offStr = "Off", *onStr = "On";
	bool on = 0;
	bool onOffStyle = true;

	constexpr BoolMenuItem() {}
	constexpr BoolMenuItem(const char *str): BaseDualTextMenuItem(str) {}
	constexpr BoolMenuItem(const char *str, const char *offStr, const char *onStr): BaseDualTextMenuItem(str),
		offStr(offStr), onStr(onStr) {}
	constexpr BoolMenuItem(SelectDelegate selectDel): selectD(selectDel) {}
	constexpr BoolMenuItem(const char *str, SelectDelegate selectDel): BaseDualTextMenuItem(str), selectD(selectDel) {}
	constexpr BoolMenuItem(const char *str, const char *offStr, const char *onStr, SelectDelegate selectDel): BaseDualTextMenuItem(str),
		selectD(selectDel), offStr(offStr), onStr(onStr), onOffStyle(false) {}
	void init(const char *str, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *str, const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void set(bool val, View &view);
	void toggle(View &view);
	void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	void select(View &parent, const Input::Event &e) override;
};

class MultiChoiceMenuItem : public DualTextMenuItem
{
public:
	typedef DelegateFunc<void (MultiChoiceMenuItem &item, View &parent, int val)> ValueDelegate;
	ValueDelegate valueD;
	ValueDelegate &onValue() { return valueD; }
	const char **choiceStr = nullptr;
	int choice = 0, choices = 0, baseVal = 0;

	constexpr MultiChoiceMenuItem() {}
	constexpr MultiChoiceMenuItem(const char *str): DualTextMenuItem(str) {}
	constexpr MultiChoiceMenuItem(ValueDelegate valueD): valueD(valueD) {}
	constexpr MultiChoiceMenuItem(const char *str, ValueDelegate valueD): DualTextMenuItem(str), valueD(valueD) {}
	void init(const char *str, const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace);
	void init(const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace);
	void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	bool updateVal(int val, View &view);
	void setVal(int val, View &view);
	bool set(int val, const Input::Event &e, View &view);
	virtual void doSet(int val, View &view) { valueD(*this, view, val); }
	void cycle(int direction, View &view);
};
