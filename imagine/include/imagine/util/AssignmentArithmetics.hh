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

namespace IG
{

template <class T>
class AssignmentAdds
{
public:
	constexpr friend T& operator+=(T &a, auto &&b)
	{
		a = a + b;
		return a;
	}

	constexpr bool operator ==(AssignmentAdds const& rhs) const = default;
};

template <class T>
class AssignmentSubtracts
{
public:
	constexpr friend T& operator-=(T &a, auto &&b)
	{
		a = a - b;
		return a;
	}

	constexpr bool operator ==(AssignmentSubtracts const& rhs) const = default;
};

template <class T>
class AssignmentMultiplies
{
public:
	constexpr friend T& operator*=(T &a, auto &&b)
	{
		a = a * b;
		return a;
	}

	constexpr bool operator ==(AssignmentMultiplies const& rhs) const = default;
};

template <class T>
class AssignmentDivides
{
public:
	constexpr friend T& operator/=(T &a, auto &&b)
	{
		a = a / b;
		return a;
	}

	constexpr bool operator ==(AssignmentDivides const& rhs) const = default;
};

template <class T>
class AssignmentArithmetics : public AssignmentAdds<T>, public AssignmentSubtracts<T>, public AssignmentMultiplies<T>, public AssignmentDivides<T>
{
public:
	constexpr bool operator ==(AssignmentArithmetics const& rhs) const = default;
};

}
