#include <gui/MenuItem/MenuItem.hh>

void TextMenuItem::init(const char *str, bool active, ResourceFace *face)
{
	t.init(str, face);
	this->active = active;
}

void TextMenuItem::deinit()
{
	t.deinit();
}

void TextMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
{
	using namespace Gfx;
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

void TextMenuItem::compile() { t.compile(); }
int TextMenuItem::ySize() { return t.face->nominalHeight(); }
GC TextMenuItem::xSize() { return t.xSize; }
void TextMenuItem::select(View *parent, const InputEvent &e)
{
	//logMsg("calling delegate");
	selectDel.invokeSafe(*this, e);
}

	void DualTextMenuItem::init(const char *str, const char *str2, bool active, ResourceFace *face)
	{
		TextMenuItem::init(str, active, face);
		t2.init(str2, face);
	}

	void DualTextMenuItem::deinit()
	{
		t2.deinit();
		TextMenuItem::deinit();
	}

	void DualTextMenuItem::compile()
	{
		TextMenuItem::compile();
		t2.compile();
	}

	void DualTextMenuItem::draw2ndText(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		t2.draw((xPos + xSize) - GuiTable1D::globalXIndent, yPos, RC2DO, LT2DO);
	}

	void DualTextMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
	}

	void BoolMenuItem::init(const char *str, bool on, bool active, ResourceFace *face)
	{
		DualTextMenuItem::init(str, on ? "On" : "Off", active, face);
		this->on = on;
	}

	void BoolMenuItem::set(bool val)
	{
		if(val != on)
		{
			//logMsg("setting bool: %d", val);
			on = val;
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

	void BoolMenuItem::select(View *parent, const InputEvent &e) { selectDel.invokeSafe(*this, e); }

	void BoolMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		if(on)
			setColor(0., 1., 0., 1.);
		else
			setColor(1., 0., 0., 1.);
		draw2ndText(xPos, yPos, xSize, ySize, align);
	}

	void BoolTextMenuItem::init(const char *str, const char *offStr, const char *onStr, bool on, bool active, ResourceFace *face)
	{
		var_selfs(offStr);
		var_selfs(onStr);
		DualTextMenuItem::init(str, on ? onStr : offStr, active, face);
		var_selfs(on);
	}

	void BoolTextMenuItem::set(bool val)
	{
		if(val != on)
		{
			//logMsg("setting bool: %d", val);
			on = val;
			t2.setString(val ? onStr : offStr);
			t2.compile();
			Base::displayNeedsUpdate();
		}
	}

	void BoolTextMenuItem::toggle()
	{
		if(on)
			set(0);
		else
			set(1);
	}

	void BoolTextMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		setColor(0., 1., 1.); // aqua
		draw2ndText(xPos, yPos, xSize, ySize, align);
	}


	void MultiChoiceMenuItem::init(const char *str, const char **choiceStr, int val, int max, int baseVal, bool active, const char *initialDisplayStr, ResourceFace *face)
	{
		val -= baseVal;
		if(!initialDisplayStr) assert(val >= 0);
		DualTextMenuItem::init(str, initialDisplayStr ? initialDisplayStr : choiceStr[val], active, face);
		assert(val < max);
		choice = val;
		choices = max;
		this->baseVal = baseVal;
		this->choiceStr = choiceStr;
	}

	void MultiChoiceMenuItem::draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const
	{
		using namespace Gfx;
		TextMenuItem::draw(xPos, yPos, xSize, ySize, align);
		setColor(0., 1., 1.); // aqua
		DualTextMenuItem::draw2ndText(xPos, yPos, xSize, ySize, align);
	}

	bool MultiChoiceMenuItem::updateVal(int val)
	{
		assert(val >= 0 && val < choices);
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

	bool MultiChoiceMenuItem::set(int val, const InputEvent &e)
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
