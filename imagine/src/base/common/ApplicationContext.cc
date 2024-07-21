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
#include <imagine/base/VibrationManager.hh>
#include <imagine/base/Sensor.hh>
#include <imagine/base/PerformanceHintManager.hh>
#include <imagine/input/Event.hh>
#include <imagine/fs/FS.hh>
#include <imagine/fs/FSUtils.hh>
#include <imagine/fs/AssetFS.hh>
#include <imagine/fs/ArchiveFS.hh>
#ifdef __ANDROID__
#include <imagine/fs/AAssetFS.hh>
#endif
#include <imagine/io/FileIO.hh>
#include <imagine/io/IO.hh>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/format.hh>
#include <imagine/util/ranges.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/memory/UniqueFileStream.hh>
#include <imagine/util/bit.hh>
#include <imagine/logger/logger.h>
#include <cstring>

namespace IG
{

constexpr SystemLogger log{"AppContext"};

void ApplicationContext::dispatchOnInit(ApplicationInitParams initParams)
{
	try
	{
		onInit(initParams);
	}
	catch(std::exception &err)
	{
		exitWithMessage(-1, err.what());
	}
}

Application &ApplicationContext::application() const
{
	return ApplicationContextImpl::application();
}

void ApplicationContext::runOnMainThread(MainThreadMessageDelegate del)
{
	application().runOnMainThread(del);
}

void ApplicationContext::flushMainThreadMessages()
{
	application().flushMainThreadMessages();
}

Window *ApplicationContext::makeWindow(WindowConfig config, WindowInitDelegate onInit)
{
	if(!Config::BASE_MULTI_WINDOW && windows().size())
	{
		bug_unreachable("no multi-window support");
	}
	auto winPtr = std::make_unique<Window>(*this, config, onInit);
	if(!*winPtr)
	{
		return nullptr;
	}
	auto ptr = winPtr.get();
	application().addWindow(std::move(winPtr));
	if(Window::shouldRunOnInitAfterAddingWindow && onInit)
	{
		try
		{
			onInit(*this, *ptr);
		}
		catch(std::exception &err)
		{
			exitWithMessage(-1, err.what());
		}
	}
	return ptr;
}

const WindowContainer &ApplicationContext::windows() const
{
	return application().windows();
}

Window &ApplicationContext::mainWindow()
{
	return application().mainWindow();
}

const ScreenContainer &ApplicationContext::screens() const
{
	return application().screens();
}

Screen &ApplicationContext::mainScreen()
{
	return application().mainScreen();
}

bool ApplicationContext::isRunning() const
{
	return application().isRunning();
}

bool ApplicationContext::isPaused() const
{
	return application().isPaused();
}

bool ApplicationContext::isExiting() const
{
	return application().isExiting();
}

bool ApplicationContext::addOnResume(ResumeDelegate del, int priority)
{
	return application().addOnResume(del, priority);
}

bool ApplicationContext::removeOnResume(ResumeDelegate del)
{
	return application().removeOnResume(del);
}

bool ApplicationContext::containsOnResume(ResumeDelegate del) const
{
	return application().containsOnResume(del);
}

bool ApplicationContext::addOnExit(ExitDelegate del, int priority)
{
	return application().addOnExit(del, priority);
}

bool ApplicationContext::removeOnExit(ExitDelegate del)
{
	return application().removeOnExit(del);
}

bool ApplicationContext::containsOnExit(ExitDelegate del) const
{
	return application().containsOnExit(del);
}

void ApplicationContext::dispatchOnResume(bool focused)
{
	application().dispatchOnResume(*this, focused);
}

void ApplicationContext::dispatchOnExit(bool backgrounded)
{
	application().dispatchOnExit(*this, backgrounded);
}

[[gnu::weak]] FS::PathString ApplicationContext::storagePath(const char *) const
{
	return sharedStoragePath();
}

FS::RootPathInfo ApplicationContext::rootPathInfo(std::string_view path) const
{
	if(path.empty())
		return {};
	if(isUri(path))
	{
		if(auto [treePath, treePos] = FS::uriPathSegment(path, FS::uriPathSegmentTreeName);
			Config::envIsAndroid && treePos != std::string_view::npos)
		{
			auto [docPath, docPos] = FS::uriPathSegment(path, FS::uriPathSegmentDocumentName);
			//log.info("tree path segment:{}", FS::PathString{treePath});
			//log.info("document path segment:{}", FS::PathString{docPath});
			if(docPos == std::string_view::npos || docPath.size() < treePath.size())
			{
				log.error("invalid document path in tree URI:{}", path);
				return {};
			}
			auto rootLen = docPos + treePath.size();
			FS::PathString rootDocUri{path.data(), rootLen};
			log.info("found root document URI:{}", rootDocUri);
			auto name = fileUriDisplayName(rootDocUri);
			if(rootDocUri.ends_with("%3A"))
				name += ':';
			return {name, rootLen};
		}
		else
		{
			log.error("rootPathInfo() unsupported URI:{}", path);
			return {};
		}
	}
	auto location = rootFileLocations();
	const FS::PathLocation *nearestPtr{};
	size_t lastMatchOffset = 0;
	for(const auto &l : location)
	{
		if(!path.starts_with(l.root.path))
			continue;
		auto matchOffset = (size_t)(&path[l.root.info.length] - path.data());
		if(matchOffset > lastMatchOffset)
		{
			nearestPtr = &l;
			lastMatchOffset = matchOffset;
		}
	}
	if(!lastMatchOffset)
		return {};
	log.info("found root location:{} with length:{}", nearestPtr->root.info.name, nearestPtr->root.info.length);
	return nearestPtr->root.info;
}

AssetIO ApplicationContext::openAsset(CStringView name, OpenFlags openFlags, [[maybe_unused]] const char* appName) const
{
	#ifdef __ANDROID__
	return {*this, name, openFlags};
	#else
	return {FS::pathString(assetPath(appName), name), openFlags};
	#endif
}

FS::AssetDirectoryIterator ApplicationContext::openAssetDirectory(CStringView path, [[maybe_unused]] const char* appName)
{
	#ifdef __ANDROID__
	return {aAssetManager(), path};
	#else
	return {FS::pathString(assetPath(appName), path)};
	#endif
}

[[gnu::weak]] bool ApplicationContext::hasSystemPathPicker() const { return false; }

[[gnu::weak]] bool ApplicationContext::showSystemPathPicker() { return false; }

[[gnu::weak]] bool ApplicationContext::hasSystemDocumentPicker() const { return false; }

[[gnu::weak]] bool ApplicationContext::showSystemDocumentPicker() { return false; }

[[gnu::weak]] bool ApplicationContext::showSystemCreateDocumentPicker() { return false; }

[[gnu::weak]] FileIO ApplicationContext::openFileUri(CStringView uri, OpenFlags openFlags) const
{
	return {uri, openFlags};
}

[[gnu::weak]] UniqueFileDescriptor ApplicationContext::openFileUriFd(CStringView uri, OpenFlags openFlags) const
{
	return PosixIO{uri, openFlags}.releaseFd();
}

[[gnu::weak]] bool ApplicationContext::fileUriExists(CStringView uri) const
{
	return FS::exists(uri);
}

[[gnu::weak]] FS::file_time_type ApplicationContext::fileUriLastWriteTime(CStringView uri) const
{
	return FS::status(uri).lastWriteTime();
}

[[gnu::weak]] std::string ApplicationContext::fileUriFormatLastWriteTimeLocal(CStringView uri) const
{
	return FS::formatLastWriteTimeLocal(*this, uri);
}

[[gnu::weak]] FS::FileString ApplicationContext::fileUriDisplayName(CStringView uri) const
{
	return FS::displayName(uri);
}

[[gnu::weak]] FS::file_type ApplicationContext::fileUriType(CStringView uri) const
{
	return FS::status(uri).type();
}

[[gnu::weak]] bool ApplicationContext::removeFileUri(CStringView uri) const
{
	return FS::remove(uri);
}

[[gnu::weak]] bool ApplicationContext::renameFileUri(CStringView oldUri, CStringView newUri) const
{
	return FS::rename(oldUri, newUri);
}

[[gnu::weak]] bool ApplicationContext::createDirectoryUri(CStringView uri) const
{
	return FS::create_directory(uri);
}

[[gnu::weak]] bool ApplicationContext::removeDirectoryUri(CStringView uri) const
{
	return FS::remove(uri);
}

[[gnu::weak]] bool ApplicationContext::forEachInDirectoryUri(CStringView uri,
	DirectoryEntryDelegate del, FS::DirOpenFlags flags) const
{
	return forEachInDirectory(uri, del, flags);
}

const InputDeviceContainer &ApplicationContext::inputDevices() const
{
	return application().inputDevices();
}

Input::Device* ApplicationContext::inputDevice(std::string_view name, int enumId) const
{
	return findPtr(inputDevices(), [&](auto &devPtr){ return devPtr->enumId() == enumId && devPtr->name() == name; });
}

void ApplicationContext::setHintKeyRepeat(bool on)
{
	application().setAllowKeyRepeatTimer(on);
}

Input::Event ApplicationContext::defaultInputEvent() const
{
	if(keyInputIsPresent())
		return Input::KeyEvent{};
	else
		return Input::MotionEvent{};
}

std::optional<bool> ApplicationContext::swappedConfirmKeysOption() const
{
	return application().swappedConfirmKeysOption();
}

bool ApplicationContext::swappedConfirmKeys() const
{
	return application().swappedConfirmKeys();
}

void ApplicationContext::setSwappedConfirmKeys(std::optional<bool> opt)
{
	application().setSwappedConfirmKeys(opt);
}

[[gnu::weak]] void ApplicationContext::setSysUIStyle(SystemUIStyleFlags) {}

[[gnu::weak]] bool ApplicationContext::hasTranslucentSysUI() const { return false; }

[[gnu::weak]] bool ApplicationContext::hasHardwareNavButtons() const { return false; }

[[gnu::weak]] bool ApplicationContext::systemAnimatesWindowRotation() const { return Config::SYSTEM_ROTATES_WINDOWS; }

[[gnu::weak]] void ApplicationContext::setDeviceOrientationChangeSensor(bool) {}

[[gnu::weak]] SensorValues ApplicationContext::remapSensorValuesForDeviceRotation(SensorValues v) const { return v; }

[[gnu::weak]] void ApplicationContext::setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate) {}

[[gnu::weak]] void ApplicationContext::setSystemOrientation(Rotation) {}

[[gnu::weak]] Orientations ApplicationContext::defaultSystemOrientations() const { return Orientations::all(); }

[[gnu::weak]] void ApplicationContext::setOnSystemOrientationChanged(SystemOrientationChangedDelegate) {}

[[gnu::weak]] bool ApplicationContext::hasDisplayCutout() const { return false; }

[[gnu::weak]] bool ApplicationContext::usesPermission(Permission) const { return false; }

[[gnu::weak]] bool ApplicationContext::permissionIsRestricted(Permission) const { return false; }

[[gnu::weak]] bool ApplicationContext::requestPermission(Permission) { return false; }

[[gnu::weak]] void ApplicationContext::addNotification(CStringView, CStringView, CStringView) {}

[[gnu::weak]] void ApplicationContext::addLauncherIcon(CStringView, CStringView) {}

[[gnu::weak]] bool VibrationManager::hasVibrator() const { return false; }

[[gnu::weak]] void VibrationManager::vibrate(Milliseconds) {}

[[gnu::weak]] NativeDisplayConnection ApplicationContext::nativeDisplayConnection() const { return {}; }

[[gnu::weak]] int ApplicationContext::cpuCount() const
{
	#ifdef __linux__
	return std::min(sysconf(_SC_NPROCESSORS_CONF), long(maxCPUs));
	#else
	return 1;
	#endif
}

[[gnu::weak]] int ApplicationContext::maxCPUFrequencyKHz([[maybe_unused]] int cpuIdx) const
{
	#ifdef __linux__
	auto maxFreqFile = UniqueFileStream{fopen(std::format("/sys/devices/system/cpu/cpu{}/cpufreq/cpuinfo_max_freq", cpuIdx).c_str(), "r")};
	if(!maxFreqFile)
		return 0;
	int freq{};
	[[maybe_unused]] auto items = fscanf(maxFreqFile.get(), "%d", &freq);
	return freq;
	#else
	return 0;
	#endif
}

[[gnu::weak]] CPUMask ApplicationContext::performanceCPUMask() const
{
	auto cpus = cpuCount();
	if(cpus <= 2) // use all cores when count is small
		return 0;
	struct CPUFreqInfo{int freq, cpuIdx;};
	StaticArrayList<CPUFreqInfo, maxCPUs> cpuFreqInfos;
	for(int i : iotaCount(cpus))
	{
		auto freq = maxCPUFrequencyKHz(i);
		if(freq > 0)
			cpuFreqInfos.emplace_back(CPUFreqInfo{freq, i});
	}
	auto [min, max] = std::ranges::minmax_element(cpuFreqInfos, {}, &CPUFreqInfo::freq);
	if(min->freq == max->freq) // not heterogeneous
		return 0;
	log.debug("Detected heterogeneous CPUs with min:{} max:{} frequencies", min->freq, max->freq);
	CPUMask mask{};
	for(auto info : cpuFreqInfos)
	{
		if(info.freq != min->freq)
			mask |= bit(info.cpuIdx);
	}
	return mask;
}

[[gnu::weak]] PerformanceHintManager ApplicationContext::performanceHintManager() { return {}; }

[[gnu::weak]] bool ApplicationContext::packageIsInstalled(CStringView) const { return false; }

[[gnu::weak]] int32_t ApplicationContext::androidSDK() const
{
	bug_unreachable("Invalid platform-specific function");
}

[[gnu::weak]] bool ApplicationContext::hasSustainedPerformanceMode() const { return false; }
[[gnu::weak]] void ApplicationContext::setSustainedPerformanceMode(bool) {}

[[gnu::weak]] std::string ApplicationContext::formatDateAndTime(WallClockTimePoint time)
{
	if(!hasTime(time))
		return {};
	std::tm localTime;
	time_t secs = duration_cast<Seconds>(time.time_since_epoch()).count();
	if(!localtime_r(&secs, &localTime)) [[unlikely]]
	{
		log.error("localtime_r failed");
		return {};
	}
	std::string str;
	str.resize_and_overwrite(64, [&](char *buf, size_t size)
	{
		return std::strftime(buf, size, "%x %r", &localTime);
	});
	return str;
}

std::string ApplicationContext::formatDateAndTimeAsFilename(WallClockTimePoint time)
{
	auto filename = formatDateAndTime(time);
	std::ranges::replace(filename, '/', '-');
	std::ranges::replace(filename, ':', '.');
	return filename;
}

[[gnu::weak]] SensorListener::SensorListener(ApplicationContext, SensorType, SensorChangedDelegate) {}

[[gnu::weak]] void PerformanceHintSession::updateTargetWorkTime(Nanoseconds) {}
[[gnu::weak]] void PerformanceHintSession::reportActualWorkTime(Nanoseconds) {}
[[gnu::weak]] PerformanceHintSession::operator bool() const { return false; }
[[gnu::weak]] PerformanceHintSession PerformanceHintManager::session(std::span<const ThreadId>, Nanoseconds) { return {}; }
[[gnu::weak]] PerformanceHintManager::operator bool() const { return false; }

OnExit::OnExit(ResumeDelegate del, ApplicationContext ctx, int priority): del{del}, ctx{ctx}
{
	ctx.addOnExit(del, priority);
}

OnExit::OnExit(OnExit &&o) noexcept
{
	*this = std::move(o);
}

OnExit &OnExit::operator=(OnExit &&o) noexcept
{
	reset();
	del = std::exchange(o.del, {});
	ctx = o.ctx;
	return *this;
}

OnExit::~OnExit()
{
	reset();
}

void OnExit::reset()
{
	if(!del)
		return;
	ctx.removeOnExit(std::exchange(del, {}));
}

ApplicationContext OnExit::appContext() const
{
	return ctx;
}

}

namespace IG::FileUtils
{

ssize_t writeToUri(ApplicationContext ctx, CStringView uri, std::span<const unsigned char> src)
{
	auto f = ctx.openFileUri(uri, OpenFlags::testNewFile());
	return f.write(src).bytes;
}

ssize_t readFromUri(ApplicationContext ctx, CStringView uri, std::span<unsigned char> dest,
	IOAccessHint accessHint)
{
	auto f = ctx.openFileUri(uri, {.test = true, .accessHint = accessHint});
	return f.read(dest).bytes;
}

std::pair<ssize_t, FS::PathString> readFromUriWithArchiveScan(ApplicationContext ctx, CStringView uri,
	std::span<unsigned char> dest, bool(*nameMatchFunc)(std::string_view), IOAccessHint accessHint)
{
	auto io = ctx.openFileUri(uri, {.accessHint = accessHint});
	if(FS::hasArchiveExtension(uri))
	{
		for(auto &entry : FS::ArchiveIterator{std::move(io)})
		{
			if(entry.type() == FS::file_type::directory)
			{
				continue;
			}
			auto name = entry.name();
			if(nameMatchFunc(name))
			{
				return {entry.read(dest).bytes, FS::PathString{name}};
			}
		}
		log.error("no recognized files in archive:{}", uri);
		return {-1, {}};
	}
	else
	{
		return {io.read(dest).bytes, FS::PathString{uri}};
	}
}

IOBuffer bufferFromUri(ApplicationContext ctx, CStringView uri, OpenFlags openFlags, size_t sizeLimit)
{
	if(!sizeLimit) [[unlikely]]
		return {};
	openFlags.accessHint = IOAccessHint::All;
	auto file = ctx.openFileUri(uri, openFlags);
	if(!file)
		return {};
	else if(file.size() > sizeLimit)
	{
		if(openFlags.test)
			return {};
		else
			throw std::runtime_error(std::format("{} exceeds {} byte limit", uri, sizeLimit));
	}
	return file.buffer(IOBufferMode::Release);
}

IOBuffer rwBufferFromUri(ApplicationContext ctx, CStringView uri, OpenFlags extraOFlags, size_t size, uint8_t initValue)
{
	if(!size) [[unlikely]]
		return {};
	extraOFlags.accessHint = IOAccessHint::Random;
	auto file = ctx.openFileUri(uri, OpenFlags::createFile() | extraOFlags);
	if(!file) [[unlikely]]
		return {};
	auto fileSize = file.size();
	if(fileSize != size)
		file.truncate(size);
	auto buff = file.buffer(IOBufferMode::Release);
	if(initValue && fileSize < size)
	{
		std::fill(&buff[fileSize], &buff[size], initValue);
	}
	return buff;
}

FILE *fopenUri(ApplicationContext ctx, CStringView path, CStringView mode)
{
	if(isUri(path))
	{
		assert(!mode.contains('a')); //append mode not supported
		OpenFlags openFlags{.test = true};
		if(mode.contains('r'))
			openFlags.read = true;
		if(mode.contains('w'))
			openFlags |= OpenFlags::newFile();
		if(mode.contains('+'))
			openFlags.write = true;
		return ctx.openFileUri(path, openFlags).toFileStream(mode);
	}
	else
	{
		return ::fopen(path, mode);
	}
}

}
