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

#define LOGTAG "BufferMapIO"
#include <imagine/io/BufferMapIO.hh>
#include <imagine/logger/logger.h>

BufferMapIO::~BufferMapIO()
{
	close();
}

BufferMapIO::BufferMapIO(BufferMapIO &&o)
{
	*this = o;
	o.resetData();
}

BufferMapIO &BufferMapIO::operator=(BufferMapIO &&o)
{
	close();
	*this = o;
	o.resetData();
	return *this;
}

BufferMapIO::operator GenericIO()
{
	return GenericIO{*this};
}

std::error_code BufferMapIO::open(const void *buff, size_t size, OnCloseDelegate onClose)
{
	close();
	setData(buff, size);
	this->onClose = onClose;
	return {};
}

void BufferMapIO::close()
{
	if(data)
	{
		if(onClose)
		{
			onClose(*this);
			onClose = {};
		}
		resetData();
	}
}

