/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/FileUtils.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#if defined CONFIG_BASE_IOS
#include <spawn.h>
#endif

extern "C"
{
	extern char **environ;
}

void fixFilePermissions(Base::ApplicationContext ctx, const char *path)
{
	#if defined CONFIG_BASE_IOS
	if(!ctx.isSystemApp())
		return;
	// try to fix permissions if using jailbreak environment
	if(FS::access(path, FS::acc::w) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	auto execPath = FS::makePathStringPrintf("%s/fixMobilePermission", ctx.assetPath().data());
	//logMsg("executing %s", execPath);
	auto fixMobilePermissionStr = FS::makeFileString("fixMobilePermission");
	auto pathStr = FS::makePathString(path);
	char *argv[]{fixMobilePermissionStr.data(), pathStr.data(), nullptr};
	int err = posix_spawn(nullptr, execPath.data(), nullptr, nullptr, argv, environ);
	if(err)
	{
		logWarn("error from fixMobilePermission helper: %d", err);
	}
	#endif
}
