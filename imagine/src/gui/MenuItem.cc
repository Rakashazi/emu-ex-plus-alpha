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

#include <imagine/gui/MenuItem.hh>

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

void BaseTextMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	if(!active)
	{
		// half-bright color
		uint col = color();
		setColor(ColorFormat.r(col)/2, ColorFormat.g(col)/2, ColorFormat.b(col)/2, ColorFormat.a(col));
	}

	if(ColorFormat.a(color()) == 0xFF)
	{
		//logMsg("using replace program for non-alpha modulated text");
		texAlphaReplaceProgram.use();
	}
	else
		texAlphaProgram.use();

	if(align.isXCentered())
		xPos += xSize/2;
	else
		xPos += TableView::globalXIndent;
	t.draw(xPos, yPos, align, projP);
}

void BaseTextMenuItem::compile(const Gfx::ProjectionPlane &projP) { t.compile(projP); }
int BaseTextMenuItem::ySize() { return t.face->nominalHeight(); }
Gfx::GC BaseTextMenuItem::xSize() { return t.xSize; }
void TextMenuItem::select(View &parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	if(selectD)
	{
		auto del = selectD;
		del(*this, parent, e);
	}
}

void DualTextMenuItem::select(View &parent, const Input::Event &e)
{
	//logMsg("calling delegate");
	if(selectD)
	{
		auto del = selectD;
		del(*this, parent, e);
	}
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

void BaseDualTextMenuItem::compile(const Gfx::ProjectionPlane &projP)
{
	BaseTextMenuItem::compile(projP);
	if(t2.str)
	{
		t2.compile(projP);
	}
}

void BaseDualTextMenuItem::draw2ndText(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	Gfx::texAlphaProgram.use();
	t2.draw((xPos + xSize) - TableView::globalXIndent, yPos, RC2DO, projP);
}

void BaseDualTextMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	if(t2.str)
		BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

void BoolMenuItem::init(const char *str, bool on, bool active, ResourceFace *face)
{
	BaseDualTextMenuItem::init(str, on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::init(const char *str, const char *offStr, const char *onStr, bool on, bool active, ResourceFace *face)
{
	var_selfs(offStr);
	var_selfs(onStr);
	onOffStyle = false;
	BaseDualTextMenuItem::init(str, on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::init(bool on, bool active, ResourceFace *face)
{
	BaseDualTextMenuItem::init(on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::init(const char *offStr, const char *onStr, bool on, bool active, ResourceFace *face)
{
	var_selfs(offStr);
	var_selfs(onStr);
	onOffStyle = false;
	BaseDualTextMenuItem::init(on ? onStr : offStr, active, face);
	var_selfs(on);
}

void BoolMenuItem::set(bool val, View &view)
{
	if(val != on)
	{
		//logMsg("setting bool: %d", val);
		on = val;
		t2.setString(val ? onStr : offStr);
		t2.compile(view.projP);
		view.postDraw();
	}
}

void BoolMenuItem::toggle(View &view)
{
	if(on)
		set(0, view);
	else
		set(1, view);
}

void BoolMenuItem::select(View &parent, const Input::Event &e)
{
	if(selectD)
	{
		auto del = selectD;
		del(*this, parent, e);
	}
}

void BoolMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	if(!onOffStyle) // custom strings
		setColor(0., .8, 1.);
	else if(on)
		setColor(.27, 1., .27);
	else
		setColor(1., .27, .27);
	draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

void MultiChoiceMenuItem::init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	val -= baseVal;
	if(!initialDisplayStr)
	{
		assert(val >= 0);
	}
	if(str)
		BaseDualTextMenuItem::init(str, initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
	else
		BaseDualTextMenuItem::init(initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
	if(val >= max)
	{
		bug_exit("%d exceeds max %d", val, max);
	}
	choice = val;
	choices = max;
	this->baseVal = baseVal;
	this->choiceStr = choiceStr;
}

void MultiChoiceMenuItem::init(const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
{
	init(nullptr, choiceStr, val, max, baseVal, active, initialDisplayStr, face);
}

void MultiChoiceMenuItem::draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const
{
	using namespace Gfx;
	BaseTextMenuItem::draw(xPos, yPos, xSize, ySize, align, projP);
	//setColor(0., 1., 1.); // aqua
	setColor(0., .8, 1.);
	BaseDualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align, projP);
}

bool MultiChoiceMenuItem::updateVal(int val, View &view)
{
	if(val < 0 || val >= choices)
	{
		bug_exit("value %d out of range for %d choices", val, choices);
	}
	if(val != choice)
	{
		choice = val;
		t2.setString(choiceStr[val]);
		t2.compile(view.projP);
		view.postDraw();
		return 1;
	}
	return 0;
}

void MultiChoiceMenuItem::setVal(int val, View &view)
{
	if(updateVal(val, view))
	{
		doSet(val + baseVal, view);
	}
}

bool MultiChoiceMenuItem::set(int val, const Input::Event &e, View &view)
{
	setVal(val, view);
	return 1;
}

void MultiChoiceMenuItem::cycle(int direction, View &view)
{
	if(direction > 0)
		setVal(IG::incWrapped(choice, choices), view);
	else if(direction < 0)
		setVal(IG::decWrapped(choice, choices), view);
}
