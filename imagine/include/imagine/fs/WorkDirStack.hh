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

#include <imagine/fs/sys.hh>
#include <imagine/logger/logger.h>

template <uint SIZE>
class WorkDirStack
{
	std::array<FsSys::PathString, SIZE> dir{{{{0}}}};
public:
	uint size = 0;
	constexpr WorkDirStack() {}

	void push()
	{
		assert(size < SIZE);
		string_copy(dir[size], FsSys::workDir());
		logMsg("pushed work dir %s", dir[size].data());
		size++;
	}

	void pop()
	{
		if(!size)
		{
			logWarn("no work dir in stack");
			return;
		}
		size--;
		logMsg("popped work dir %s", dir[size].data());
		FsSys::chdir(dir[size].data());
	}
};
