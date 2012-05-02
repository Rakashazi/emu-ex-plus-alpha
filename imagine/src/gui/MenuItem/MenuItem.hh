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
#include <util/Delegate.hh>

class MenuItem
{
public:
	constexpr MenuItem() { }
	virtual void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const = 0;
	virtual void compile() = 0;
	virtual int ySize() = 0;
	virtual GC xSize() = 0;
	virtual void deinit() = 0;
	virtual void select(View *parent, const InputEvent &e) { bug_exit("unimplemented select()"); };
};

class TextMenuItem : public MenuItem
{
public:
	constexpr TextMenuItem(): active(0) { }
	GfxText t;
	bool active;

	void init(const char *str, bool active = 1, ResourceFace *face = View::defaultFace);
	void deinit();
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
	void compile();
	int ySize();
	GC xSize();
	void select(View *parent, const InputEvent &e);

	typedef Delegate<void (TextMenuItem &item, const InputEvent &e)> SelectDelegate;
	SelectDelegate selectDel;
	SelectDelegate &selectDelegate() { return selectDel; }
};

class DualTextMenuItem : public TextMenuItem
{
public:
	constexpr DualTextMenuItem() { }
	GfxText t2;

	void init(const char *str, const char *str2, bool active = 1, ResourceFace *face = View::defaultFace);
	void deinit();
	void compile();
	void draw2ndText(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
};


class BoolMenuItem : public DualTextMenuItem
{
public:
	constexpr BoolMenuItem(): on(0) { }
	bool on;

	void init(const char *str, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void set(bool val);
	void toggle();
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;

	void select(View *parent, const InputEvent &e);
	typedef Delegate<void (BoolMenuItem &item, const InputEvent &e)> SelectDelegate;
	SelectDelegate selectDel;
	SelectDelegate &selectDelegate() { return selectDel; }
};

class BoolTextMenuItem : public BoolMenuItem
{
public:
	constexpr BoolTextMenuItem(): offStr(nullptr), onStr(nullptr) { }
	const char *offStr, *onStr;
	void init(const char *str, const char *offStr, const char *onStr, bool on, bool active = 1, ResourceFace *face = View::defaultFace);
	void set(bool val);
	void toggle();
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
};

class MultiChoiceMenuItem : public DualTextMenuItem
{
public:
	constexpr MultiChoiceMenuItem() : choice(0), choices(0), baseVal(0), choiceStr(0) { }
	int choice, choices, baseVal;
	const char **choiceStr;

	void init(const char *str, const char **choiceStr, int val, int max, int baseVal = 0, bool active = 1, const char *initialDisplayStr = 0, ResourceFace *face = View::defaultFace);
	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;
	bool updateVal(int val);
	void setVal(int val);
	bool set(int val, const InputEvent &e);
	//virtual void doSet(int val) { }
	virtual void doSet(int val) { valueDel.invoke(*this, val); }
	void cycle(int direction);

	typedef Delegate<void (MultiChoiceMenuItem &item, int val)> ValueDelegate;
	ValueDelegate valueDel;
	ValueDelegate &valueDelegate() { return valueDel; }
};
