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
#include <gui/View.hh>

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

	State inputEvent(const IG::Rect2<int> &bt, const Input::Event &e);
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

	void init(const IG::Rect2<int> *viewFrame, const IG::Rect2<int> *contentFrame);
	void place(View &view);
	bool clipOverEdge(int minC, int maxC, View &view);
	void clipDragOverEdge(int minC, int maxC);
	void decel2(View &view);
	bool inputEvent(const Input::Event &e, View &view);
	bool inputEvent(int minClip, int maxClip, const Input::Event &e, View &view);
	void setOffset(int o);
	void animate(int minClip, int maxClip, View &view);

private:
	const IG::Rect2<int> *viewFrame = nullptr;
	const IG::Rect2<int> *contentFrame = nullptr;
};

class ScrollView1D
{
public:
	constexpr ScrollView1D() { }
	KScroll scroll;
	IG::Rect2<int> viewFrame;
	IG::Rect2<int> scrollBarRect;
	bool contentIsBiggerThanView = 0;

	void init(IG::Rect2<int> *contentFrame);
	void updateView(); // move content frame in position along view frame
	void place(IG::Rect2<int> *frame, View &view);
	void updateGfx(View &view);
	void draw();
	int inputEvent(const Input::Event &e, View &view);

private:
	IG::Rect2<int> *contentFrame = nullptr;
};
