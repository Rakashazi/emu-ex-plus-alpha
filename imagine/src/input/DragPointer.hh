#pragma once

#include <input/Input.hh>
#include <gfx/Gfx.hh>
#include <util/rectangle2.h>

class DragPointer
{
public:
	constexpr DragPointer() { }

	void pointerEvent(uint button, uint state, IG::Point2D<int> pos)
	{
		if(state == Input::PUSHED)
		{
			pushX = pos.x;
			pushY = pos.y;
			//time = gfx_frameTime;
			pushed = 1;
			//logMsg("pushX %d pushY %d", pushX, pushY);
		}
		else if(state == Input::RELEASED)
		{
			pushed = 0;
		}
		prevX = this->x;
		prevY = this->y;
		this->x = pos.x;
		this->y = pos.y;
		//logMsg("new drag state prevX %d prevY %d", prevX, prevY);
	}

	int relX() const
	{
		return x - prevX;
	}

	int relY() const
	{
		return y - prevY;
	}

	bool dragged() const
	{
		return dragX() != 0 || dragY() != 0;
	}

	/*bool draggedFromRect(const Rect<int> &r) const
	{
		return dragged() && r.overlaps(pushX, pushY);
	}*/

	bool draggedFromRect(const Rect2<int> &r) const
	{
		return dragged() && r.overlaps(pushX, pushY);
	}

	int dragX() const
	{
		if(pushed)
			return x - pushX;
		else
			return 0;
	}

	int dragY() const
	{
		if(pushed)
			return y - pushY;
		else
			return 0;
	}

	/*uint timeSincePush()
	{
		return gfx_frameTime - time;
	}*/

	int prevX = 0, prevY = 0; // location during previous update
	int pushX = 0, pushY = 0; // location of most recent push
	int x = 0, y = 0; // current location
	//int pid;
	//uint time;
	uint pushed = 0;
};

namespace Input
{
#ifdef INPUT_SUPPORTS_POINTER
	DragPointer *dragState(int p);
#else
	//static DragPointer *dragState(int p) { bugExit("input_dragState called with no platform support"); return 0; }
#endif
}
