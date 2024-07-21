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

#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>
#include <sys/stat.h>

namespace IG
{

constexpr SystemLogger log{"AppCtx"};
constexpr mode_t defaultDirMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

void ApplicationContext::exit(int returnVal)
{
	application().setExitingActivityState();
	dispatchOnExit(false);
	delete static_cast<BaseApplication*>(appPtr);
	::exit(returnVal);
}

void ApplicationContext::openURL(CStringView url) const
{
	log.info("opening url:{}", url);
	[[maybe_unused]] auto ret = system(std::format("xdg-open {}", url).data());
}

FS::PathString ApplicationContext::assetPath(const char *) const
{
	return application().appPath();
}

FS::PathString ApplicationContext::supportPath(const char *appName) const
{
	if(auto home = getenv("XDG_DATA_HOME");
		home)
	{
		auto path = FS::pathString(home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	else if(auto home = getenv("HOME");
		home)
	{
		auto path = FS::pathString(home, ".local/share", appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	log.error("XDG_DATA_HOME and HOME env variables not defined");
	return {};
}

FS::PathString ApplicationContext::cachePath(const char *appName) const
{
	if(auto home = getenv("XDG_CACHE_HOME");
		home)
	{
		auto path = FS::pathString(home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	else if(auto home = getenv("HOME");
		home)
	{
		auto path = FS::pathString(home, ".cache", appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	log.error("XDG_DATA_HOME and HOME env variables not defined");
	return {};
}

FS::PathString ApplicationContext::sharedStoragePath() const
{
	if(Config::MACHINE_IS_PANDORA)
	{
		// look for the first mounted SD card
		for(auto &entry : FS::directory_iterator{"/media"})
		{
			if(entry.type() == FS::file_type::directory && std::string_view{entry.name()} == "mmcblk")
			{
				//log.info("storage dir:{}", entry.path());
				return entry.path();
			}
		}
		// fall back to appPath
	}
	if(auto home = getenv("HOME");
		home)
	{
		return home;
	}
	log.error("HOME env variable not defined");
	return {};
}

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	auto name = Config::MACHINE_IS_PANDORA ? "Media" : "Home";
	return {path, name};
}

std::vector<FS::PathLocation> ApplicationContext::rootFileLocations() const
{
	std::vector<FS::PathLocation> path;
	path.reserve(1);
	if(auto loc = sharedStoragePathLocation();
		loc)
	{
		path.emplace_back(loc);
	}
	return path;
}

FS::PathString ApplicationContext::libPath(const char *) const
{
	return application().appPath();
}

void ApplicationContext::exitWithMessage(int exitVal, const char *msg)
{
	auto cmd = std::format("zenity --warning --title='Exited with error' --text='{}'", msg);
	[[maybe_unused]] auto cmdResult = system(cmd.data());
	::exit(exitVal);
}

void ApplicationContext::setIdleDisplayPowerSave(bool on) { application().setIdleDisplayPowerSave(on); }

void ApplicationContext::endIdleByUserActivity() { application().endIdleByUserActivity(); }

bool ApplicationContext::registerInstance(ApplicationInitParams initParams, const char *name) { return application().registerInstance(initParams, name); }

void ApplicationContext::setAcceptIPC(bool on, const char *name) { application().setAcceptIPC(on, name); }

}
