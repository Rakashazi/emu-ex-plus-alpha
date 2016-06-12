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

namespace Input
{

void DragTrackerState::update(IG::WP pos)
{
	assert(isTracking_);
	pos_ = pos;
	if(!isDragging_ &&
		((uint)std::abs(downPos_.x - pos.x) > xDragStart ||
		(uint)std::abs(downPos_.y - pos.y) > yDragStart))
	{
		isDragging_ = true;
	}
}

void DragTrackerState::finish()
{
	isDragging_ = false;
	isTracking_ = false;
}

DragTrackerState DragTracker::state(int id) const
{
	auto s = std::find_if(state_.begin(), state_.end(), [id](DragTrackerState s){ return s.id() == id; });
	if(s == state_.end())
	{
		return {};
	}
	return *s;
}

}
