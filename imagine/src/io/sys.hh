#pragma once

#include <io/Io.hh>
#include <fs/sys.hh> // for FsSys::cPath
#include <base/Base.hh>

#ifdef CONFIG_IO_FD
	#include <io/fd/IoFd.hh>
	#define IoSys IoFd
#endif

#ifdef CONFIG_BASE_ANDROID
	#include <io/zip/IoZip.hh>
#endif

static CallResult copyIoToPath(Io *io, const char *outPath)
{
	var_copy(outFile, IoSys::create(outPath));
	if(!outFile)
		return IO_ERROR;
	CallResult ret = io->writeToIO(outFile);
	outFile->close();
	return ret;
}

static Io *openAppAssetIo(const char *name)
{
	FsSys::cPath path;
	#ifdef CONFIG_BASE_ANDROID
	if(!string_printf(path, sizeof(path), "assets/%s", name))
		return 0;
	return IoZip::open(Base::appPath, path);
	#else
	if(!string_printf(path, sizeof(path), "%s/%s", Base::appPath, name))
		return 0;
	return IoSys::open(path);
	#endif
}
