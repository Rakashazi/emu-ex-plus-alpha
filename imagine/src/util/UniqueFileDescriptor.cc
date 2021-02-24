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

#define LOGTAG "UniqueFileDescriptor"
#include <imagine/config/defs.hh>
#include <imagine/util/UniqueFileDescriptor.hh>
#include <imagine/logger/logger.h>
#include <unistd.h>
#include <errno.h>
#include <utility>
#include <cstring>

namespace IG
{

UniqueFileDescriptor::UniqueFileDescriptor(UniqueFileDescriptor &&o)
{
	*this = std::move(o);
}

UniqueFileDescriptor &UniqueFileDescriptor::operator=(UniqueFileDescriptor &&o)
{
	reset();
	fd_ = o.release();
	return *this;
}

UniqueFileDescriptor::~UniqueFileDescriptor()
{
	reset();
}

int UniqueFileDescriptor::release()
{
	return std::exchange(fd_, -1);
}

void UniqueFileDescriptor::reset()
{
	if(fd_ == -1)
		return;
	auto fd = std::exchange(fd_, -1);
	//logDMsg("closing fd:%d", fd);
	if(::close(fd) == -1 && Config::DEBUG_BUILD)
	{
		logErr("close(%d) error: %s", fd, strerror(errno));
	}
}


}
