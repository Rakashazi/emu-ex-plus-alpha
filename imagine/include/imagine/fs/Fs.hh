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

#include <imagine/engine-globals.h>
#include <imagine/util/bits.h>

typedef int(*FsDirFilterFunc)(const char *name, int type);
typedef int(*FsDirSortFunc)(const char *name1, long int mtime1, const char *name2, long int mtime2);

class Fs
{
public:
	static constexpr uint OPEN_UNSORT = IG::bit(0);
	enum { TYPE_NONE = 0, TYPE_FILE, TYPE_DIR };

	constexpr Fs() {}
	virtual uint numEntries() const = 0;
	virtual const char *entryFilename(uint index) const = 0;
	virtual void closeDir() = 0;

	static int sortMTime(const char *name1, long int mtime1, const char *name2, long int mtime2)
	{
		if(mtime1 < mtime2)
			return 1;
		else if(mtime1 > mtime2)
			return -1;
		else
			return 0;
	}
};
