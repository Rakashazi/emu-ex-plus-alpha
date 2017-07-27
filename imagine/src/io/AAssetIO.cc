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
#include <unistd.h>
#include <sys/mman.h>
#include <imagine/io/AAssetIO.hh>
#include "utils.hh"
#include "../base/android/android.hh"
#include <imagine/logger/logger.h>

AAssetIO::~AAssetIO()
{
	close();
}

AAssetIO::AAssetIO(AAssetIO &&o)
{
	asset = o.asset;
	o.asset = {};
	mapIO = std::move(o.mapIO);
}

AAssetIO &AAssetIO::operator=(AAssetIO &&o)
{
	close();
	asset = o.asset;
	o.asset = {};
	mapIO = std::move(o.mapIO);
	return *this;
}

GenericIO AAssetIO::makeGeneric()
{
	return GenericIO{*this};
}

std::error_code AAssetIO::open(const char *name)
{
	logMsg("opening asset %s", name);
	asset = AAssetManager_open(Base::activityAAssetManager(), name, AASSET_MODE_BUFFER);
	if(!asset)
	{
		logErr("error in AAssetManager_open");
		return {EINVAL, std::system_category()};
	}

	// try to get a memory mapping
	const void *buff = AAsset_getBuffer(asset);
	if(buff)
	{
		auto size = AAsset_getLength(asset);
		if(!AAsset_isAllocated(asset) && size > 4096 && madvise(buff, size, MADV_SEQUENTIAL) != 0)
			logWarn("madvise failed");
		mapIO.open(buff, size);
		logMsg("mapped into memory");
	}
	return {};
}

ssize_t AAssetIO::read(void *buff, size_t bytes, std::error_code *ecOut)
{
	if(mapIO)
		return mapIO.read(buff, bytes, ecOut);
	auto bytesRead = AAsset_read(asset, buff, bytes);
	if(bytesRead < 0)
	{
		if(ecOut)
			*ecOut = {EIO, std::system_category()};
		return -1;
	}
	return bytesRead;
}

const char *AAssetIO::mmapConst()
{
	if(mapIO)
		return mapIO.mmapConst();
	else return nullptr;
}

ssize_t AAssetIO::write(const void *buff, size_t bytes, std::error_code *ecOut)
{
	if(ecOut)
		*ecOut = {ENOSYS, std::system_category()};
	return -1;
}

off_t AAssetIO::seek(off_t offset, IO::SeekMode mode, std::error_code *ecOut)
{
	if(mapIO)
		return mapIO.seek(offset, mode, ecOut);
	assumeExpr(isSeekModeValid(mode));
	auto newPos = AAsset_seek(asset, offset, mode);
	if(newPos < 0)
	{
		if(ecOut)
			*ecOut = {EINVAL, std::system_category()};;
		return -1;
	}
	return newPos;
}

void AAssetIO::close()
{
	mapIO.close();
	if(asset)
	{
		logMsg("closing asset: %p", asset);
		AAsset_close(asset);
		asset = nullptr;
	}
}

size_t AAssetIO::size()
{
	if(mapIO)
		return mapIO.size();
	return AAsset_getLength(asset);
}

bool AAssetIO::eof()
{
	if(mapIO)
		return mapIO.eof();
	return !AAsset_getRemainingLength(asset);
}

AAssetIO::operator bool()
{
	return asset;
}
