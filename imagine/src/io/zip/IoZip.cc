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

#define thisModuleName "io:zip"
#include "IoZip.hh"
#include <io/sys.hh>
#include <logger/interface.h>


static voidpf zOpenFunc(voidpf opaque, const char* filename, int mode)
{
	// nothing to do, IO stored in opaque so just use it to avoid returning NULL
	return opaque;
}

static uLong zReadFunc(voidpf opaque, voidpf stream, void* buf, uLong size)
{
	Io *f = (Io*)opaque;
	return f->fread(buf, 1, size);
}

static uLong zWriteFunc(voidpf opaque, voidpf stream, const void* buf, uLong size)
{
	return 0;
}

static long zTellFunc(voidpf opaque, voidpf stream)
{
	Io *f = (Io*)opaque;
	return f->ftell();
}

static long zSeekFunc(voidpf opaque, voidpf stream, uLong offset, int origin)
{
	Io *f = (Io*)opaque;
	int fseekOrigin = 0;
	switch (origin)
	{
		case ZLIB_FILEFUNC_SEEK_CUR : fseekOrigin = SEEK_CUR; break;
		case ZLIB_FILEFUNC_SEEK_END : fseekOrigin = SEEK_END; break;
		case ZLIB_FILEFUNC_SEEK_SET : fseekOrigin = SEEK_SET; break;
		default: return -1;
    }
	long ret = 0;
    if(f->fseek(offset, fseekOrigin) != 0)
        ret = -1;
    return ret;
}

static int zCloseFunc(voidpf opaque, voidpf stream)
{
	Io *f = (Io*)opaque;
	delete f;
	return 0;
}

static int zTesterrorFunc(voidpf opaque, voidpf stream)
{
	return 0; // TODO
}

bool IoZip::openZipFile(const char *path)
{
	zipIo = IoSys::open(path);
	if(!zipIo)
		return 0;

	zlib_filefunc_def api =
	{
		zOpenFunc,
		zReadFunc,
		zWriteFunc,
		zTellFunc,
		zSeekFunc,
		zCloseFunc,
		zTesterrorFunc,
		zipIo
	};

	zip = unzOpen2(0, &api);
	if(!zip)
	{
		delete zipIo;
		return 0;
	}

	return 1;
}

bool IoZip::openFileInZip()
{
	unz_file_info info;
	unzGetCurrentFileInfo(zip, &info, 0, 0, 0, 0, 0, 0);
	logMsg("current file size %d, comp method %d", (int)info.uncompressed_size, (int)info.compression_method);
	// TODO: allow direct IO on files with compression_method == 0

	if(unzOpenCurrentFile(zip) != UNZ_OK)
	{
		return 0;
	}

	uncompSize = info.uncompressed_size;
	return 1;
}

void IoZip::resetFileInZip()
{
	unzCloseCurrentFile(zip);
	unzOpenCurrentFile(zip);
}

Io* IoZip::open(const char *path, const char *pathInZip)
{
	IoZip *inst = new IoZip;
	if(!inst)
	{
		logErr("out of memory");
		return 0;
	}

	if(!inst->openZipFile(path))
	{
		logMsg("error opening %s as zip", path);
		delete inst;
		return 0;
	}

	if(unzLocateFile(inst->zip, pathInZip, 1) != UNZ_OK)
	{
		logErr("%s not found in zip", pathInZip);
		delete inst;
		return 0;
	}

	if(!inst->openFileInZip())
	{
		logMsg("error opening %s in zip", pathInZip);
		delete inst;
		return 0;
	}

	logMsg("opened %s in zip %s", pathInZip, path);
	return inst;
}

void IoZip::close()
{
	if(zip)
	{
		unzCloseCurrentFile(zip);
		unzClose(zip); // closes zipIo
		zip = nullptr;
		zipIo = nullptr;
	}
	else
	{
		delete zipIo;
		zipIo = nullptr;
	}
	logMsg("closed file @ %p", this);
}

void IoZip::truncate(ulong offset) { }
void IoZip::sync() { }

size_t IoZip::readUpTo(void* buffer, size_t numBytes)
{
	int bytesRead = unzReadCurrentFile(zip, buffer, numBytes);
	if(bytesRead < 0)
		bytesRead = 0;
	return(bytesRead);
}

size_t IoZip::fwrite(const void* ptr, size_t size, size_t nmemb)
{
	return 0;
}

CallResult IoZip::tell(ulong *offset)
{
	long pos = unztell(zip);
	if(pos >= 0)
	{
		*offset = pos;
		return OK;
	}
	else
		return IO_ERROR;
}

CallResult IoZip::seekU(ulong offset, uint mode)
{
	return seekS(offset, mode);
}

// TODO: merge this will similar code in IoMmap
static ulong seekOffsetToAbs(uint mode, long offset, ulong size, ulong current)
{
	switch(mode)
	{
		case IO_SEEK_ABS: return offset;
		case IO_SEEK_ABS_END: return size-offset;
		case IO_SEEK_ADD: return current + offset;
		case IO_SEEK_SUB: return current - offset;
		default: bug_branch("%d", mode); return 0;
	}
}

CallResult IoZip::seekS(long offset, uint mode)
{
	ulong pos;
	if(tell(&pos) != OK)
		return IO_ERROR;

	ulong absOffset = seekOffsetToAbs(mode, offset, size(), pos);
	ulong bytesToSkip = 0;
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

	uchar dummy[4096];
	while(bytesToSkip)
	{
		size_t bytesRead = readUpTo(dummy, std::min(sizeof(dummy), (size_t)bytesToSkip));
		//logDMsg("skipped %d bytes", (int)bytesRead);
		if(!bytesRead)
			break;
		assert(bytesRead <= bytesToSkip);
		bytesToSkip -= bytesRead;
	}

	return OK;
}

ulong IoZip::size()
{
	return uncompSize;
}

int IoZip::eof()
{
	return unzeof(zip);
}
