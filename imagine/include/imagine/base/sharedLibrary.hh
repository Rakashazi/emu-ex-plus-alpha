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

#include <imagine/util/bitset.hh>
#include <type_traits>

namespace Base
{

using SharedLibraryRef = void*;
static constexpr unsigned RESOLVE_ALL_SYMBOLS_FLAG = IG::bit(0);

SharedLibraryRef openSharedLibrary(const char *name, unsigned flags = 0);
void closeSharedLibrary(SharedLibraryRef lib);
void *loadSymbol(SharedLibraryRef lib, const char *name);

template<class T>
static bool loadSymbol(T &symPtr, SharedLibraryRef lib, const char *name)
{
	static_assert(std::is_pointer_v<T>, "called loadSymbol() without pointer type");
	symPtr = (T)loadSymbol(lib, name);
	return symPtr;
}

}
