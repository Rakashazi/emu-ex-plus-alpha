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

#define LOGTAG "Base"
#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/fs/FS.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/string.h>
#include <sys/stat.h>
#include <cstring>

namespace Base
{

constexpr mode_t defaultDirMode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;

LinuxApplication::LinuxApplication(ApplicationInitParams initParams):
	BaseApplication{*initParams.ctxPtr}
{
	setAppPath(FS::makeAppPathFromLaunchCommand(initParams.argv[0]));
	#ifdef CONFIG_BASE_DBUS
	initDBus();
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	initEvdev(initParams.eventLoop);
	#endif
}

LinuxApplication::~LinuxApplication()
{
	#ifdef CONFIG_BASE_DBUS
	deinitDBus();
	#endif
}

void ApplicationContext::exit(int returnVal)
{
	application().setExitingActivityState();
	dispatchOnExit(false);
	delete static_cast<BaseApplication*>(appPtr);
	::exit(returnVal);
}

void ApplicationContext::openURL(const char *url) const
{
	logMsg("opening url:%s", url);
	auto ret = system(FS::makePathStringPrintf("xdg-open %s", url).data());
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
		auto path = FS::makePathString(home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	else if(auto home = getenv("HOME");
		home)
	{
		auto path = FS::makePathStringPrintf("%s/.local/share/%s", home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	logErr("XDG_DATA_HOME and HOME env variables not defined");
	return {};
}

FS::PathString ApplicationContext::cachePath(const char *appName) const
{
	if(auto home = getenv("XDG_CACHE_HOME");
		home)
	{
		auto path = FS::makePathString(home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	else if(auto home = getenv("HOME");
		home)
	{
		auto path = FS::makePathStringPrintf("%s/.cache/%s", home, appName);
		g_mkdir_with_parents(path.data(), defaultDirMode);
		return path;
	}
	logErr("XDG_DATA_HOME and HOME env variables not defined");
	return {};
}

FS::PathString ApplicationContext::sharedStoragePath() const
{
	if(Config::MACHINE_IS_PANDORA)
	{
		// look for the first mounted SD card
		for(auto &entry : FS::directory_iterator{"/media"})
		{
			if(entry.type() == FS::file_type::directory && std::strstr(entry.name(), "mmcblk"))
			{
				//logMsg("storage dir: %s", entry.path().data());
				return entry.path();
			}
		}
		// fall back to appPath
	}
	if(auto home = getenv("HOME");
		home)
	{
		return FS::makePathString(home);
	}
	logErr("HOME env variable not defined");
	return {};
}

FS::PathLocation ApplicationContext::sharedStoragePathLocation() const
{
	auto path = sharedStoragePath();
	auto name = Config::MACHINE_IS_PANDORA ? FS::makeFileString("Media") : FS::makeFileString("Home");
	return {path, name, {name, strlen(path.data())}};
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

FS::PathString LinuxApplication::appPath() const
{
	return appPath_;
}

void LinuxApplication::setAppPath(FS::PathString path)
{
	appPath_ = path;
}

void ApplicationContext::exitWithErrorMessageVPrintf(int exitVal, const char *format, va_list args)
{
	std::array<char, 512> msg{};
	auto result = vsnprintf(msg.data(), msg.size(), format, args);
	auto cmd = string_makePrintf<1024>("zenity --warning --title='Exited with error' --text='%s'", msg.data());
	auto cmdResult = system(cmd.data());
	::exit(exitVal);
}

}

int main(int argc, char** argv)
{
	logger_setLogDirectoryPrefix(".");
	auto eventLoop = Base::EventLoop::makeForThread();
	Base::ApplicationContext ctx{};
	Base::ApplicationInitParams initParams{eventLoop, &ctx, argc, argv};
	ctx.onInit(initParams);
	ctx.application().setRunningActivityState();
	ctx.dispatchOnResume(true);
	bool eventLoopRunning = true;
	eventLoop.run(eventLoopRunning);
	return 0;
}
