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

#include <imagine/input/Input.hh>
#include <imagine/util/rectangle2.h>

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

	bool draggedFromRect(const IG::WindowRect &r) const
	{
		return dragged() && r.overlaps({pushX, pushY});
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

	IG::Point2D<int> dragOffset() const
	{
		return {dragX(), dragY()};
	}

	IG::Point2D<int> pushPos() const
	{
		return {pushX, pushY};
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
	DragPointer *dragState(int p);
}
