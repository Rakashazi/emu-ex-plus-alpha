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
#include <imagine/util/utility.h>
#include <imagine/util/string.h>

namespace IG::FS
{

template <class... Args>
static std::shared_ptr<IG::ArchiveEntry> makeArchiveEntryPtr(Args&& ...args)
{
	IG::ArchiveEntry entry{std::forward<Args>(args)...};
	if(entry.hasEntry())
	{
		return std::make_shared<IG::ArchiveEntry>(std::move(entry));
	}
	else
	{
		// empty archive
		return {};
	}
}

ArchiveIterator::ArchiveIterator(IG::CStringView path):
	impl{makeArchiveEntryPtr(path)}
{}

ArchiveIterator::ArchiveIterator(IG::IO io):
	impl{makeArchiveEntryPtr(std::move(io))}
{}

ArchiveIterator::ArchiveIterator(IG::ArchiveEntry entry):
	impl{entry.hasEntry() ? std::make_shared<IG::ArchiveEntry>(std::move(entry)) : std::shared_ptr<IG::ArchiveEntry>{}}
{}

IG::ArchiveEntry& ArchiveIterator::operator*()
{
	return *impl;
}

IG::ArchiveEntry* ArchiveIterator::operator->()
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

static IG::ArchiveIO fileFromArchiveGeneric(auto &&init, std::string_view filePath)
{
	for(auto &entry : FS::ArchiveIterator{std::forward<decltype(init)>(init)})
	{
		if(entry.type() == FS::file_type::directory)
		{
			continue;
		}
		if(entry.name() == filePath)
		{
			return entry.moveIO();
		}
	}
	return {};
}

IG::ArchiveIO fileFromArchive(IG::CStringView archivePath, std::string_view filePath)
{
	return fileFromArchiveGeneric(archivePath, filePath);
}

IG::ArchiveIO fileFromArchive(IG::IO io, std::string_view filePath)
{
	return fileFromArchiveGeneric(std::move(io), filePath);
}

bool hasArchiveExtension(std::string_view name)
{
	return endsWithAnyCaseless(name, ".7z", ".rar", ".zip");
}

}
