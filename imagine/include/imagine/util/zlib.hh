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

#include <span>
#include <bit>
#include <cstdint>
#include <zlib.h>

namespace IG
{

inline size_t compressGzip(std::span<uint8_t> dest, std::span<const uint8_t> src, int level)
{
	z_stream s{};
	s.avail_in = src.size();
	s.next_in = const_cast<z_const Bytef*>(src.data());
	s.avail_out = dest.size();
	s.next_out = dest.data();
  deflateInit2(&s, level, Z_DEFLATED, MAX_WBITS + 16, 8, Z_DEFAULT_STRATEGY);
  deflate(&s, Z_FINISH);
  deflateEnd(&s);
  return s.total_out;
}

inline size_t uncompressGzip(std::span<uint8_t> dest, std::span<const uint8_t> src)
{
	z_stream s{};
	s.avail_in = src.size();
	s.next_in = const_cast<z_const Bytef*>(src.data());
	s.avail_out = dest.size();
	s.next_out = dest.data();
  inflateInit2(&s, MAX_WBITS + 16);
  auto res = inflate(&s, Z_FINISH);
  inflateEnd(&s);
	if(res != Z_STREAM_END)
		return 0;
  return s.total_out;
}

inline bool hasGzipHeader(std::span<const uint8_t> buff)
{
	return buff.size() > 10 && buff[0] == 0x1F && buff[1] == 0x8B;
}

inline size_t gzipUncompressedSize(std::span<const uint8_t> buff)
{
	if(buff.size() < 18)
		return 0;
	using uint32u [[gnu::aligned(1)]] = uint32_t;
	uint32_t size = *reinterpret_cast<const uint32u*>(&*(buff.end() - 4));
	if constexpr(std::endian::native == std::endian::big)
		return std::byteswap(size);
	return size;
}

}
