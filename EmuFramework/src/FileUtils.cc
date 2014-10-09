#include <emuframework/FileUtils.hh>
#include <imagine/util/strings.h>
#include <imagine/fs/sys.hh>
#include <imagine/base/Base.hh>

void chdirFromFilePath(const char *path)
{
	FsSys::PathString dirnameTemp;
	FsSys::chdir(string_dirname(path, dirnameTemp));
}

void fixFilePermissions(const char *path)
{
	#if defined CONFIG_BASE_IOS
	if(!Base::isSystemApp())
		return;
	// try to fix permissions if using jailbreak environment
	if(FsSys::hasWriteAccess(path) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	auto execPath = makeFSPathStringPrintf("%s/fixMobilePermission '%s'", Base::assetPath(), path);
	//logMsg("executing %s", execPath);
	int err = system(execPath.data());
	if(err)
	{
		logWarn("error from fixMobilePermission helper: %d", err);
	}
	#endif
}
