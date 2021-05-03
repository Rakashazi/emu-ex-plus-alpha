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

#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/container/FlatSet.hh>
#include <imagine/util/algorithm.h>

template <typename FUNC>
class DelegateFuncSet
{
public:
	constexpr DelegateFuncSet() {}

	bool add(FUNC del, int priority = 0)
	{
		if(contains(del))
			return false;
		delegate.emplace(del, priority);
		return true;
	}

	bool remove(FUNC del)
	{
		auto it = find(del, delegate);
		if(it == delegate.end())
			return false;
		delegate.erase(it);
		return true;
	}

	bool contains(FUNC del) const
	{
		return find(del, delegate) != delegate.end();
	}

	unsigned size() const
	{
		return delegate.size();
	}

	template <typename Exec>
	void runAll(Exec exec, bool skipRemovedDelegates = false)
	{
		if(!size())
			return;
		DelegateEntry delegateCopy[delegate.size()];
		std::copy(delegate.begin(), delegate.end(), delegateCopy);
		for(auto &d : delegateCopy)
		{
			if(skipRemovedDelegates && !contains(d.del))
			{
				continue;
			}
			if(!exec(d.del))
			{
				IG::eraseFirst(delegate, d);
			}
		}
	}

protected:
	struct DelegateEntry
	{
		FUNC del{};
		int priority = 0;

		constexpr DelegateEntry() {}
		constexpr DelegateEntry(FUNC del, int priority):
			del{del}, priority{priority} {}
		bool operator==(const DelegateEntry &rhs) const { return del == rhs.del; }
		bool operator<(const DelegateEntry &rhs) const { return priority < rhs.priority; }
	};

	template <class Container>
	static auto find(FUNC del, Container &delegate)
	{
		return std::find_if(delegate.begin(), delegate.end(),
			[&](auto &e) { return e.del == del; });
	}

	IG::FlatMultiSet<DelegateEntry> delegate{};
};
