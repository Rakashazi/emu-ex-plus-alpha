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

#include <imagine/input/DragTracker.hh>

namespace IG::Input
{

void DragTrackerState::update(WPt pos, int dragStartPixels)
{
	assert(isTracking());
	pos_ = pos;
	if(!isDragging_ &&
		(std::abs(downPos_.x - pos.x) > dragStartPixels ||
		std::abs(downPos_.y - pos.y) > dragStartPixels))
	{
		isDragging_ = true;
	}
}

void DragTrackerState::finish()
{
	isDragging_ = false;
	id_ = NULL_POINTER_ID;
}

}
