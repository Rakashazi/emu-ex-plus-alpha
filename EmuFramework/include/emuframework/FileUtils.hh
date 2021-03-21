#pragma once

#include <imagine/fs/FS.hh>
#include <imagine/base/ApplicationContext.hh>

// used on iOS to allow saves on incorrectly root-owned files/dirs
void fixFilePermissions(Base::ApplicationContext, const char *path);

static void fixFilePermissions(Base::ApplicationContext app, const FS::PathString &path)
{
	return fixFilePermissions(app, path.data());
}
