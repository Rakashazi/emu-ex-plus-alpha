#include <FileUtils.hh>
#include <util/strings.h>
#include <fs/sys.hh>
#include <base/Base.hh>

void chdirFromFilePath(const char *path)
{
	FsSys::cPath dirnameTemp;
	FsSys::chdir(string_dirname(path, dirnameTemp));
}

void fixFilePermissions(const char *path)
{
	if(FsSys::hasWriteAccess(path) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	FsSys::cPath execPath;
	string_printf(execPath, "%s/fixMobilePermission '%s'", Base::appPath, path);
	//logMsg("executing %s", execPath);
	int err = system(execPath);
	if(err)
	{
		logWarn("error from fixMobilePermission helper: %d", err);
	}
}
