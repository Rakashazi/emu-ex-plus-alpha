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

#include <imagine/io/Io.hh>
#include <imagine/fs/sys.hh> // for FsSys::PathString
#include <imagine/base/Base.hh>

#if defined CONFIG_IO_FD
#include <imagine/io/IoFd.hh>
using IoSys = IoFd;
#elif defined CONFIG_IO_WIN32
#include <imagine/io/IoWin32.hh>
using IoSys = IoWin32;
#endif

#ifdef CONFIG_IO_AASSET
#include <imagine/io/AAssetIO.hh>
#endif

static CallResult copyIoToPath(Io &io, const char *outPath)
{
	auto outFile = IOFile(IoSys::create(outPath));
	if(!outFile)
		return IO_ERROR;
	CallResult ret = io.writeToIO(*outFile.io());
	return ret;
}

static Io *openAppAssetIo(const char *name)
{
	#ifdef CONFIG_IO_AASSET
	return AAssetIO::open(name);
	#else
	return IoSys::open(makeFSPathStringPrintf("%s/%s", Base::assetPath(), name).data());
	#endif
}
