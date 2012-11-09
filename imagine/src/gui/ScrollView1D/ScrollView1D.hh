#pragma once

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

	State inputEvent(const Rect2<int> &bt, const InputEvent &e);
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
	bool inputEvent(const InputEvent &e);
	bool inputEvent(int minClip, int maxClip, const InputEvent &e);
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
	int inputEvent(const InputEvent &e);

private:
	Rect2<int> *contentFrame = nullptr;
};
