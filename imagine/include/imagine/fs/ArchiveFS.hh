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

#include <imagine/config/defs.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/io/ArchiveIO.hh>
#include <imagine/util/string/CStringView.hh>
#include <memory>
#include <iterator>
#include <string_view>

namespace IG
{
class IO;
}

namespace IG::FS
{

class ArchiveIterator
{
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = ArchiveEntry;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	constexpr ArchiveIterator() = default;
	ArchiveIterator(CStringView path);
	ArchiveIterator(IO);
	ArchiveIterator(ArchiveEntry);
	ArchiveIterator(const ArchiveIterator&) = default;
	ArchiveIterator(ArchiveIterator&&) = default;
	ArchiveIterator &operator=(ArchiveIterator &&o) = default;
	ArchiveEntry& operator*();
	ArchiveEntry* operator->();
	void operator++();
	bool operator==(ArchiveIterator const &rhs) const;
	void rewind();
	bool hasEntry() const { return (bool)impl; }

private:
	std::shared_ptr<ArchiveEntry> impl;
};

static const ArchiveIterator &begin(const ArchiveIterator &iter)
{
	return iter;
}

static ArchiveIterator end(const ArchiveIterator &)
{
	return {};
}

ArchiveIO fileFromArchive(CStringView archivePath, std::string_view filePath);
ArchiveIO fileFromArchive(IO archiveIO, std::string_view filePath);
bool hasArchiveExtension(std::string_view name);

};
