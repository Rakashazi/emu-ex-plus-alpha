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

#include <gfx/Gfx.hh>
#include <input/Input.hh>
#include <util/rectangle2.h>

class ContentDrag
{
public:
	constexpr ContentDrag() { }
	uint devId = 0;
	bool active = 0, pushed = 0;

	int dragStartX = 0, dragStartY = 0;

	enum { X_AXIS, Y_AXIS, XY_AXIS };
	uint axis = 0;

	void init(uint axis = Y_AXIS);

	bool testingXAxis() { return axis == X_AXIS || axis == XY_AXIS; }
	bool testingYAxis() { return axis == Y_AXIS || axis == XY_AXIS; }

	enum State { INACTIVE, PUSHED, ENTERED_ACTIVE, ACTIVE, LEFT_ACTIVE, RELEASED, NO_CHANGE };

	State inputEvent(const Rect2<int> &bt, const Input::Event &e);
};

class KScroll : public ContentDrag
{
public:
	constexpr KScroll() { }
	int start = 0, offset = 0;
	int prevOffset = 0;
	GC vel = 0;
	bool scrollWholeArea = 0, allowScrollWholeArea = 0;
	int maxClip = 0;

	void init(const Rect2<int> *viewFrame, const Rect2<int> *contentFrame);
	void place();
	bool clipOverEdge(int minC, int maxC);
	void clipDragOverEdge(int minC, int maxC);
	void decel2();
	bool inputEvent(const Input::Event &e);
	bool inputEvent(int minClip, int maxClip, const Input::Event &e);
	void setOffset(int o);
	void animate(int minClip, int maxClip);

private:
	const Rect2<int> *viewFrame = nullptr;
	const Rect2<int> *contentFrame = nullptr;
};

class ScrollView1D
{
public:
	constexpr ScrollView1D() { }
	KScroll scroll;
	Rect2<int> viewFrame;
	Rect2<int> scrollBarRect;
	bool contentIsBiggerThanView = 0;

	void init(Rect2<int> *contentFrame);
	void updateView(); // move content frame in position along view frame
	void place(Rect2<int> *frame);
	void updateGfx();
	void draw();
	int inputEvent(const Input::Event &e);

private:
	Rect2<int> *contentFrame = nullptr;
};
