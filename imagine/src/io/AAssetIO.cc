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

#define LOGTAG "AAssetIO"
#include <imagine/io/AAssetIO.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"
#include <android/asset_manager.h>
#include <unistd.h>
#include <sys/mman.h>

namespace IG
{

static int accessHintToAAssetMode(IO::AccessHint advice)
{
	switch(advice)
	{
		default: return AASSET_MODE_UNKNOWN;
		case IO::AccessHint::SEQUENTIAL: return AASSET_MODE_STREAMING;
		case IO::AccessHint::RANDOM: return AASSET_MODE_RANDOM;
		case IO::AccessHint::ALL: return AASSET_MODE_BUFFER;
	}
}

AAssetIO::AAssetIO(ApplicationContext ctx, IG::CStringView name, AccessHint access, unsigned openFlags):
	asset{AAssetManager_open(ctx.aAssetManager(), name, accessHintToAAssetMode(access))}
{
	if(!asset) [[unlikely]]
	{
		logErr("error in AAssetManager_open(%s, %s)", name.data(), accessHintStr(access));
		if(openFlags & OPEN_TEST)
			return;
		else
			throw std::runtime_error{fmt::format("Error opening asset: {}", name)};
	}
	logMsg("opened asset:%p name:%s access:%s", asset.get(), name.data(), accessHintStr(access));
	if(access == IO::AccessHint::ALL)
		makeMapIO();
}

ssize_t AAssetIO::read(void *buff, size_t bytes)
{
	if(mapIO)
		return mapIO.read(buff, bytes);
	auto bytesRead = AAsset_read(asset.get(), buff, bytes);
	if(bytesRead < 0) [[unlikely]]
	{
		return -1;
	}
	return bytesRead;
}

ssize_t AAssetIO::readAtPos(void *buff, size_t bytes, off_t offset)
{
	if(mapIO)
		return mapIO.readAtPos(buff, bytes, offset);
	else
		return IO::readAtPos(buff, bytes, offset);
}

std::span<uint8_t> AAssetIO::map()
{
	if(makeMapIO())
		return mapIO.map();
	else
		return {};
}

ssize_t AAssetIO::write(const void *buff, size_t bytes)
{
	return -1;
}

off_t AAssetIO::seek(off_t offset, IO::SeekMode mode)
{
	if(mapIO)
		return mapIO.seek(offset, mode);
	auto newPos = AAsset_seek(asset.get(), offset, (int)mode);
	if(newPos < 0) [[unlikely]]
	{
		return -1;
	}
	return newPos;
}

size_t AAssetIO::size()
{
	if(mapIO)
		return mapIO.size();
	return AAsset_getLength(asset.get());
}

bool AAssetIO::eof()
{
	if(mapIO)
		return mapIO.eof();
	return !AAsset_getRemainingLength(asset.get());
}

AAssetIO::operator bool() const
{
	return (bool)asset;
}

void AAssetIO::advise(off_t offset, size_t bytes, Advice advice)
{
	if(offset == 0 && bytes == 0 && advice == IO::Advice::WILLNEED)
	makeMapIO();
	if(mapIO)
		mapIO.advise(offset, bytes, advice);
}

bool AAssetIO::makeMapIO()
{
	if(mapIO)
		return true;
	const void *buff = AAsset_getBuffer(asset.get());
	if(!buff)
	{
		logErr("error in AAsset_getBuffer(%p)", asset.get());
		return false;
	}
	auto size = AAsset_getLength(asset.get());
	mapIO = {IG::ByteBuffer{{(uint8_t*)buff, (size_t)size}}};
	return true;
}

IG::ByteBuffer AAssetIO::releaseBuffer()
{
	if(!makeMapIO())
		return {};
	auto map = mapIO.map();
	logMsg("releasing asset:%p with buffer:%p (%zu bytes)", asset.get(), map.data(), map.size());
	return {map,
		[asset = asset.release()](const uint8_t *ptr, size_t)
		{
			logMsg("closing released asset:%p", asset);
			AAsset_close(asset);
		}
	};
}

void AAssetIO::closeAAsset(AAsset *asset)
{
	if(!asset)
		return;
	logMsg("closing asset:%p", asset);
	AAsset_close(asset);
}

}
