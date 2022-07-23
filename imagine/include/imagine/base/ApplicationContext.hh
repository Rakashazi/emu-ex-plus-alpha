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

#if defined CONFIG_BASE_X11
#include <imagine/base/x11/XApplicationContext.hh>
#elif defined __ANDROID__
#include <imagine/base/android/AndroidApplicationContext.hh>
#elif defined CONFIG_BASE_IOS
#include <imagine/base/iphone/IOSApplicationContext.hh>
#endif

#include <imagine/base/baseDefs.hh>
#include <imagine/io/ioDefs.hh>
#include <imagine/fs/FSDefs.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/utility.h>
#include <imagine/util/string/CStringView.hh>
#include <imagine/util/memory/UniqueFileDescriptor.hh>
#include <vector>
#include <optional>

namespace IG::Input
{
class Event;
class Device;
}

namespace IG::FS
{
class AssetDirectoryIterator;
}

namespace IG
{

class Application;
struct ApplicationInitParams;
class PixelFormat;
class AssetIO;
class FileIO;

enum class Permission
{
	WRITE_EXT_STORAGE,
	COARSE_LOCATION
};

static constexpr uint32_t
	SYS_UI_STYLE_NO_FLAGS = 0,
	SYS_UI_STYLE_DIM_NAV = bit(0),
	SYS_UI_STYLE_HIDE_NAV = bit(1),
	SYS_UI_STYLE_HIDE_STATUS = bit(2);

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
		auto appStoragePtr = ::operator new(sizeof(T)); // allocate the storage
		setApplicationPtr((Application*)appStoragePtr); // point the context to the storage
		return *(new(appStoragePtr) T(IG_forward(args)...)); // construct the application with the storage
	}

	Application &application() const;
	void runOnMainThread(MainThreadMessageDelegate);
	void flushMainThreadMessages();

	Window *makeWindow(WindowConfig, WindowInitDelegate);
	const WindowContainer &windows() const;
	Window &mainWindow();
	bool systemAnimatesWindowRotation() const;
	IG::PixelFormat defaultWindowPixelFormat() const;

	const ScreenContainer &screens() const;
	Screen &mainScreen();

	NativeDisplayConnection nativeDisplayConnection() const;

	// App Callbacks

	// Called when another process sends the app a message
	void setOnInterProcessMessage(InterProcessMessageDelegate);
	void dispatchOnInterProcessMessage(const char *filename);
	bool hasOnInterProcessMessage() const;

	// Called when a Screen is connected/disconnected or its properties change
	void setOnScreenChange(ScreenChangeDelegate del);

	// Called when app returns from backgrounded state
	bool addOnResume(ResumeDelegate, int priority = APP_ON_RESUME_PRIORITY);
	bool removeOnResume(ResumeDelegate);
	bool containsOnResume(ResumeDelegate) const;
	void dispatchOnResume(bool focused);

	// Called when OS needs app to free any cached data
	void setOnFreeCaches(FreeCachesDelegate del);
	void dispatchOnFreeCaches(bool running);

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
	void openURL(IG::CStringView url) const;
	bool packageIsInstalled(IG::CStringView name) const;

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
	AssetIO openAsset(CStringView name, IOAccessHint access, FileOpenFlags oFlags = {}, const char *appName = applicationName) const;
	FS::AssetDirectoryIterator openAssetDirectory(IG::CStringView path, const char *appName = applicationName);

	// path/file access using OS-specific URIs such as those in the Android Storage Access Framework,
	// backwards compatible with regular file system paths, all thread-safe except for picker functions
	bool hasSystemPathPicker() const;
	void showSystemPathPicker(SystemDocumentPickerDelegate);
	bool hasSystemDocumentPicker() const;
	void showSystemDocumentPicker(SystemDocumentPickerDelegate);
	void showSystemCreateDocumentPicker(SystemDocumentPickerDelegate);
	FileIO openFileUri(CStringView uri, IOAccessHint, FileOpenFlags oFlags = {}) const;
	FileIO openFileUri(CStringView uri, FileOpenFlags oFlags = {}) const;
	UniqueFileDescriptor openFileUriFd(CStringView uri, FileOpenFlags oFlags = {}) const;
	bool fileUriExists(IG::CStringView uri) const;
	std::string fileUriFormatLastWriteTimeLocal(IG::CStringView uri) const;
	FS::FileString fileUriDisplayName(IG::CStringView uri) const;
	bool removeFileUri(IG::CStringView uri) const;
	bool renameFileUri(IG::CStringView oldUri, IG::CStringView newUri) const;
	bool createDirectoryUri(IG::CStringView uri) const;
	bool removeDirectoryUri(IG::CStringView uri) const;
	void forEachInDirectoryUri(IG::CStringView uri, FS::DirectoryEntryDelegate) const;

	// OS UI management (status & navigation bar)
	void setSysUIStyle(uint32_t flags);
	bool hasTranslucentSysUI() const;
	bool hasHardwareNavButtons() const;
	void setSystemOrientation(Orientation);
	Orientation defaultSystemOrientations() const;
	Orientation validateOrientationMask(Orientation oMask) const;
	bool hasDisplayCutout() const;

	// Sensors
	void setDeviceOrientationChangeSensor(bool on);

	// Notification/Launcher icons
	void addNotification(IG::CStringView onShow, IG::CStringView title, IG::CStringView message);
	void addLauncherIcon(IG::CStringView name, IG::CStringView path);

	// Power Management
	void setIdleDisplayPowerSave(bool on);
	void endIdleByUserActivity();

	// Permissions
	bool usesPermission(Permission p) const;
	bool permissionIsRestricted(Permission p) const;
	bool requestPermission(Permission p);

	// Input
	const InputDeviceContainer &inputDevices() const;
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

	// Called when a known input device addition/removal/change occurs
	void setOnInputDeviceChange(InputDeviceChangeDelegate);

	// Called when the device list is rebuilt, all devices should be re-checked
	void setOnInputDevicesEnumerated(InputDevicesEnumeratedDelegate);

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
