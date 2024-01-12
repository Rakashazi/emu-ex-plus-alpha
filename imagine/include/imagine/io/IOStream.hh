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

#include <imagine/io/ioDefs.hh>
#include <streambuf>
#include <istream>
#include <ostream>
#include <memory>

namespace IG
{

class FileIO;
class IO;

// templates for using IO types with standard C++ stream-based IO

template <class IO>
class IOStreamBuf final : public std::streambuf
{
public:
	static constexpr size_t buffSize = 8192;

	constexpr IOStreamBuf() = default;

	IOStreamBuf(IO io_, std::ios::openmode mode):
		io{std::forward<IO>(io_)}
	{
		if(!io)
			return;
		if(mode & std::ios::in)
		{
			if(auto map = io.map();
				map.data())
			{
				setg((char_type*)map.data(), (char_type*)map.data(), (char_type*)map.data() + map.size());
			}
			else
			{
				buffPtr = std::make_unique<char_type[]>(buffSize);
				underflow();
			}
		}
		if(mode & std::ios::out)
		{
			setp(nullptr, nullptr);
		}
	}

	int_type underflow() final
	{
		if(!buffPtr)
		{
			return traits_type::eof();
		}
		auto bytesRead = io.read(buffPtr.get(), buffSize);
		if(bytesRead <= 0)
		{
			return traits_type::eof();
		}
		setg(buffPtr.get(), buffPtr.get(), buffPtr.get() + bytesRead);
		return buffPtr[0];
	}

	int_type overflow(int_type ch) final
	{
		char_type c = ch;
		return xsputn(&c, 1);
	}

	std::streamsize xsputn(const char_type *s, std::streamsize count) final
	{
		auto bytesWritten = io.write(s, count);
		if(bytesWritten == -1) [[unlikely]]
		{
			return 0;
		}
		return bytesWritten;
	}

	static IOSeekMode seekMode(std::ios::seekdir way)
	{
		switch(way)
		{
			default:
			case std::ios::beg: return IOSeekMode::Set;
			case std::ios::cur: return IOSeekMode::Cur;
			case std::ios::end: return IOSeekMode::End;
		}
	}

	pos_type seekoff(off_type off, std::ios::seekdir way, std::ios::openmode) final
	{
		return io.seek(off * sizeof(char_type), seekMode(way));
	}

	pos_type seekpos(pos_type sp, std::ios::openmode) final
	{
		return io.seek(sp * sizeof(char_type));
	}

	explicit operator bool() const { return (bool)io; }

protected:
	std::unique_ptr<char_type[]> buffPtr;
	IO io;
};

template <class IO, class StdStream>
class StreamBase : public StdStream
{
public:
	StreamBase() = default;

	StreamBase(IO io, std::ios::openmode mode = openMode()):
		StdStream{&streamBuf},
		streamBuf{std::forward<IO>(io), mode | implicitOpenMode()}
	{
		if(!streamBuf) [[unlikely]]
			this->setstate(std::ios::failbit);
	}

	StreamBase(StreamBase&& o) noexcept
	{
		*this = std::move(o);
	}

	StreamBase& operator=(StreamBase&& o) noexcept
	{
		StdStream::operator=(std::move(o));
		streamBuf = std::move(o.streamBuf);
		rdbuf(&streamBuf);
		return *this;
	}

	static constexpr std::ios::openmode openMode()
	{
		if constexpr(std::is_same_v<StdStream, std::iostream>)
			return std::ios::in | std::ios::out;
		else
			return implicitOpenMode();
	}

	static constexpr std::ios::openmode implicitOpenMode()
	{
		if constexpr(std::is_same_v<StdStream, std::istream>)
			return std::ios::in;
		else if constexpr(std::is_same_v<StdStream, std::ostream>)
			return std::ios::out;
		else
			return {};
	}

	bool is_open() const { return (bool)streamBuf; }

protected:
	IOStreamBuf<IO> streamBuf{};
};

template <class IO>
using IStream = StreamBase<IO, std::istream>;

template <class IO>
using OStream = StreamBase<IO, std::ostream>;

template <class IO>
using IOStream = StreamBase<IO, std::iostream>;

using IFStream = IStream<FileIO>;

using OFStream = OStream<FileIO>;

using FStream = IOStream<FileIO>;

using GenericIStream = IStream<IO>;

using GenericOStream = OStream<IO>;

using GenericIOStream = OStream<IO>;

}
