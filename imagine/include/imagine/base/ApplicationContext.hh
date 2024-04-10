#pragma once

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

#include <imagine/config/defs.hh>

#if defined CONFIG_PACKAGE_X11
#include <imagine/base/x11/XApplicationContext.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidApplicationContext.hh>
#elif defined CONFIG_OS_IOS
#include <imagine/base/iphone/IOSApplicationContext.hh>
#endif

#include <imagine/base/baseDefs.hh>
#include <imagine/io/ioDefs.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/time/Time.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/util/utility.h>
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <vector>
#include <optional>
#include <string>
#include <string_view>

namespace IG::Input
{
class Event;
class Device;
}

namespace IG::FS
{
class AssetDirectoryIterator;
class directory_entry;
}

namespace IG
{

class PixelFormat;
class PerformanceHintManager;

using DirectoryEntryDelegate = DelegateFuncS<sizeof(void*)*3, bool(const FS::directory_entry &)>;

enum class Permission
{
	WRITE_EXT_STORAGE,
	COARSE_LOCATION
};

struct SystemUIStyleFlags
{
	uint8_t
	dimNavigation:1{},
	hideNavigation:1{},
	hideStatus:1{};

	constexpr bool operator ==(SystemUIStyleFlags const &) const = default;
};


class ApplicationContext : public ApplicationContextImpl
{
public:
	using ApplicationContextImpl::ApplicationContextImpl;

	// defined in user code
	static const char *const applicationName;
	static const char *const applicationId;

	// Called on app startup via dispatchOnInit() to create the Application object, defined in user code
	[[gnu::cold]] void onInit(ApplicationInitParams);

	// Calls onInit() and handles displaying error messages from any exceptions
	[[gnu::cold]] void dispatchOnInit(ApplicationInitParams);

	// Initialize the main Application object with a user-defined class,
	// must be called first in onInit() before using any other methods
	template<class T>
	T &initApplication(auto &&... args)
	{
		return *new T{IG_forward(args)...}; // Application constructor assigns this pointer to ApplicationContext and frees it when the app exits
	}

	Application &application() const;
	template<class T> T &applicationAs() const { return static_cast<T&>(application()); }
	void runOnMainThread(MainThreadMessageDelegate);
	void flushMainThreadMessages();

	Window *makeWindow(WindowConfig, WindowInitDelegate);
	const WindowContainer &windows() const;
	Window &mainWindow();
	bool systemAnimatesWindowRotation() const;
	PixelFormat defaultWindowPixelFormat() const;

	const ScreenContainer &screens() const;
	Screen &mainScreen();

	NativeDisplayConnection nativeDisplayConnection() const;

	// CPU configuration
	int cpuCount() const;
	int maxCPUFrequencyKHz(int cpuIdx) const;
	CPUMask performanceCPUMask() const;
	PerformanceHintManager performanceHintManager();

	// App Callbacks

	// Called when app returns from backgrounded state
	bool addOnResume(ResumeDelegate, int priority = APP_ON_RESUME_PRIORITY);
	bool removeOnResume(ResumeDelegate);
	bool containsOnResume(ResumeDelegate) const;
	void dispatchOnResume(bool focused);

	// Called when app will finish execution
	// If backgrounded == true, app may eventually resume execution
	bool addOnExit(ExitDelegate, int priority = APP_ON_EXIT_PRIORITY);
	bool removeOnExit(ExitDelegate);
	bool containsOnExit(ExitDelegate) const;
	void dispatchOnExit(bool backgrounded);

	// Called when device changes orientation and the sensor is enabled
	void setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate);

	// Called when system UI changes orientation
	void setOnSystemOrientationChanged(SystemOrientationChangedDelegate);

	// App run state
	bool isRunning() const;
	bool isPaused() const;
	bool isExiting() const;

	// Inter-process messages
	bool registerInstance(ApplicationInitParams, const char *appId = applicationId);
	void setAcceptIPC(bool on, const char *appId = applicationId);

	// external services
	void openURL(CStringView url) const;
	bool packageIsInstalled(CStringView name) const;

	// file system paths & asset loading, thread-safe
	FS::PathString assetPath(const char *appName = applicationName) const;
	FS::PathString libPath(const char *appName = applicationName) const;
	FS::PathString supportPath(const char *appName = applicationName) const;
	FS::PathString storagePath(const char *appName = applicationName) const;
	FS::PathString cachePath(const char *appName = applicationName) const;
	FS::PathString sharedStoragePath() const;
	FS::PathLocation sharedStoragePathLocation() const;
	std::vector<FS::PathLocation> rootFileLocations() const;
	FS::RootPathInfo rootPathInfo(std::string_view path) const;
	AssetIO openAsset(CStringView name, OpenFlags oFlags = {}, const char *appName = applicationName) const;
	FS::AssetDirectoryIterator openAssetDirectory(CStringView path, const char *appName = applicationName);

	// path/file access using OS-specific URIs such as those in the Android Storage Access Framework,
	// backwards compatible with regular file system paths, all thread-safe except for picker functions
	bool hasSystemPathPicker() const;
	bool showSystemPathPicker();
	bool hasSystemDocumentPicker() const;
	bool showSystemDocumentPicker();
	bool showSystemCreateDocumentPicker();
	FileIO openFileUri(CStringView uri, OpenFlags oFlags = {}) const;
	UniqueFileDescriptor openFileUriFd(CStringView uri, OpenFlags oFlags = {}) const;
	bool fileUriExists(CStringView uri) const;
	WallClockTimePoint fileUriLastWriteTime(CStringView uri) const;
	std::string fileUriFormatLastWriteTimeLocal(CStringView uri) const;
	FS::FileString fileUriDisplayName(CStringView uri) const;
	FS::file_type fileUriType(CStringView uri) const;
	bool removeFileUri(CStringView uri) const;
	bool renameFileUri(CStringView oldUri, CStringView newUri) const;
	bool createDirectoryUri(CStringView uri) const;
	bool removeDirectoryUri(CStringView uri) const;
	bool forEachInDirectoryUri(CStringView uri, DirectoryEntryDelegate, FS::DirOpenFlags flags = {}) const;

	// OS UI management (status & navigation bar)
	void setSysUIStyle(SystemUIStyleFlags);
	bool hasTranslucentSysUI() const;
	bool hasHardwareNavButtons() const;
	void setSystemOrientation(Rotation);
	Orientations defaultSystemOrientations() const;
	bool hasDisplayCutout() const;

	// Sensors
	void setDeviceOrientationChangeSensor(bool on);
	SensorValues remapSensorValuesForDeviceRotation(SensorValues) const;

	// Notification/Launcher icons
	void addNotification(CStringView onShow, CStringView title, CStringView message);
	void addLauncherIcon(CStringView name, CStringView path);

	// Power Management
	void setIdleDisplayPowerSave(bool on);
	void endIdleByUserActivity();
	bool hasSustainedPerformanceMode() const;
	void setSustainedPerformanceMode(bool on);

	// Permissions
	bool usesPermission(Permission p) const;
	bool permissionIsRestricted(Permission p) const;
	bool requestPermission(Permission p);

	// Date & Time
	std::string formatDateAndTime(WallClockTimePoint timeSinceEpoch);
	std::string formatDateAndTimeAsFilename(WallClockTimePoint timeSinceEpoch);

	// Input
	const InputDeviceContainer &inputDevices() const;
	Input::Device *inputDevice(std::string_view name, int enumId) const;
	void enumInputDevices() const;
	void setHintKeyRepeat(bool on);
	Input::Event defaultInputEvent() const;
	std::optional<bool> swappedConfirmKeysOption() const;
	bool swappedConfirmKeys() const;
	void setSwappedConfirmKeys(std::optional<bool>);
	bool keyInputIsPresent() const;
	void flushInputEvents();
	void flushSystemInputEvents();
	void flushInternalInputEvents();
	bool hasInputDeviceHotSwap() const;

	// App exit
	void exit(int returnVal);
	void exit() { exit(0); }
	void exitWithMessage(int exitVal, const char *msg);

	// Platform-specific
	int32_t androidSDK() const;
};

class OnExit
{
public:
	constexpr OnExit() = default;
	constexpr OnExit(ApplicationContext ctx):ctx{ctx} {}
	OnExit(ExitDelegate, ApplicationContext, int priority = APP_ON_EXIT_PRIORITY);
	OnExit(OnExit &&) noexcept;
	OnExit &operator=(OnExit &&) noexcept;
	~OnExit();
	void reset();
	ApplicationContext appContext() const;

protected:
	ExitDelegate del{};
	ApplicationContext ctx{};
};

}
