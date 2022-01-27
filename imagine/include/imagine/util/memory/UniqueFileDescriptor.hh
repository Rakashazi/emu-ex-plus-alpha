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

#pragma once

#include <imagine/util/memory/UniqueResource.hh>
#include <unistd.h>

namespace IG
{

struct FileDescriptorDeleter
{
	void operator()(int fd) const
	{
		::close(fd);
	}
};

using UniqueFileDescriptor = UniqueResource<int, FileDescriptorDeleter, -1>;

struct OwnsFileDescriptorDeleter
{
	bool ownsFd{};

	void operator()(int fd) const
	{
		if(ownsFd)
			::close(fd);
	}
};

class MaybeUniqueFileDescriptor : public UniqueResource<int, OwnsFileDescriptorDeleter, -1>
{
public:
	using UniqueResource::UniqueResource;

	constexpr MaybeUniqueFileDescriptor(int fd):
		UniqueResource{fd, {false}} {}

	MaybeUniqueFileDescriptor(UniqueFileDescriptor fd):
		UniqueResource{fd.release(), {true}} {}

	constexpr bool ownsFd() const { return get_deleter().ownsFd; }
};

}
