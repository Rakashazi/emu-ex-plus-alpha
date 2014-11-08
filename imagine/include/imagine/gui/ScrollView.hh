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
#include <imagine/gfx/Gfx.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/gui/View.hh>

class ContentDrag
{
public:
	enum State { INACTIVE, PUSHED, ENTERED_ACTIVE, ACTIVE, LEFT_ACTIVE, RELEASED, NO_CHANGE };
	uint devId = 0;
	bool active = false, pushed = false;
	int dragStartX = 0, dragStartY = 0;
	enum { X_AXIS, Y_AXIS, XY_AXIS };
	uint axis = 0;

	constexpr ContentDrag() {}
	void init(uint axis = Y_AXIS);
	bool testingXAxis() { return axis == X_AXIS || axis == XY_AXIS; }
	bool testingYAxis() { return axis == Y_AXIS || axis == XY_AXIS; }
	State inputEvent(const IG::WindowRect &bt, const Input::Event &e);
};

class KScroll : public ContentDrag
{
public:
	int start = 0, offset = 0;
	int prevOffset = 0;
	Gfx::GC vel = 0;
	bool scrollWholeArea = 0, allowScrollWholeArea = 0;
	int maxClip = 0;

	constexpr KScroll() {}
	void init(const IG::WindowRect *viewFrame, const IG::WindowRect *contentFrame);
	void place(View &view);
	bool clipOverEdge(int minC, int maxC, View &view);
	void clipDragOverEdge(int minC, int maxC);
	void decel2(View &view);
	bool inputEvent(const Input::Event &e, View &view);
	bool inputEvent(int minClip, int maxClip, const Input::Event &e, View &view);
	void setOffset(int o);
	void animate(int minClip, int maxClip, View &view);

private:
	const IG::WindowRect *viewFrame = nullptr;
	const IG::WindowRect *contentFrame = nullptr;
};

class ScrollView : public View
{
public:
	using View::View;
	void init();
	bool isDoingScrollGesture() { return scroll.active; }

protected:
	KScroll scroll;
	IG::WindowRect contentSize;
	IG::WindowRect scrollBarRect;
	bool contentIsBiggerThanView = false;

	void setContentSize(IG::WP size);
	void updateGfx();
	void drawScrollContent();
	int scrollInputEvent(const Input::Event &e);
};
