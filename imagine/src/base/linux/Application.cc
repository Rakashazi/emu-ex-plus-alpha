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
#include <imagine/util/format.hh>
#include <cstring>

namespace IG
{

LinuxApplication::LinuxApplication(ApplicationInitParams initParams):
	BaseApplication{({initParams.ctxPtr->setApplicationPtr(static_cast<Application*>(this)); *initParams.ctxPtr;})}
{
	setAppPath(FS::makeAppPathFromLaunchCommand(initParams.argv[0]));
	initDBus();
	initEvdev(initParams.eventLoop);
}

LinuxApplication::~LinuxApplication()
{
	deinitDBus();
}

const FS::PathString &LinuxApplication::appPath() const
{
	return appPath_;
}

void LinuxApplication::setAppPath(FS::PathString path)
{
	appPath_ = std::move(path);
}

#ifndef CONFIG_PACKAGE_DBUS
bool LinuxApplication::initDBus() { return false; }
void LinuxApplication::deinitDBus() {}
void LinuxApplication::setIdleDisplayPowerSave(bool) {}
void LinuxApplication::endIdleByUserActivity() {}
bool LinuxApplication::registerInstance(ApplicationInitParams, char const*) { return false; }
void LinuxApplication::setAcceptIPC(bool, char const*) {}
#endif

}

int main(int argc, char** argv)
{
	using namespace IG;
	logger_setLogDirectoryPrefix(".");
	auto eventLoop = EventLoop::makeForThread();
	ApplicationContext ctx{};
	ApplicationInitParams initParams{eventLoop, &ctx, argc, argv};
	ctx.dispatchOnInit(initParams);
	ctx.application().setRunningActivityState();
	ctx.dispatchOnResume(true);
	bool eventLoopRunning = true;
	eventLoop.run(eventLoopRunning);
	return 0;
}
