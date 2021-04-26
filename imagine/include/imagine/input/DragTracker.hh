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
#include <imagine/util/Point2D.hh>
#include <imagine/util/container/ArrayList.hh>

namespace Input
{

class DragTrackerState
{
public:
	constexpr DragTrackerState() {}
	constexpr DragTrackerState(int id, IG::WP pos)
	{
		start(id, pos);
	}

	void start(int id, IG::WP pos)
	{
		id_ = id;
		downPos_ = pos_ = pos;
		isTracking_ = true;
	}

	void update(IG::WP pos);
	void finish();
	int id() const { return id_; }
	IG::WP pos() const { return pos_; }
	IG::WP downPos() const { return downPos_; }
	IG::WP downPosDiff() const { return pos_ - downPos_; }
	void setXDragStartDistance(uint32_t dist) { xDragStart = dist; }
	void setYDragStartDistance(uint32_t dist) { yDragStart = dist; }
	bool isDragging() const { return isDragging_; }
	bool isTracking() const { return isTracking_; }
	bool isTracking(int id) const { return isTracking_ && id_ == id; }

protected:
	int id_{};
	IG::WP pos_{};
	IG::WP downPos_{};
	uint32_t xDragStart{};
	uint32_t yDragStart{};
	bool isDragging_ = false;
	bool isTracking_ = false;
};

class DragTracker
{
public:
	constexpr DragTracker() {}

	template <class OnDown, class OnMove, class OnUp>
	bool inputEvent(Event e, OnDown onDown, OnMove onMove, OnUp onUp)
	{
		if(!e.isPointer())
			return false;
		auto pID = e.pointerID();
		switch(e.state())
		{
			case Input::Action::PUSHED:
			{
				if(state_.isFull() || e.mapKey() != Input::Pointer::LBUTTON)
					return false;
				DragTrackerState startState{e.pointerID(), e.pos()};
				state_.push_back(startState);
				onDown(startState);
				return false;
			}
			case Input::Action::MOVED:
			{
				auto s = std::find_if(state_.begin(), state_.end(), [pID](const auto &s){ return s.id() == pID; });
				if(s == state_.end())
					return false;
				auto prevState = *s;
				s->update(e.pos());
				onMove(*s, prevState);
				return s->isDragging();
			}
			case Input::Action::RELEASED:
			case Input::Action::CANCELED:
			{
				auto s = std::find_if(state_.begin(), state_.end(), [pID](const auto &s){ return s.id() == pID; });
				if(s == state_.end())
					return false;
				auto finishState = *s;
				state_.erase(s);
				auto prevState = finishState;
				finishState.update(e.pos());
				if(prevState.pos() != finishState.pos())
					onMove(finishState, prevState);
				onUp(finishState);
				return false;
			}
			default:
				return false;
		}
	}

	DragTrackerState state(int id) const;

protected:
	StaticArrayList<DragTrackerState, Config::Input::MAX_POINTERS> state_{};
};

class SingleDragTracker
{
public:
	constexpr SingleDragTracker() {}

	template <class OnDown, class OnMove, class OnUp>
	bool inputEvent(Event e, OnDown onDown, OnMove onMove, OnUp onUp)
	{
		if(!e.isPointer())
			return false;
		auto pID = e.pointerID();
		switch(e.state())
		{
			case Input::Action::PUSHED:
			{
				if(state_.isTracking() || e.mapKey() != Input::Pointer::LBUTTON)
					return false;
				state_.start(pID, e.pos());
				onDown(state_);
				return false;
			}
			case Input::Action::MOVED:
			{
				if(!state_.isTracking(pID))
					return false;
				auto prevState = state_;
				state_.update(e.pos());
				onMove(state_, prevState);
				return state_.isDragging();
			}
			case Input::Action::RELEASED:
			case Input::Action::CANCELED:
			{
				if(!state_.isTracking(pID))
					return false;
				auto prevState = state_;
				state_.update(e.pos());
				if(prevState.pos() != state_.pos())
					onMove(state_, prevState);
				onUp(state_);
				state_.finish();
				return false;
			}
			default:
				return false;
		}
	}

	DragTrackerState state() const { return state_; }
	void finish() { state_.finish(); }
	void setXDragStartDistance(uint32_t dist) { state_.setXDragStartDistance(dist); }
	void setYDragStartDistance(uint32_t dist) { state_.setYDragStartDistance(dist); }
	bool isDragging() const { return state_.isDragging(); }

protected:
	DragTrackerState state_{};
};

}
