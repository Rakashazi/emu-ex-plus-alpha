#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/io/MapIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

using namespace IG;

template<class ON_KEY>
static bool readConfigKeys(MapIO io, ON_KEY onKey)
{
	if(!io)
		return false;

	auto configSize = io.size();
	if(configSize < 4)
	{
		logErr("skipping config of only %d bytes", (int)configSize);
		return false;
	}

	auto blockSize = io.get<uint8_t>();
	auto fileBytesLeft = configSize - 1;

	if(blockSize != 2)
	{
		logErr("can't read config with block size %d", blockSize);
		return false;
	}

	while(!io.eof() && fileBytesLeft >= 2)
	{
		size_t size = io.get<uint16_t>();
		auto nextBlockPos = io.tell() + size;

		if(!size)
		{
			logMsg("invalid 0 size block, skipping rest of config");
			return false;
		}

		if(size > fileBytesLeft)
		{
			logErr("size:%zu of key exceeds rest of file (%zu bytes), skipping rest of config", size, fileBytesLeft);
			return false;
		}
		fileBytesLeft -= size;

		if(size < 2) // all blocks are at least a 2 byte key
		{
			logMsg("skipping %zu byte block", size);
			if(io.seek(nextBlockPos) == -1)
			{
				logErr("unable to seek to next block, skipping rest of config");
				return false;
			}
			continue;
		}

		auto key = io.get<uint16_t>();
		size -= 2;

		logMsg("got config key %u, size %zu", key, size);
		auto ioView = io.subView(io.tell(), size);
		onKey(key, ioView);

		if(io.seek(nextBlockPos) == -1)
		{
			logErr("unable to seek to next block, skipping rest of config");
			return false;
		}
	}
	return true;
}

static void writeConfigHeader(FileIO &io)
{
	uint8_t blockHeaderSize = 2;
	io.put(blockHeaderSize);
}

}
