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

static_assert(__has_feature(objc_arc), "This file requires ARC");
#define LOGTAG "Base"
#import "MainApp.hh"
#import <dlfcn.h>
#import <unistd.h>
#include <imagine/base/Base.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "private.hh"
#include <imagine/fs/FS.hh>
#include <imagine/time/Time.hh>
#include <imagine/util/coreFoundation.h>
#include <imagine/util/string.h>
#include "../common/basePrivate.hh"
#include "../common/windowPrivate.hh"
#include "../common/screenPrivate.hh"
#include "ios.hh"

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/NSPathUtilities.h>

#if defined __ARM_ARCH_6K__
// firstObject is availible since iOS 4.0
// but not declared in the 5.1 SDK
@interface NSArray (FirstObject)
- (id)firstObject;
@end
#endif

namespace Base
{
	MainApp *mainApp{};
}

namespace Input
{
	int GSEVENTKEY_KEYCODE = sizeof(NSInteger) == 8 ? GSEVENTKEY_KEYCODE_64_BIT : 15;
	UITextField *vkbdField{};
	InputTextDelegate vKeyboardTextDelegate;
	IG::WindowRect textRect{8, 200, 8+304, 200+48};
}

namespace Base
{

bool isIPad = false;
static bool isRunningAsSystemApp = false;
CGColorSpaceRef grayColorSpace{}, rgbColorSpace{};
UIApplication *sharedApp{};
static const char *docPath{};
static FS::PathString appPath{};
static id onOrientationChangedObserver = nil;

#ifdef IPHONE_IMG_PICKER
static UIImagePickerController* imagePickerController;
static IPhoneImgPickerCallback imgPickCallback = NULL;
static void *imgPickUserPtr = NULL;
static NSData *imgPickData[2];
static uchar imgPickDataElements = 0;
#include "imagePicker.h"
#endif

#ifdef IPHONE_MSG_COMPOSE
static MFMailComposeViewController *composeController;
#include "mailCompose.h"
#endif

#ifdef IPHONE_GAMEKIT
#include "gameKit.h"
#endif

uint appState = APP_RUNNING;

uint appActivityState() { return appState; }

static Screen &setupUIScreen(UIScreen *screen, bool setOverscanCompensation)
{
	// prevent overscan compensation
	if(hasAtLeastIOS5() && setOverscanCompensation)
		screen.overscanCompensation = UIScreenOverscanCompensationInsetApplicationFrame;
	auto s = new Screen();
	s->init(screen);
	[s->displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	Screen::addScreen(s);
	return *s;
}

}

@implementation MainApp

#if defined IPHONE_VKEYBOARD
/*- (BOOL)textView:(UITextView *)textView shouldChangeTextInRange:(NSRange)range replacementText:(NSString *)text
{
	if (textView.text.length >= 127 && range.length == 0)
	{
		logMsg("not changing text");
		return NO;
	}
	return YES;
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
	logMsg("editing ended");
	Input::finishSysTextInput();
}*/

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	logMsg("pushed return");
	[textField resignFirstResponder];
	return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
	using namespace Input;
	logMsg("editing ended");
	//inVKeyboard = 0;
	auto delegate = IG::moveAndClear(vKeyboardTextDelegate);
	char text[256];
	string_copy(text, [textField.text UTF8String]);
	[textField removeFromSuperview];
	vkbdField = nil;
	if(delegate)
	{
		logMsg("running text entry callback");
		delegate(text);
	}
}

#endif

#if 0
- (void)keyboardWasShown:(NSNotification *)notification
{
	return;
	using namespace Base;
	#ifndef NDEBUG
	CGSize keyboardSize = [[[notification userInfo] objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;
	logMsg("keyboard shown with size %d", (int)keyboardSize.height * pointScale);
	int visibleY = IG::max(1, int(mainWin.rect.y2 - keyboardSize.height * pointScale));
	float visibleFraction = visibleY / mainWin.rect.y2;
	/*if(isIPad)
		Gfx::viewMMHeight_ = 197. * visibleFraction;
	else
		Gfx::viewMMHeight_ = 75. * visibleFraction;*/
	//generic_resizeEvent(mainWin.rect.x2, visibleY);
	#endif
	mainWin.postDraw();
}

- (void) keyboardWillHide:(NSNotification *)notification
{
	return;
	using namespace Base;
	logMsg("keyboard hidden");
	mainWin.postDraw();
}
#endif

static uint iOSOrientationToGfx(UIDeviceOrientation orientation)
{
	switch(orientation)
	{
		case UIDeviceOrientationPortrait: return Base::VIEW_ROTATE_0;
		case UIDeviceOrientationLandscapeLeft: return Base::VIEW_ROTATE_90;
		case UIDeviceOrientationLandscapeRight: return Base::VIEW_ROTATE_270;
		case UIDeviceOrientationPortraitUpsideDown: return Base::VIEW_ROTATE_180;
		default : return 0; // TODO: handle Face-up/down
	}
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	using namespace Base;
	if(Config::DEBUG_BUILD)
	{
		//logMsg("in didFinishLaunchingWithOptions(), UUID %s", [[[UIDevice currentDevice] uniqueIdentifier] cStringUsingEncoding: NSASCIIStringEncoding]);
		logMsg("iOS version %s", [[[UIDevice currentDevice] systemVersion] cStringUsingEncoding: NSASCIIStringEncoding]);
	}
	mainApp = self;
	sharedApp = [UIApplication sharedApplication];
	if(!Config::MACHINE_IS_GENERIC_ARMV6 && UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		isIPad = 1;
		logMsg("running on iPad");
	}
	Input::init();
	if(hasAtLeastIOS7())
	{
		#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
		[sharedApp setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
		#endif
		if(sizeof(NSInteger) == 4)
			Input::GSEVENTKEY_KEYCODE = Input::GSEVENTKEY_KEYCODE_IOS7;
	}
	else
	{
		#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000
		[sharedApp setStatusBarStyle:UIStatusBarStyleBlackOpaque animated:YES];
		#endif
	}

	//[nCenter addObserver:self selector:@selector(keyboardWasShown:) name:UIKeyboardDidShowNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
	#ifdef CONFIG_BASE_MULTI_SCREEN
	{
		NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
		[nCenter addObserverForName:UIScreenDidConnectNotification
			object:nil queue:nil usingBlock:
			^(NSNotification *note)
			{
				logMsg("screen connected");
				if(!screen_.freeSpace())
				{
					logWarn("max screens reached");
					return;
				}
				UIScreen *screen = [note object];
				for(auto s : screen_)
				{
					if(s->uiScreen() == screen)
					{
						logMsg("screen %p already in list", screen);
						return;
					}
				}
				auto &s = setupUIScreen(screen, true);
				if(Screen::onChange)
					Screen::onChange(s, {Screen::Change::ADDED});
			}];
		[nCenter addObserverForName:UIScreenDidDisconnectNotification
			object:nil queue:nil usingBlock:
			^(NSNotification *note)
			{
				logMsg("screen disconnected");
				UIScreen *screen = [note object];
				forEachInContainer(screen_, it)
				{
					Screen *removedScreen = *it;
					if(removedScreen->uiScreen() == screen)
					{
						it.erase();
						if(Screen::onChange)
							Screen::onChange(*removedScreen, { Screen::Change::REMOVED });
						[removedScreen->displayLink() removeFromRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
						removedScreen->deinit();
						delete removedScreen;
						break;
					}
				}
			}];
		if(Config::DEBUG_BUILD)
		{
			[nCenter addObserverForName:UIScreenModeDidChangeNotification
				object:nil queue:nil usingBlock:
				^(NSNotification *note)
				{
					logMsg("screen mode change");
				}];
		}
	}
	for(UIScreen *screen in [UIScreen screens])
	{
		setupUIScreen(screen, Screen::screens());
		if(!screen_.freeSpace())
		{
			logWarn("max screens reached");
			break;
		}
	}
	#else
	mainScreen().init([UIScreen mainScreen]);
	[mainScreen().displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	#endif
	// TODO: use NSProcessInfo
	onInit(0, nullptr);
	if(!deviceWindow())
		bug_unreachable("no main window created");
	logMsg("exiting didFinishLaunchingWithOptions");
	return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	using namespace Base;
	logMsg("resign active");
	iterateTimes(Window::windows(), i)
	{
		Window::window(i)->dispatchFocusChange(false);
	}
	Input::deinitKeyRepeatTimer();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	using namespace Base;
	logMsg("became active");
	iterateTimes(Window::windows(), i)
	{
		Window::window(i)->dispatchFocusChange(true);
	}
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	using namespace Base;
	logMsg("app exiting");
	Base::appState = APP_EXITING;
	dispatchOnExit(false);
	logMsg("app exited");
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	using namespace Base;
	logMsg("entering background");
	appState = APP_PAUSED;
	dispatchOnExit(true);
	Base::Screen::setActiveAll(false);
	Input::deinitKeyRepeatTimer();
	logMsg("entered background");
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	using namespace Base;
	logMsg("entered foreground");
	Base::appState = APP_RUNNING;
	Base::Screen::setActiveAll(true);
	iterateTimes(Window::windows(), i)
	{
		Window::window(i)->postDraw();
	}
	dispatchOnResume(true);
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	logMsg("got memory warning");
	Base::dispatchOnFreeCaches();
}

@end

@interface UIApplication ()
- (void)handleKeyUIEvent:(UIEvent *)event;
@end

@implementation MainUIApp

#ifndef __aarch64__
- (void)sendEvent:(UIEvent *)event
{
	[super sendEvent:event];
	if(sizeof(NSInteger) == 4 && Input::GSEVENTKEY_KEYCODE != Input::GSEVENTKEY_KEYCODE_IOS7)
		Input::handleKeyEvent(event);
}
#endif

#ifndef __ARM_ARCH_6K__
- (void)handleKeyUIEvent:(UIEvent *)event
{
	[super handleKeyUIEvent:event];
	Input::handleKeyEvent(event);
}
#endif

@end

namespace Base
{

void updateWindowSizeAndContentRect(Window &win, int width, int height, UIApplication *sharedApp)
{
	win.updateSize({width, height});
	win.updateContentRect(win.width(), win.height(), win.softOrientation(), sharedApp);
}

static void setStatusBarHidden(bool hidden)
{
	assert(sharedApp);
	logMsg("setting status bar hidden: %d", (int)hidden);
	[sharedApp setStatusBarHidden: (hidden ? YES : NO) withAnimation: UIStatusBarAnimationFade];
	if(deviceWindow())
	{
		auto &win = *deviceWindow();
		win.updateContentRect(win.width(), win.height(), win.softOrientation(), sharedApp);
		win.postDraw();
	}
}

void setSysUIStyle(uint flags)
{
	setStatusBarHidden(flags & SYS_UI_STYLE_HIDE_STATUS);
}

bool hasTranslucentSysUI()
{
	return hasAtLeastIOS7();
}

UIInterfaceOrientation gfxOrientationToUIInterfaceOrientation(uint orientation)
{
	using namespace Base;
	switch(orientation)
	{
		default: return UIInterfaceOrientationPortrait;
		case VIEW_ROTATE_270: return UIInterfaceOrientationLandscapeLeft;
		case VIEW_ROTATE_90: return UIInterfaceOrientationLandscapeRight;
		case VIEW_ROTATE_180: return UIInterfaceOrientationPortraitUpsideDown;
	}
}

void setDeviceOrientationChangeSensor(bool enable)
{
	UIDevice *currDev = [UIDevice currentDevice];
	auto notificationsAreOn = currDev.generatesDeviceOrientationNotifications;
	if(enable && !notificationsAreOn)
	{
		logMsg("enabling device orientation notifications");
		[currDev beginGeneratingDeviceOrientationNotifications];
	}
	else if(!enable && notificationsAreOn)
	{
		logMsg("disabling device orientation notifications");
		[currDev endGeneratingDeviceOrientationNotifications];
	}
}

void setOnDeviceOrientationChanged(DeviceOrientationChangedDelegate onOrientationChanged)
{
	if(onOrientationChanged)
	{
		NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
		if(onOrientationChangedObserver)
		{
			[nCenter removeObserver:onOrientationChangedObserver];
		}
		onOrientationChangedObserver = [nCenter addObserverForName:UIDeviceOrientationDidChangeNotification
		                               object:nil queue:nil usingBlock:
		                               ^(NSNotification *note)
		                               {
		                              	auto o = iOSOrientationToGfx([[UIDevice currentDevice] orientation]);
		                              	if(o)
		                              	{
		                              		onOrientationChanged(o);
		                              	}
		                               }];
	}
	else if(!onOrientationChanged && onOrientationChangedObserver)
	{
		[[NSNotificationCenter defaultCenter] removeObserver:onOrientationChangedObserver];
		onOrientationChangedObserver = nil;
	}

}

void setSystemOrientation(uint o)
{
	logMsg("setting system orientation %s", orientationToStr(o));
	using namespace Input;
	if(vKeyboardTextDelegate) // TODO: allow orientation change without aborting text input
	{
		logMsg("aborting active text input");
		vKeyboardTextDelegate(nullptr);
		vKeyboardTextDelegate = {};
	}
	assert(sharedApp);
	[sharedApp setStatusBarOrientation:gfxOrientationToUIInterfaceOrientation(o) animated:YES];
	if(deviceWindow())
	{
		auto &win = *deviceWindow();
		win.updateContentRect(win.width(), win.height(), win.softOrientation(), sharedApp);
		win.postDraw();
	}
}

uint defaultSystemOrientations()
{
	return Base::isIPad ? VIEW_ROTATE_ALL : VIEW_ROTATE_ALL_BUT_UPSIDE_DOWN;
}

void setOnSystemOrientationChanged(SystemOrientationChangedDelegate del)
{
	// TODO
}

void exit(int returnVal)
{
	appState = APP_EXITING;
	dispatchOnExit(false);
	::exit(returnVal);
}
void abort() { ::abort(); }

void openURL(const char *url)
{
	[sharedApp openURL:[NSURL URLWithString:
		[NSString stringWithCString:url encoding:NSASCIIStringEncoding]]];
}

void setIdleDisplayPowerSave(bool on)
{
	assert(sharedApp);
	sharedApp.idleTimerDisabled = on ? NO : YES;
	logMsg("set idleTimerDisabled %d", (int)sharedApp.idleTimerDisabled);
}

void endIdleByUserActivity()
{
	if(!sharedApp.idleTimerDisabled)
	{
		sharedApp.idleTimerDisabled = YES;
		sharedApp.idleTimerDisabled = NO;
	}
}

FS::PathString assetPath() { return appPath; }

FS::PathString documentsPath()
{
	if(isRunningAsSystemApp)
		return {"/User/Library/Preferences"};
	else
	{
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectory = [paths firstObject];
		if(documentsDirectory)
			FS::makePathString([documentsDirectory cStringUsingEncoding: NSASCIIStringEncoding]);
		return {FS::makePathString([documentsDirectory cStringUsingEncoding: NSASCIIStringEncoding])};
	}
}

FS::PathString storagePath()
{
	if(isRunningAsSystemApp)
		return {"/User/Media"};
	else
		return documentsPath();
}

FS::PathString libPath()
{
	return appPath;
}

bool documentsPathIsShared()
{
	return isRunningAsSystemApp;
}

bool deviceIsIPad()
{
	return isIPad;
}

bool isSystemApp()
{
	return isRunningAsSystemApp;
}

bool hasAtLeastIOS5()
{
	return !Config::MACHINE_IS_GENERIC_ARMV6;
}

bool hasAtLeastIOS7()
{
	return !Config::MACHINE_IS_GENERIC_ARMV6 &&
			kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_7_0;
}

bool hasAtLeastIOS8()
{
	return !Config::MACHINE_IS_GENERIC_ARMV6 &&
			kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_8_0;
}

bool usesPermission(Permission p)
{
	return false;
}

bool requestPermission(Permission p)
{
	return false;
}

void registerInstance(const char *appID, int argc, char** argv) {}

void setAcceptIPC(const char *appID, bool on) {}

void addNotification(const char *onShow, const char *title, const char *message) {}

void addLauncherIcon(const char *name, const char *path) {}

bool hasVibrator() { return false; }

void vibrate(uint ms) {}

void exitWithErrorMessageVPrintf(int exitVal, const char *format, va_list args)
{
	std::array<char, 512> msg{};
	auto result = vsnprintf(msg.data(), msg.size(), format, args);
	logErr("%s", msg.data());
	exit(exitVal);
}

#ifdef CONFIG_BASE_IOS_SETUID

uid_t realUID = 0, effectiveUID = 0;
static void setupUID()
{
	realUID = getuid();
	effectiveUID = geteuid();
	seteuid(realUID);
}

void setUIDReal()
{
	seteuid(Base::realUID);
}

bool setUIDEffective()
{
	return seteuid(Base::effectiveUID) == 0;
}

#endif

}

int main(int argc, char *argv[])
{
	using namespace Base;
	// check if running from system apps directory
	if(strlen(argv[0]) >= 14 && memcmp(argv[0], "/Applications/", 14) == 0)
	{
		logMsg("launched as system app from: %s", argv[0]);
		isRunningAsSystemApp = true;
	}
	#ifdef CONFIG_BASE_IOS_SETUID
	setupUID();
	#endif
	engineInit();
	logger_init();
	appPath = FS::makeAppPathFromLaunchCommand(argv[0]);
	
	#ifdef CONFIG_BASE_IOS_SETUID
	logMsg("real UID %d, effective UID %d", realUID, effectiveUID);
	if(access("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", F_OK) == 0)
	{
		logMsg("manually loading Backgrounder.dylib");
		dlopen("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", RTLD_LAZY | RTLD_GLOBAL);
	}
	#endif

	Base::grayColorSpace = CGColorSpaceCreateDeviceGray();
	Base::rgbColorSpace = CGColorSpaceCreateDeviceRGB();

	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, @"MainUIApp", @"MainApp");
	}
}
