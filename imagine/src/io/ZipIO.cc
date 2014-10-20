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

#define LOGTAG "ZipIO"
#include <imagine/io/ZipIO.hh>
#include <imagine/io/FileIO.hh>
#include <imagine/logger/logger.h>
#include "utils.hh"

static_assert(ZLIB_FILEFUNC_SEEK_CUR == SEEK_CUR
	&& ZLIB_FILEFUNC_SEEK_END == SEEK_END
	&& ZLIB_FILEFUNC_SEEK_SET == SEEK_SET,
	"Seek constants must match");

ZipIO::~ZipIO()
{
	close();
}

ZipIO::ZipIO(ZipIO &&o)
{
	zip = o.zip;
	zipIo = std::move(o.zipIo);
	uncompSize = o.uncompSize;
	o.zip = {};
	o.uncompSize = 0;
}

ZipIO &ZipIO::operator=(ZipIO &&o)
{
	close();
	zip = o.zip;
	zipIo = std::move(o.zipIo);
	uncompSize = o.uncompSize;
	o.zip = {};
	o.uncompSize = 0;
	return *this;
}

ZipIO::operator GenericIO()
{
	return GenericIO{*this};
}

CallResult ZipIO::open(const char *path, const char *pathInZip)
{
	close();
	if(!openZipFile(path))
	{
		logMsg("error opening %s as zip", path);
		close();
		return INVALID_PARAMETER;
	}

	if(unzLocateFile(zip, pathInZip, 1) != UNZ_OK)
	{
		logErr("%s not found in zip", pathInZip);
		close();
		return INVALID_PARAMETER;
	}

	if(!openFileInZip())
	{
		logMsg("error opening %s in zip", pathInZip);
		close();
		return IO_ERROR;
	}

	logMsg("opened %s in zip %s", pathInZip, path);
	return OK;
}

ssize_t ZipIO::read(void *buff, size_t bytes, CallResult *resultOut)
{
	int bytesRead = unzReadCurrentFile(zip, buff, bytes);
	if(bytesRead < 0)
	{
		bytesRead = -1;
		if(resultOut)
			*resultOut = READ_ERROR;
	}
	return bytesRead;
}

ssize_t ZipIO::write(const void* buff, size_t bytes, CallResult *resultOut)
{
	if(resultOut)
		*resultOut = UNSUPPORTED_OPERATION;
	return -1;
}

off_t ZipIO::tell(CallResult *resultOut)
{
	long pos = unztell(zip);
	if(pos < 0)
	{
		if(resultOut)
			*resultOut = IO_ERROR;
		return -1;
	}
	return pos;
}

CallResult ZipIO::seek(off_t offset, SeekMode mode)
{
	auto pos = tell();
	if(pos == -1)
		return IO_ERROR;

	if(!isSeekModeValid(mode))
	{
		bug_exit("invalid seek mode: %d", (int)mode);
		return INVALID_PARAMETER;
	}
	off_t absOffset = transformOffsetToAbsolute(mode, offset, 0, size(), pos);
	size_t bytesToSkip = 0;
	if(pos > absOffset) // seeking backwards, need to return to start of zip
	{
		//logWarn("warning: backwards seek");
		resetFileInZip();
		bytesToSkip = absOffset;
	}
	else
	{
		bytesToSkip = absOffset - pos;
	}

	while(bytesToSkip)
	{
		char dummy[4096];
		ssize_t bytesRead = read(dummy, std::min(sizeof(dummy), (size_t)bytesToSkip), nullptr);
		//logDMsg("skipped %d bytes", (int)bytesRead);
		if(bytesRead <= 0)
			break;
		assert((size_t)bytesRead <= bytesToSkip);
		bytesToSkip -= bytesRead;
	}

	return OK;
}

void ZipIO::close()
{
	if(zip)
	{
		unzClose(zip);
		zip = {};
		logMsg("closed zip");
	}
	zipIo.close();
}

size_t ZipIO::size()
{
	return uncompSize;
}

bool ZipIO::eof()
{
	return unzeof(zip);
}

ZipIO::operator bool()
{
	return zip;
}

bool ZipIO::openZipFile(const char *path)
{
	{
		FileIO file;
		if(file.open(path) != OK)
			return false;
		zipIo = file;
	}
	zlib_filefunc_def api
	{
		[](voidpf opaque, const char *, int) // open
		{
			// nothing to do, IO stored in opaque so just use it to avoid returning nullptr
			return (voidpf)opaque;
		},
		[](voidpf opaque, voidpf, void *buff, uLong bytes) // read
		{
			auto &io = *(GenericIO*)opaque;
			auto bytesRead = io.read(buff, bytes);
			return bytesRead > 0 ? (uLong)bytesRead : 0;
		},
		[](voidpf, voidpf, const void *, uLong) // write
		{
			return (uLong)0;
		},
		[](voidpf opaque, voidpf) // tell
		{
			auto &io = *(GenericIO*)opaque;
			return (long)io.tell();
		},
		[](voidpf opaque, voidpf, uLong offset, int origin) // seek
		{
			auto &io = *(GenericIO*)opaque;
			SeekMode mode;
			switch (origin)
			{
				bcase ZLIB_FILEFUNC_SEEK_CUR : mode = SEEK_CUR;
				bcase ZLIB_FILEFUNC_SEEK_END : mode = SEEK_END;
				bcase ZLIB_FILEFUNC_SEEK_SET : mode = SEEK_SET;
				bdefault: return (long)-1;
		    }
			long ret = 0;
			if(io.seek(offset, mode) != OK)
				ret = -1;
			return ret;
		},
		[](voidpf, voidpf) // close
		{
			return 0;
		},
		[](voidpf, voidpf) // test error
		{
			return 0; // TODO
		},
		&zipIo
	};
	zip = unzOpen2(nullptr, &api);
	if(!zip)
	{
		return false;
	}
	return true;
}

bool ZipIO::openFileInZip()
{
	unz_file_info info;
	unzGetCurrentFileInfo(zip, &info, nullptr, 0, nullptr, 0, nullptr, 0);
	logMsg("current file size %d, comp method %d", (int)info.uncompressed_size, (int)info.compression_method);
	// TODO: allow direct IO on files with compression_method == 0

	if(unzOpenCurrentFile(zip) != UNZ_OK)
	{
		return 0;
	}

	uncompSize = info.uncompressed_size;
	return 1;
}

void ZipIO::resetFileInZip()
{
	unzCloseCurrentFile(zip);
	unzOpenCurrentFile(zip);
}
