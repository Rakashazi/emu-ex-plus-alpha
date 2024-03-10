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

#include <imagine/io/IOUtils.hh>
#include <imagine/io/MapIO.hh>
#include <imagine/util/string/CStringView.hh>
#include <memory>
#include <span>

struct AAsset;

namespace IG
{
class ApplicationContext;
}

namespace IG
{

class AAssetIO : public IOUtils<AAssetIO>
{
public:
	using IOUtilsBase = IOUtils<AAssetIO>;
	using IOUtilsBase::read;
	using IOUtilsBase::write;
	using IOUtilsBase::seek;
	using IOUtilsBase::tell;
	using IOUtilsBase::send;
	using IOUtilsBase::buffer;
	using IOUtilsBase::get;
	using IOUtilsBase::toFileStream;

	constexpr AAssetIO() = default;
	AAssetIO(ApplicationContext, CStringView name, OpenFlags oFlags = {});
	ssize_t read(void *buff, size_t bytes, std::optional<off_t> offset = {});
	ssize_t write(const void *buff, size_t bytes, std::optional<off_t> offset = {});
	std::span<uint8_t> map();
	off_t seek(off_t offset, SeekMode mode);
	size_t size();
	bool eof();
	explicit operator bool() const;
	void advise(off_t offset, size_t bytes, Advice advice);
	IOBuffer releaseBuffer();

protected:
	struct AAssetDeleter
	{
		void operator()(AAsset *ptr) const
		{
			closeAAsset(ptr);
		}
	};
	using UniqueAAsset = std::unique_ptr<AAsset, AAssetDeleter>;

	UniqueAAsset asset{};
	MapIO mapIO{};

	bool makeMapIO();
	static void closeAAsset(AAsset *);
};

}
