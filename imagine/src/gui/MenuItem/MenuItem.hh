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

#include <gfx/GfxText.hh>
#include <gui/View.hh>
#include <gui/GuiTable1D/GuiTable1D.hh>
#include <util/DelegateFunc.hh>

class MenuItem
{
public:
	constexpr MenuItem() { }
	virtual void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const = 0;
	virtual void compile() = 0;
	virtual int ySize() = 0;
	virtual GC xSize() = 0;
	virtual void deinit() = 0;
	virtual void select(View *parent, const Input::Event &e) { bug_exit("unimplemented select()"); };
};

class BaseTextMenuItem : public MenuItem
{
public:
	constexpr BaseTextMenuItem() { }
	constexpr BaseTextMenuItem(const char *str): t(str) { }

	Gfx::Text t;
	bool active = 1;

	void init(const char *str, bool active, ResourceFace *face = View::defaultFace);
	void init(const char *str, ResourceFace *face = View::defaultFace);
	void init(bool active, ResourceFace *face = View::defaultFace);
	void init();
	void deinit() override;
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
	void compile() override;
	int ySize() override;
	GC xSize() override;
};

class TextMenuItem : public BaseTextMenuItem
{
public:
	typedef DelegateFunc<void (TextMenuItem &item, const Input::Event &e)> SelectDelegate;
	SelectDelegate selectD;
	SelectDelegate &onSelect() { return selectD; }

	constexpr TextMenuItem() { }
	constexpr TextMenuItem(const char *str): BaseTextMenuItem(str) { }
	constexpr TextMenuItem(SelectDelegate selectDel): selectD(selectDel) { }
	constexpr TextMenuItem(const char *str, SelectDelegate selectDel): BaseTextMenuItem(str), selectD(selectDel) { }

	void select(View *parent, const Input::Event &e) override;
};

class BaseDualTextMenuItem : public BaseTextMenuItem
{
public:
	constexpr BaseDualTextMenuItem() { }
	constexpr BaseDualTextMenuItem(const char *str): BaseTextMenuItem(str) { }
	Gfx::Text t2;

	void init(const char *str, const char *str2, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *str2, bool active = 1, ResourceFace *face = View::defaultFace);
	void deinit() override;
	void compile() override;
	void draw2ndText(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
};

class DualTextMenuItem : public BaseDualTextMenuItem
{
public:
	typedef DelegateFunc<void (DualTextMenuItem &item, const Input::Event &e)> SelectDelegate;
	SelectDelegate selectD;
	SelectDelegate &onSelect() { return selectD; }

	constexpr DualTextMenuItem() { }
	constexpr DualTextMenuItem(const char *str): BaseDualTextMenuItem(str) { }
	constexpr DualTextMenuItem(SelectDelegate selectDel): selectD(selectDel) { }
	constexpr DualTextMenuItem(const char *str, SelectDelegate selectDel): BaseDualTextMenuItem(str), selectD(selectDel) { }

	void select(View *parent, const Input::Event &e) override;
};


class BoolMenuItem : public BaseDualTextMenuItem
{
public:
	typedef DelegateFunc<void (BoolMenuItem &item, const Input::Event &e)> SelectDelegate;
	constexpr BoolMenuItem() { }
	constexpr BoolMenuItem(const char *str): BaseDualTextMenuItem(str) { }
	constexpr BoolMenuItem(SelectDelegate selectDel): selectD(selectDel) { }
	constexpr BoolMenuItem(const char *str, SelectDelegate selectDel): BaseDualTextMenuItem(str), selectD(selectDel) { }
	bool on = 0;
	const char *offStr = nullptr, *onStr = nullptr;
	void init(const char *str, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *str, const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void init(const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void set(bool val);
	void toggle();
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;

	void select(View *parent, const Input::Event &e) override;
	SelectDelegate selectD;
	SelectDelegate &onSelect() { return selectD; }
};

class MultiChoiceMenuItem : public DualTextMenuItem
{
public:
	typedef DelegateFunc<void (MultiChoiceMenuItem &item, int val)> ValueDelegate;

	constexpr MultiChoiceMenuItem() { }
	constexpr MultiChoiceMenuItem(const char *str): DualTextMenuItem(str) { }
	constexpr MultiChoiceMenuItem(ValueDelegate valueD): valueD(valueD) { }
	constexpr MultiChoiceMenuItem(const char *str, ValueDelegate valueD): DualTextMenuItem(str), valueD(valueD) { }
	int choice = 0, choices = 0, baseVal = 0;
	const char **choiceStr = nullptr;

	void init(const char *str, const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace);
	void init(const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace);
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
	bool updateVal(int val);
	void setVal(int val);
	bool set(int val, const Input::Event &e);
	virtual void doSet(int val) { valueD(*this, val); }
	void cycle(int direction);

	ValueDelegate valueD;
	ValueDelegate &onValue() { return valueD; }
};
