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

#include <imagine/fs/FSDefs.hh>
#if defined __linux__ && !defined __ANDROID__
#include <libgen.h>
#undef basename
#include <string.h> // use GNU version of basename() that doesn't modify argument
constexpr bool dirnameCanModifyArgument = true;
#else
// Bionic or BSD
#include <libgen.h>
constexpr bool dirnameCanModifyArgument = false;
#endif

namespace IG::FS
{

static auto basenameImpl(const char *path)
{
	return [](auto path)
	{
		if constexpr(requires {::basename(path);})
		{
			// Bionic or GNU C versions take const char*
			return ::basename(path);
		}
		else
		{
			// BSD version takes char *, but always returns its own allocated storage
			return ::basename(PathString{path}.data());
		}
	}(path);
}

static auto dirnameImpl(const char *path)
{
	return [](auto path)
	{
		if constexpr(requires {::dirname(path);})
		{
			// Bionic version takes const char*
			return ::dirname(path);
		}
		else
		{
			if constexpr(dirnameCanModifyArgument)
			{
				// standard version can modify input, and returns a pointer within it
				PathString tempPath{path};
				FileString output{::dirname(tempPath.data())};
				return output;
			}
			else
			{
				// BSD version takes char *, but always returns its own allocated storage
				return ::dirname(PathString{path}.data());
			}
		}
	}(path);
}

}

// make sure basename macro doesn't leak outside
#undef basename
