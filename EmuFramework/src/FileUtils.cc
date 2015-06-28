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
#include <imagine/util/strings.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>

void chdirFromFilePath(const char *path)
{
	FS::current_path(FS::dirname(path));
}

void fixFilePermissions(const char *path)
{
	#if defined CONFIG_BASE_IOS
	if(!Base::isSystemApp())
		return;
	// try to fix permissions if using jailbreak environment
	if(FS::access(path, FS::acc::w) == 0)
	{
		logMsg("%s lacks write permission, setting user as owner", path);
	}
	else
		return;

	auto execPath = FS::makePathStringPrintf("%s/fixMobilePermission '%s'", Base::assetPath(), path);
	//logMsg("executing %s", execPath);
	int err = system(execPath.data());
	if(err)
	{
		logWarn("error from fixMobilePermission helper: %d", err);
	}
	#endif
}
