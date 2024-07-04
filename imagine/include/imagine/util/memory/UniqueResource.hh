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

#pragma once

#include <concepts>
#include <utility>

namespace IG
{

template <class T, std::invocable<T> Deleter, T NULL_VALUE = T()>
class UniqueResource
{
public:
	constexpr UniqueResource() = default;

	constexpr UniqueResource(T r, Deleter del):
		r{r}, del{del} {}

	constexpr UniqueResource(T r):
		r{r} {}

	constexpr UniqueResource(Deleter del):
		del{del} {}

	UniqueResource(UniqueResource &&o) noexcept
	{
		*this = std::move(o);
	}

	UniqueResource &operator=(UniqueResource &&o) noexcept
	{
		reset();
		r = o.release();
		del = o.del;
		return *this;
	}

	~UniqueResource()
	{
		reset();
	}

	constexpr T release() { return std::exchange(r, NULL_VALUE); }

	void reset()
	{
		if(r == NULL_VALUE)
			return;
		del(release());
	}

	constexpr auto& get(this auto&& self) { return self.r; }
	constexpr auto& get_deleter(this auto&& self) { return self.del; }
	constexpr operator const T&() const { return r; }
	constexpr auto operator->(this auto&& self) { return &self.r; }

protected:
	T r{NULL_VALUE};
	[[no_unique_address]] Deleter del{};
};

}
