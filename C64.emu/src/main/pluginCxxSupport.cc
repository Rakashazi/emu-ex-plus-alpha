/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <cstdlib>
#include <new>

// minimal set of C++ symbols needed by reSID

void* operator new (std::size_t size) { return std::malloc(size); }

void* operator new[] (std::size_t size) { return std::malloc(size); }

void operator delete (void *o) noexcept { std::free(o); }

void operator delete[] (void *o) noexcept { std::free(o); }

void operator delete (void *o, std::size_t) noexcept { std::free(o); }

void operator delete[] (void *o, std::size_t) noexcept { std::free(o); }
