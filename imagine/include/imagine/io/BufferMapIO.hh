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
#include <imagine/io/MapIO.hh>
#include <imagine/util/DelegateFunc.hh>

class BufferMapIO : public MapIO
{
public:
	// optional function to call on close
	using OnCloseDelegate = DelegateFunc<void (BufferMapIO &io)>;

	constexpr BufferMapIO() {}
	~BufferMapIO() override;
	BufferMapIO(BufferMapIO &&o);
	BufferMapIO &operator=(BufferMapIO &&o);
	operator GenericIO();
	CallResult open(const void *buff, size_t size, OnCloseDelegate onClose);
	CallResult open(const void *buff, size_t size)
	{
		return open(buff, size, {});
	}

	void close() override;

protected:
	OnCloseDelegate onClose;

	// no copying outside of class
	BufferMapIO(const BufferMapIO &) = default;
	BufferMapIO &operator=(const BufferMapIO &) = default;
};
