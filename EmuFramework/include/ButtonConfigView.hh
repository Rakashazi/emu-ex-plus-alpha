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

#pragma once

#include <util/gui/BaseMenuView.hh>
#include <AlertView.hh>
#include <EmuInput.hh>

void removeModalView();
extern View *modalView;
extern YesNoAlertView ynAlertView;
extern KeyMapping keyMapping;
void buildKeyMapping();

struct BtnConfigMenuItem : public DualTextMenuItem
{
	uint *btn = nullptr; uint dev = 0;
	void init(const char *name, uint *btn, uint dev);

	void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const;

	void select(View *view, const InputEvent &e);

	void setButton(const InputEvent &e);
};

static class ButtonConfigSetView : public View
{
	Rect2<int> viewFrame
	#ifndef CONFIG_BASE_PS3
	, unbindB, cancelB
	#endif
	;

	GfxText text
	#ifndef CONFIG_BASE_PS3
	, unbind, cancel
	#endif
	;

public:
	BtnConfigMenuItem *onSet = nullptr;
	uint devType = 0;

	Rect2<int> &viewRect() { return viewFrame; }

	void init(uint devType, BtnConfigMenuItem *onSet = 0)
	{
		this->devType = devType;
		if(!Config::envIsPS3)
			text.init("Push a key to set", View::defaultFace);
		else
			text.init("Push a key to set\n\nUse Square in previous screen to unbind", View::defaultFace);
		#ifndef CONFIG_BASE_PS3
		unbind.init("Unbind", View::defaultFace);
		cancel.init("Cancel", View::defaultFace);
		#endif
		this->onSet = onSet;
		Input::setHandleVolumeKeys(1);
	}

	void deinit()
	{
		text.deinit();
		#ifndef CONFIG_BASE_PS3
		unbind.deinit();
		cancel.deinit();
		#endif
		Input::setHandleVolumeKeys(0);
	}

	void place(Rect2<int> rect)
	{
		View::place(rect);
	}

	void place()
	{
		text.compile();

		#ifndef CONFIG_BASE_PS3
		unbind.compile();
		cancel.compile();

		unbindB.setPosRel(viewFrame.xPos(LB2DO), viewFrame.yPos(LB2DO),
				viewFrame.xSize()/2, Gfx::toIYSize(unbind.nominalHeight*2), LB2DO);

		cancelB.setPosRel(viewFrame.xPos(RB2DO), viewFrame.yPos(RB2DO),
				viewFrame.xSize()/2, Gfx::toIYSize(unbind.nominalHeight*2), RB2DO);
		#endif
	}

	void inputEvent(const InputEvent &e)
	{
		if(e.isPointer() && e.state == INPUT_RELEASED)
		{
			#ifndef CONFIG_BASE_PS3
			if(unbindB.overlaps(e.x, e.y))
			{
				logMsg("unbinding key");
				if(onSet)
					onSet->setButton(InputEvent());
				removeModalView();
			}
			else if(cancelB.overlaps(e.x, e.y))
			{
				removeModalView();
			}
			#endif
		}
		else if(!e.isPointer() && e.state == INPUT_PUSHED)
		{
			if((devType != InputEvent::DEV_KEYBOARD && e.devType != devType)
				|| (devType == InputEvent::DEV_KEYBOARD && (e.isGamepad())))
			{
				logMsg("ignoring input from device type %s", InputEvent::devTypeName(e.devType));
				return;
			}
			if(onSet)
				onSet->setButton(e);
			removeModalView();
		}
	}

	void draw()
	{
		using namespace Gfx;
		setBlendMode(0);
		resetTransforms();
		setColor(.4, .4, .4, 1.);
		GeomRect::draw(viewFrame);
		#ifndef CONFIG_BASE_PS3
		setColor(.2, .2, .2, 1.);
		GeomRect::draw(unbindB);
		GeomRect::draw(cancelB);
		#endif

		setColor(COLOR_WHITE);
		#ifndef CONFIG_BASE_PS3
		unbind.draw(gXPos(unbindB, C2DO), gYPos(unbindB, C2DO), C2DO);
		cancel.draw(gXPos(cancelB, C2DO), gYPos(cancelB, C2DO), C2DO);
		#endif
		text.draw(0, 0, C2DO);
	}
} btnSetView;



class ButtonConfigView : public BaseMenuView
{
	TextMenuItem reset;

	void inputEvent(const InputEvent &e);

	MenuItem **text = nullptr;
	BtnConfigMenuItem *btn = nullptr;

public:
	constexpr ButtonConfigView() { }
	KeyCategory *cat = nullptr;
	uint devType = 0;
	void init(KeyCategory *cat,
			uint devType, bool highlightFirst);
	void deinit();
	void confirmUnbindKeysAlert(const InputEvent &e);
	void resetHandler(TextMenuItem &, const InputEvent &e);
};

static ButtonConfigView bcMenu;
