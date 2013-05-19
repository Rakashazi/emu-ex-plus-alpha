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

#include <gui/MenuItem/MenuItem.hh>

void BaseTextMenuItem::init(const char *str, bool active, ResourceFace *face)
{
	t.init(str, face);
	this->active = active;
}

void BaseTextMenuItem::init(const char *str, ResourceFace *face)
{
	t.init(str, face);
}

void BaseTextMenuItem::init(bool active, ResourceFace *face)
{
	t.setFace(face);
	this->active = active;
}

void BaseTextMenuItem::init()
{
	t.setFace(View::defaultFace);
}

void BaseTextMenuItem::deinit()
{
	t.deinit();
}

void BaseTextMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
	setColor(COLOR_WHITE);
	if(!active)
	{
		uint col = color();
		setColor(ColorFormat.r(col)/2, ColorFormat.g(col)/2, ColorFormat.b(col)/2, ColorFormat.a(col));
	}

	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += GuiTable1D::globalXIndent;
	t.draw(xPos, yPos, align);
}

void BaseTextMenuItem::compile() { t.compile(); }
int BaseTextMenuItem::ySize() { return t.face->nominalHeight(); }
GC BaseTextMenuItem::xSize() { return t.xSize; }
void TextMenuItem::select(View *parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	if(selectD)
		selectD(*this, e);
}

void DualTextMenuItem::select(View *parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	if(selectD)
		selectD(*this, e);
}

void BaseDualTextMenuItem::init(const char *str, const char *str2, bool active, ResourceFace *face)
{
	BaseTextMenuItem::init(str, active, face);
	if(str2)
		t2.init(str2, face);
	else
		t2.init(face);
}

void BaseDualTextMenuItem::init(const char *str2, bool active, ResourceFace *face)
{
	BaseTextMenuItem::init(active, face);
	if(str2)
		t2.init(str2, face);
	else
		t2.init(face);
}

void BaseDualTextMenuItem::deinit()
{
	t2.deinit();
	BaseTextMenuItem::deinit();
}

void BaseDualTextMenuItem::compile()
{
	BaseTextMenuItem::compile();
	if(t2.str)
	{
		t2.compile();
	}
}

void BaseDualTextMenuItem::draw2ndText(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	t2.draw((xPos + xSize) - GuiTable1D::globalXIndent, yPos, RC2DO, LT2DO);
}

void BaseDualTextMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align);
	if(t2.str)
		BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
}

void BoolMenuItem::init(const char *str, bool on, bool active, ResourceFace *face)
{
	offStr = onStr = nullptr;
	BaseDualTextMenuItem::init(str, on ? "On" : "Off", active, face);
	var_selfs(on);
}

void BoolMenuItem::init(const char *str, const char *offStr, const char *onStr, bool on, bool active, ResourceFace *face)
{
	var_selfs(offStr);
	var_selfs(onStr);
	BaseDualTextMenuItem::init(str, on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::init(bool on, bool active, ResourceFace *face)
{
	offStr = onStr = nullptr;
	BaseDualTextMenuItem::init(on ? "On" : "Off", active, face);
	var_selfs(on);
}

void BoolMenuItem::init(const char *offStr, const char *onStr, bool on, bool active, ResourceFace *face)
{
	var_selfs(offStr);
	var_selfs(onStr);
	BaseDualTextMenuItem::init(on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::set(bool val)
{
	if(val != on)
	{
		//logMsg("setting bool: %d", val);
		on = val;
		if(onStr)
			t2.setString(val ? onStr : offStr);
		else
			t2.setString(val ? "On" : "Off");
		t2.compile();
		Base::displayNeedsUpdate();
	}
}

void BoolMenuItem::toggle()
{
	if(on)
		set(0);
	else
		set(1);
}

void BoolMenuItem::select(View *parent, const Input::Event &e) { if(selectD) selectD(*this, e); }

void BoolMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align);
	if(onStr) // custom strings
		setColor(0., 1., 1.); // aqua
	else if(on)
		setColor(0., 1., 0., 1.);
	else
		setColor(1., 0., 0., 1.);
	draw2ndText(xPos, yPos, xSize, ySize, align);
}

void MultiChoiceMenuItem::init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	val -= baseVal;
	if(!initialDisplayStr) assert(val >= 0);
	if(str)
		BaseDualTextMenuItem::init(str, initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
	else
		BaseDualTextMenuItem::init(initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
	assert(val < max);
	choice = val;
	choices = max;
	this->baseVal = baseVal;
	this->choiceStr = choiceStr;
}

void MultiChoiceMenuItem::init(const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	init(nullptr, choiceStr, val, max, baseVal, active, initialDisplayStr, face);
}

void MultiChoiceMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align);
	setColor(0., 1., 1.); // aqua
	BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
}

bool MultiChoiceMenuItem::updateVal(int val)
{
	if(val < 0 || val >= choices)
	{
		bug_exit("value %d out of range for %d choices", val, choices);
	}
	if(val != choice)
	{
		choice = val;
		t2.setString(choiceStr[val]);
		t2.compile();
		Base::displayNeedsUpdate();
		return 1;
	}
	return 0;
}

void MultiChoiceMenuItem::setVal(int val)
{
	if(updateVal(val))
	{
		doSet(val + baseVal);
	}
}

bool MultiChoiceMenuItem::set(int val, const Input::Event &e)
{
	setVal(val);
	return 1;
}

void MultiChoiceMenuItem::cycle(int direction)
{
	if(direction > 0)
		setVal(IG::incWrapped(choice, choices));
	else if(direction < 0)
		setVal(IG::decWrapped(choice, choices));
}
