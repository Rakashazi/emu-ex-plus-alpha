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

#include <unistd.h> // for SEEK_*

namespace IG::IODefs
{

enum class Advice
{
	NORMAL, SEQUENTIAL, RANDOM, WILLNEED
};

enum class AccessHint
{
	NORMAL, SEQUENTIAL, RANDOM, ALL, UNMAPPED
};

enum class BufferMode
{
	DIRECT, // may point directly to mapped memory, not valid after IO is destroyed
	RELEASE // may take IO's underlying memory and is always valid, invalidates IO object
};

enum class SeekMode
{
	SET = SEEK_SET,
	CUR = SEEK_CUR,
	END = SEEK_END,
};

using OpenFlags = unsigned;

}
