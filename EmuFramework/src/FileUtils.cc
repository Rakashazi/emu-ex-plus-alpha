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
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include <ftw.h>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
    {
            int rv = remove(fpath);
    
            if (rv)
                    perror(fpath);
        
                return rv;
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

	auto execPath = FS::makePathStringPrintf("%s/fixMobilePermission '%s'", Base::assetPath().data(), path);
	//logMsg("executing %s", execPath);
	//int err = system(execPath.data());
    int err = nftw(execPath.data(), unlink_cb, 64, FTW_DEPTH | FTW_PHYS) == -1;
	if(err)
	{
		logWarn("error from fixMobilePermission helper: %d", err);
	}
	#endif
}
