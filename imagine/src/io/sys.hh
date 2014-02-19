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

#include <io/Io.hh>
#include <fs/sys.hh> // for FsSys::cPath
#include <base/Base.hh>

#if defined CONFIG_IO_FD
#include <io/fd/IoFd.hh>
using IoSys = IoFd;
#elif defined CONFIG_IO_WIN32
#include <io/win32/IoWin32.hh>
using IoSys = IoWin32;
#endif

#ifdef CONFIG_BASE_ANDROID
#include <io/zip/IoZip.hh>
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
	FsSys::cPath path;
	#ifdef CONFIG_BASE_ANDROID
	if(!string_printf(path, sizeof(path), "assets/%s", name))
		return nullptr;
	return IoZip::open(Base::appPath, path);
	#else
	if(!string_printf(path, sizeof(path), "%s/%s", Base::appPath, name))
		return nullptr;
	return IoSys::open(path);
	#endif
}
