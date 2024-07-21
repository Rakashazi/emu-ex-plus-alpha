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
#include <imagine/util/string/CStringView.hh>
#include <memory>
#include <string_view>

struct AAssetManager;
struct AAssetDir;

namespace IG::FS
{

class AAssetDirectory
{
public:
	AAssetDirectory(AAssetManager *, CStringView path);
	bool readNextDir();
	bool hasEntry() const;
	std::string_view name() const;
	PathString path() const;

protected:
	struct AAssetDirDeleter
	{
		void operator()(AAssetDir *ptr) const
		{
			closeAAssetDir(ptr);
		}
	};
	using UniqueAAssetDir = std::unique_ptr<AAssetDir, AAssetDirDeleter>;

	UniqueAAssetDir dir{};
	const char *entryName{};
	PathString basePath{};

	static void closeAAssetDir(AAssetDir *);
};

class AAssetIterator
{
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = AAssetDirectory;
	using difference_type = ptrdiff_t;
	using pointer = value_type*;
	using reference = value_type&;

	constexpr AAssetIterator() = default;
	AAssetIterator(AAssetManager *, CStringView path);
	AAssetIterator(const AAssetIterator&) = default;
	AAssetIterator(AAssetIterator&&) = default;
	AAssetDirectory& operator*();
	AAssetDirectory* operator->();
	void operator++();
	bool operator==(AAssetIterator const &rhs) const;

protected:
	std::shared_ptr<AAssetDirectory> impl;
};

inline const AAssetIterator &begin(const AAssetIterator &iter)
{
	return iter;
}

inline AAssetIterator end(const AAssetIterator &)
{
	return {};
}

}
