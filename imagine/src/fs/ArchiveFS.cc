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
#include <imagine/util/string.h>

namespace FS
{

ArchiveIterator::ArchiveIterator(const char *path):
	impl{std::make_shared<ArchiveEntry>(path)}
{}

ArchiveIterator::ArchiveIterator(const char *path, std::error_code &ec):
	impl{std::make_shared<ArchiveEntry>(path, ec)}
{}

ArchiveIterator::ArchiveIterator(GenericIO io):
	impl{std::make_shared<ArchiveEntry>(std::move(io))}
{}

ArchiveIterator::ArchiveIterator(GenericIO io, std::error_code &ec):
	impl{std::make_shared<ArchiveEntry>(std::move(io), ec)}
{}

ArchiveIterator::ArchiveIterator(ArchiveEntry entry):
	impl{std::make_shared<ArchiveEntry>(std::move(entry))}
{}

ArchiveEntry& ArchiveIterator::operator*()
{
	return *impl;
}

ArchiveEntry* ArchiveIterator::operator->()
{
	return impl.get();
}

void ArchiveIterator::operator++()
{
	assumeExpr(impl); // incrementing end-iterator is undefined
	if(!impl->readNextEntry())
		impl.reset();
}

bool ArchiveIterator::operator==(ArchiveIterator const &rhs) const
{
	return impl == rhs.impl;
}

void ArchiveIterator::rewind()
{
	impl->rewind();
}

ArchiveIO fileFromArchive(const char *archivePath, const char *filePath)
{
	for(auto &entry : FS::ArchiveIterator{archivePath})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		if(string_equal(entry.name(), filePath))
		{
			return entry.moveIO();
		}
	}
	return {};
}

}
