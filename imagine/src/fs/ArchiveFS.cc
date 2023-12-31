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

#define LOGTAG "ArchFS"
#include <imagine/fs/ArchiveFS.hh>
#include <imagine/io/IO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/util/utility.h>
#include <imagine/util/string.h>

namespace IG::FS
{

template <class... Args>
static std::shared_ptr<ArchiveIO> makeArchiveEntryPtr(Args&& ...args)
{
	ArchiveIO entry{std::forward<Args>(args)...};
	if(entry.hasEntry())
	{
		return std::make_shared<ArchiveIO>(std::move(entry));
	}
	else
	{
		// empty archive
		return {};
	}
}

ArchiveIterator::ArchiveIterator(CStringView path):
	impl{makeArchiveEntryPtr(path)} {}

ArchiveIterator::ArchiveIterator(IO io):
	impl{makeArchiveEntryPtr(std::move(io))} {}

ArchiveIterator::ArchiveIterator(FileIO io):
	impl{makeArchiveEntryPtr(std::move(io))} {}

ArchiveIterator::ArchiveIterator(ArchiveIO entry):
	impl{entry.hasEntry() ? std::make_shared<ArchiveIO>(std::move(entry)) : std::shared_ptr<ArchiveIO>{}} {}

ArchiveIO& ArchiveIterator::operator*()
{
	return *impl;
}

ArchiveIO* ArchiveIterator::operator->()
{
	return impl.get();
}

void ArchiveIterator::operator++()
{
	assumeExpr(impl->hasEntry()); // incrementing end-iterator is undefined
	impl->readNextEntry();
}

void ArchiveIterator::rewind()
{
	impl->rewind();
}

bool hasArchiveExtension(std::string_view name)
{
	return endsWithAnyCaseless(name, ".7z", ".rar", ".zip");
}

}
