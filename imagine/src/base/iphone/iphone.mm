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
#import <imagine/base/iphone/EAGLView.hh>
#import <dlfcn.h>
#import <unistd.h>

#include <imagine/base/Base.hh>
#include <imagine/base/GLContext.hh>
#include "private.hh"
#include <imagine/gfx/Gfx.hh>
#include <imagine/fs/sys.hh>
#include <imagine/util/time/sys.hh>
#include <imagine/util/coreFoundation.h>
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
	MainApp *mainApp = nullptr;
}

#if defined IPHONE_VKEYBOARD
namespace Input
{
	//static UITextView *vkbdField = nil;
	UITextField *vkbdField = nil;
	//static bool inVKeyboard = 0;
	InputTextDelegate vKeyboardTextDelegate;
	IG::WindowRect textRect(8, 200, 8+304, 200+48);
}
#endif

#ifdef CONFIG_INPUT_ICADE
#include "ICadeHelper.hh"
namespace Input
{
	ICadeHelper iCade {nil};
}
#endif

namespace Base
{

bool isIPad = false;
static bool isRunningAsSystemApp = false;
CGColorSpaceRef grayColorSpace = nullptr, rgbColorSpace = nullptr;
UIApplication *sharedApp = nullptr;
static const char *docPath = nullptr;
static FsSys::PathString appPath{};

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
	auto delegate = moveAndClear(vKeyboardTextDelegate);
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

- (void)screenDidConnect:(NSNotification *)aNotification
{
	using namespace Base;
	logMsg("screen connected");
	if(!screen_.freeSpace())
	{
		logWarn("max screens reached");
		return;
	}
	UIScreen *screen = [aNotification object];
	for(auto s : screen_)
	{
		if(s->uiScreen() == screen)
		{
			logMsg("screen %p already in list", screen);
			return;
		}
	}
	// prevent overscan compensation
	screen.overscanCompensation = UIScreenOverscanCompensationInsetApplicationFrame;
	auto s = new Screen();
	s->init(screen);
	[s->displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	Screen::addScreen(s);
	if(Screen::onChange)
		Screen::onChange(*s, { Screen::Change::ADDED });
}

- (void)screenDidDisconnect:(NSNotification *)aNotification
{
	using namespace Base;
	logMsg("screen disconnected");
	UIScreen *screen = [aNotification object];
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
}

- (void)screenModeDidChange:(NSNotification *)aNotification
{
	logMsg("screen mode change");
}

static uint iOSOrientationToGfx(UIDeviceOrientation orientation)
{
	switch(orientation)
	{
		case UIDeviceOrientationPortrait: return Base::VIEW_ROTATE_0;
		case UIDeviceOrientationLandscapeLeft: return Base::VIEW_ROTATE_90;
		case UIDeviceOrientationLandscapeRight: return Base::VIEW_ROTATE_270;
		case UIDeviceOrientationPortraitUpsideDown: return Base::VIEW_ROTATE_180;
		default : return 255; // TODO: handle Face-up/down
	}
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	using namespace Base;
	#if !defined NDEBUG
	//logMsg("in didFinishLaunchingWithOptions(), UUID %s", [[[UIDevice currentDevice] uniqueIdentifier] cStringUsingEncoding: NSASCIIStringEncoding]);
	logMsg("iOS version %s", [[[UIDevice currentDevice] systemVersion] cStringUsingEncoding: NSASCIIStringEncoding]);
	#endif
	mainApp = self;
	sharedApp = [UIApplication sharedApplication];

	bool usingIOS7 = false;
	#ifndef __ARM_ARCH_6K__
	if(kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber_iOS_7_0)
	{
		usingIOS7 = true;
	}
	if(usingIOS7)
		[sharedApp setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
	#endif
	
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
	[nCenter addObserver:self selector:@selector(orientationChanged:) name:UIDeviceOrientationDidChangeNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWasShown:) name:UIKeyboardDidShowNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
	#endif
	#ifdef CONFIG_BASE_MULTI_SCREEN
	{
		NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
		[nCenter addObserver:self selector:@selector(screenDidConnect:) name:UIScreenDidConnectNotification object:nil];
		[nCenter addObserver:self selector:@selector(screenDidDisconnect:) name:UIScreenDidDisconnectNotification object:nil];
		[nCenter addObserver:self selector:@selector(screenModeDidChange:) name:UIScreenModeDidChangeNotification object:nil];
	}
	for(UIScreen *screen in [UIScreen screens])
	{
		if(Screen::screens())
		{
			// prevent overscan compensation on secondary screens
			screen.overscanCompensation = UIScreenOverscanCompensationInsetApplicationFrame;
		}
		auto s = new Screen();
		s->init(screen);
		[s->displayLink() addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		Screen::addScreen(s);
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
	doOrAbort(onInit(0, nullptr));
	if(!deviceWindow())
		bug_exit("no main window created");
	logMsg("exiting didFinishLaunchingWithOptions");
	return YES;
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION
- (void)orientationChanged:(NSNotification *)notification
{
	using namespace Base;
	uint o = iOSOrientationToGfx([[UIDevice currentDevice] orientation]);
	if(o == 255)
		return;
	if(o == Base::VIEW_ROTATE_180 && !Base::isIPad)
		return; // ignore upside-down orientation unless using iPad
	logMsg("new orientation %s", Base::orientationToStr(o));
	deviceWindow()->preferedOrientation = o;
	deviceWindow()->setOrientation(deviceWindow()->preferedOrientation, true);
}
#endif

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
	Base::Screen::unpostAll();
	#ifdef CONFIG_INPUT_ICADE
	Input::iCade.didEnterBackground();
	#endif
	Input::deinitKeyRepeatTimer();
	iterateTimes(Window::windows(), i)
	{
		[Window::window(i)->glView() deleteDrawable];
	}
	GLContext::setDrawable(nullptr);
	glFinish();
	logMsg("entered background");
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
	using namespace Base;
	logMsg("entered foreground");
	Base::appState = APP_RUNNING;
	iterateTimes(Window::windows(), i)
	{
		Window::window(i)->postDraw();
	}
	dispatchOnResume(true);
	#ifdef CONFIG_INPUT_ICADE
	Input::iCade.didBecomeActive();
	#endif
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	logMsg("got memory warning");
	Base::dispatchOnFreeCaches();
}

@end

namespace Base
{

void updateWindowSizeAndContentRect(Window &win, int width, int height, UIApplication *sharedApp)
{
	win.updateSize({width, height});
	win.updateContentRect(win.width(), win.height(), win.rotateView, sharedApp);
}

static void setStatusBarHidden(bool hidden)
{
	assert(sharedApp);
	logMsg("setting status bar hidden: %d", (int)hidden);
	#if __IPHONE_OS_VERSION_MIN_REQUIRED >= 30200
	[sharedApp setStatusBarHidden: (hidden ? YES : NO) withAnimation: UIStatusBarAnimationFade];
	#else
	[sharedApp setStatusBarHidden: (hidden ? YES : NO) animated:YES];
	#endif
	if(deviceWindow())
	{
		auto &win = *deviceWindow();
		win.updateContentRect(win.width(), win.height(), win.rotateView, sharedApp);
		win.postDraw();
	}
}

void setSysUIStyle(uint flags)
{
	setStatusBarHidden(flags & SYS_UI_STYLE_HIDE_STATUS);
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

#ifdef CONFIG_GFX_SOFT_ORIENTATION
void Window::setSystemOrientation(uint o)
{
	using namespace Input;
	if(vKeyboardTextDelegate) // TODO: allow orientation change without aborting text input
	{
		logMsg("aborting active text input");
		vKeyboardTextDelegate(nullptr);
		vKeyboardTextDelegate = {};
	}
	assert(sharedApp);
	[sharedApp setStatusBarOrientation:gfxOrientationToUIInterfaceOrientation(o) animated:YES];
	updateContentRect(width(), height(), rotateView, sharedApp);
}

static bool autoOrientationState = 0; // Turned on in applicationDidFinishLaunching

void Window::setAutoOrientation(bool on)
{
	if(autoOrientationState == on)
		return;
	autoOrientationState = on;
	logMsg("set auto-orientation: %d", on);
	if(on)
		[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	else
	{
		deviceWindow()->preferedOrientation = deviceWindow()->rotateView;
		[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
	}
}
#endif

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

const char *assetPath() { return appPath.data(); }

const char *documentsPath()
{
	if(!docPath)
	{
		if(isRunningAsSystemApp)
			return "/User/Library/Preferences";
		else
		{
			NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
			NSString *documentsDirectory = [paths firstObject];
			if(documentsDirectory)
				docPath = strdup([documentsDirectory cStringUsingEncoding: NSASCIIStringEncoding]);
			else
				docPath = "";
		}
	}
	return docPath;
}

const char *storagePath()
{
	if(isRunningAsSystemApp)
		return "/User/Media";
	else
		return documentsPath();
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

double TimeMach::timebaseNSec = 0, TimeMach::timebaseUSec = 0,
	TimeMach::timebaseMSec = 0, TimeMach::timebaseSec = 0;

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
	doOrAbort(logger_init());
	TimeMach::setTimebase();
	appPath = makeAppPathFromLaunchCommand(argv[0]);
	
	#ifdef CONFIG_BASE_IOS_SETUID
	logMsg("real UID %d, effective UID %d", realUID, effectiveUID);
	if(access("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", F_OK) == 0)
	{
		logMsg("manually loading Backgrounder.dylib");
		dlopen("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", RTLD_LAZY | RTLD_GLOBAL);
	}
	#endif
	
	#if !defined __ARM_ARCH_6K__
	if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		isIPad = 1;
		logMsg("running on iPad");
	}
	#endif
	
	doOrAbort(Input::init());
	
	#ifdef CONFIG_AUDIO
	Audio::initSession();
	#endif

	Base::grayColorSpace = CGColorSpaceCreateDeviceGray();
	Base::rgbColorSpace = CGColorSpaceCreateDeviceRGB();

	@autoreleasepool
	{
		return UIApplicationMain(argc, argv, nil, @"MainApp");
	}
}
