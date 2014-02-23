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
#include <io/aasset/AAssetIO.hh>
#include <io/mmap/generic/IoMmapGeneric.hh>
#include <io/sys.hh>
#include <io/utils.hh>
#include <base/android/private.hh>
#include <logger/interface.h>

Io* AAssetIO::open(const char *name)
{
	logMsg("opening asset %s", name);
	AAsset *asset = AAssetManager_open(Base::activityAAssetManager(), name, AASSET_MODE_BUFFER);
	if(!asset)
	{
		logErr("error in AAssetManager_open");
		return nullptr;
	}

	// try to get a memory mapping
	const void *buff = AAsset_getBuffer(asset);
	if(buff)
	{
		auto size = AAsset_getLength(asset);
		if(!AAsset_isAllocated(asset) && madvise(buff, size, MADV_SEQUENTIAL) != 0)
			logWarn("madvise failed");
		auto mapIO = IoMmapGeneric::open(buff, size,
			[asset](IoMmapGeneric &)
			{
				logMsg("closing mapped asset @ %p", asset);
				AAsset_close(asset);
			});
		if(mapIO)
		{
			logMsg("mapped into memory");
			return mapIO;
		}
	}

	// fall back to using AAsset directly
	AAssetIO *inst = new AAssetIO;
	if(!inst)
	{
		logErr("out of memory");
		AAsset_close(asset);
		return nullptr;
	}
	inst->asset = asset;
	return inst;
}

void AAssetIO::close()
{
	if(asset)
	{
		logMsg("closing asset @ %p", asset);
		AAsset_close(asset);
		asset = nullptr;
	}
}

void AAssetIO::truncate(ulong offset) {}
void AAssetIO::sync() {}

ssize_t AAssetIO::readUpTo(void* buffer, size_t numBytes)
{
	auto bytesRead = AAsset_read(asset, buffer, numBytes);
	if(bytesRead < 0)
		bytesRead = 0;
	return bytesRead;
}

size_t AAssetIO::fwrite(const void* ptr, size_t size, size_t nmemb)
{
	return 0;
}

CallResult AAssetIO::tell(ulong &offset)
{
	offset = size() - AAsset_getRemainingLength(asset);
	return OK;
}

static int fdSeekMode(int mode)
{
	switch(mode)
	{
		case IO_SEEK_ABS: return SEEK_SET;
		case IO_SEEK_ABS_END: return SEEK_END;
		case IO_SEEK_REL: return SEEK_CUR;
		default:
			bug_branch("%d", mode);
			return 0;
	}
}

CallResult AAssetIO::seek(long offset, uint mode)
{
	if(!isSeekModeValid(mode))
	{
		bug_exit("invalid seek mode: %u", mode);
		return INVALID_PARAMETER;
	}
	if(AAsset_seek(asset, offset, fdSeekMode(mode)) >= 0)
	{
		return OK;
	}
	else
		return IO_ERROR;
}

ulong AAssetIO::size()
{
	return AAsset_getLength(asset);
}

int AAssetIO::eof()
{
	return !AAsset_getRemainingLength(asset);
}
