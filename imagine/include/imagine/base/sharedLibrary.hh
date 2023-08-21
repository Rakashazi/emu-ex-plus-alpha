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

#include <imagine/util/concepts.hh>
#include <type_traits>

namespace IG
{

using SharedLibraryRef = void*;

struct OpenSharedLibraryFlags
{
	uint8_t
	resolveAllSymbols:1{};
};

SharedLibraryRef openSharedLibrary(const char *name, OpenSharedLibraryFlags flags = {});
void closeSharedLibrary(SharedLibraryRef lib);
void *loadSymbol(SharedLibraryRef lib, const char *name);
const char *lastOpenSharedLibraryError();

static bool loadSymbol(Pointer auto &symPtr, SharedLibraryRef lib, const char *name)
{
	symPtr = reinterpret_cast<std::remove_reference_t<decltype(symPtr)>>(loadSymbol(lib, name));
	return symPtr;
}

}
