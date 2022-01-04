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
#include <imagine/util/algorithm.h>

namespace IG::Input
{

class DragTrackerState
{
public:
	constexpr DragTrackerState() = default;
	constexpr DragTrackerState(PointerId id, IG::WP pos)
	{
		start(id, pos);
	}

	constexpr void start(PointerId id, IG::WP pos)
	{
		id_ = id;
		downPos_ = pos_ = pos;
		isDragging_ = false;
	}

	void update(IG::WP pos, int dragStartPixels);
	void finish();
	constexpr PointerId id() const { return id_; }
	constexpr IG::WP pos() const { return pos_; }
	constexpr IG::WP downPos() const { return downPos_; }
	constexpr IG::WP downPosDiff() const { return pos_ - downPos_; }
	constexpr bool isDragging() const { return isDragging_; }
	constexpr bool isTracking() const { return id_ != NULL_POINTER_ID; }
	constexpr bool isTracking(PointerId id) const { return id_ == id; }

protected:
	PointerId id_{NULL_POINTER_ID};
	IG::WP pos_{};
	IG::WP downPos_{};
	bool isDragging_ = false;
};

struct EmptyUserData {};

template <class UserData = EmptyUserData, size_t MAX_POINTERS = Config::Input::MAX_POINTERS>
class DragTracker
{
public:
	struct State
	{
		DragTrackerState dragState{};
		[[no_unique_address]] UserData data{};

		constexpr State() {}
		constexpr State(DragTrackerState dragState, UserData data):
			dragState{dragState}, data{data} {}
	};

	constexpr DragTracker() = default;

	bool inputEvent(Event e,
		IG::invocable<DragTrackerState, UserData&> auto &&onDown,
		IG::invocable<DragTrackerState, DragTrackerState, UserData&> auto &&onMove,
		IG::invocable<DragTrackerState, UserData&> auto &&onUp)
	{
		if(!e.isPointer())
			return false;
		auto pID = e.pointerId();
		switch(e.state())
		{
			case Input::Action::PUSHED:
			{
				if(state_.isFull())
					return false;
				auto &startState = state_.emplace_back(DragTrackerState{pID, e.pos()}, UserData{});
				onDown(startState.dragState, startState.data);
				return false;
			}
			case Input::Action::MOVED:
			{
				auto s = state(pID);
				if(!s)
					return false;
				auto &[dragState, userData] = *s;
				auto prevDragState = dragState;
				dragState.update(e.pos(), dragStartPixels);
				onMove(dragState, prevDragState, userData);
				return dragState.isDragging();
			}
			case Input::Action::RELEASED:
			case Input::Action::CANCELED:
			{
				auto s = state(pID);
				if(!s)
					return false;
				auto [finishDragState, userData] = *s;
				state_.erase(s);
				auto prevDragState = finishDragState;
				finishDragState.update(e.pos(), dragStartPixels);
				if(prevDragState.pos() != finishDragState.pos())
					onMove(finishDragState, prevDragState, userData);
				onUp(finishDragState, userData);
				return false;
			}
			default:
				return false;
		}
	}

	void reset()
	{
		state_.clear();
	}

	auto &stateList()
	{
		return state_;
	}

	bool isDragging() const { return state_.size() && state_[0].dragState.isDragging(); }

	void setDragStartPixels(int p) { dragStartPixels = p; }

protected:
	StaticArrayList<State, MAX_POINTERS> state_{};
	int dragStartPixels{};

	State *state(PointerId id)
	{
		auto s = IG::find_if(state_, [id](const auto &s){ return s.dragState.id() == id; });
		if(s == state_.end())
		{
			return nullptr;
		}
		return s;
	}
};

template <class UserData = EmptyUserData>
using SingleDragTracker = DragTracker<UserData, 1>;

}
