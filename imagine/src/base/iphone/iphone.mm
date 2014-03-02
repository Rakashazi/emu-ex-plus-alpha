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
#import "MainApp.h"
#import "EAGLView.h"
#import <dlfcn.h>
#import <unistd.h>

#include <base/Base.hh>
#include <base/iphone/private.hh>
#include <gfx/Gfx.hh>
#include <fs/sys.hh>
#include <config/machine.hh>
#include <util/time/sys.hh>
#include <base/common/funcs.h>
#include <base/common/windowPrivate.hh>

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <Foundation/NSPathUtilities.h>

namespace Base
{
	MainApp *mainApp = nullptr;
	#ifdef CONFIG_BASE_IOS_RETINA_SCALE
	uint screenPointScale = 1;
	#endif
}


#if defined(CONFIG_INPUT) && defined(IPHONE_VKEYBOARD)
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
namespace Base
{
	ICadeHelper iCade {nil};
}
#endif

namespace Base
{

const char *appPath = 0;
EAGLContext *mainContext = nullptr;
CADisplayLink *displayLink = nullptr;
BOOL displayLinkActive = NO;
bool isIPad = 0;
CGColorSpaceRef grayColorSpace = nullptr, rgbColorSpace = nullptr;
UIApplication *sharedApp = nullptr;

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

#if defined(CONFIG_INPUT) && defined(IPHONE_VKEYBOARD)
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

/*- (void) screenDidConnect:(NSNotification *)aNotification
{
	logMsg("New screen connected");
	UIScreen *screen = [aNotification object];
	UIScreenMode *mode = [[screen availibleModes] lastObject];
	screen.currentMode = mode;
	if(!externalWindow)
	{
		externalWindow = [UIWindow alloc];
	}
	CGRect rect = CGRectMake(0, 0, mode.size.width, mode.size.height);
	[externalWindow initWithFrame:rect];
	externalWindow.screen = screen;
	[externalWindow makeKeyAndVisible];
}
 
- (void) screenDidDisconnect:(NSNotification *)aNotification
{
	logMsg("Screen dis-connected");
}
 
- (void) screenModeDidChange:(NSNotification *)aNotification
{
	UIScreen *screen = [aNotification object];
	logMsg("Screen-mode change"); // [screen currentMode]
}*/

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

- (void)displayLinkCallback
{
	//logMsg("screen update");
	/*TimeSys now;
	now.setTimeNow();
	logMsg("frame time stamp %f, duration %f, now %f", displayLink.timestamp, displayLink.duration, (float)now);*/
	Base::frameUpdate(Base::displayLink.timestamp);
	if(!Base::mainScreen().frameIsPosted())
	{
		//logMsg("stopping screen updates");
		Base::mainScreen().unpostFrame();
	}
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	using namespace Base;
	NSString *currSysVer = [[UIDevice currentDevice] systemVersion];
	#if !defined NDEBUG
	//logMsg("in didFinishLaunchingWithOptions(), UUID %s", [[[UIDevice currentDevice] uniqueIdentifier] cStringUsingEncoding: NSASCIIStringEncoding]);
	logMsg("iOS version %s", [currSysVer cStringUsingEncoding: NSASCIIStringEncoding]);
	#endif
	mainApp = self;
	sharedApp = [UIApplication sharedApplication];

	bool usingIOS7 = false;
	#ifndef __ARM_ARCH_6K__
	if([currSysVer compare:@"7.0" options:NSNumericSearch] != NSOrderedAscending)
	{
		usingIOS7 = true;
	}
	#endif
	/*logMsg("enabling iOS 3.2 external display features");
	NSNotificationCenter *center = [NSNotificationCenter defaultCenter];
	[center addObserver:self selector:@selector(screenDidConnect:) name:UIScreenDidConnectNotification object:nil];
	[center addObserver:self selector:@selector(screenDidDisconnect:) name:UIScreenDidDisconnectNotification object:nil];
	[center addObserver:self selector:@selector(screenModeDidChange:) name:UIScreenModeDidChangeNotification object:nil];*/
	#ifndef __ARM_ARCH_6K__
	if(usingIOS7)
		[sharedApp setStatusBarStyle:UIStatusBarStyleLightContent animated:YES];
	#endif
	
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	NSNotificationCenter *nCenter = [NSNotificationCenter defaultCenter];
	[nCenter addObserver:self selector:@selector(orientationChanged:) name:UIDeviceOrientationDidChangeNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWasShown:) name:UIKeyboardDidShowNotification object:nil];
	//[nCenter addObserver:self selector:@selector(keyboardWillHide:) name:UIKeyboardWillHideNotification object:nil];
	#endif
	Base::displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(displayLinkCallback)];
	Base::displayLinkActive = YES;
	[Base::displayLink setFrameInterval:1];
	[Base::displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
	#ifdef CONFIG_BASE_IOS_RETINA_SCALE
	if([UIScreen mainScreen].scale == 2.0)
	{
		logMsg("running on Retina Display");
		screenPointScale = 2;
	}
	else
		logMsg("not running on Retina Display");
	#endif
	// TODO: use NSProcessInfo
	doOrAbort(onInit(0, nullptr));
	if(!mainWindow())
		bug_exit("no main window created");
	logMsg("exiting didFinishLaunchingWithOptions");
	return YES;
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION
- (void)orientationChanged:(NSNotification *)notification
{
	uint o = iOSOrientationToGfx([[UIDevice currentDevice] orientation]);
	if(o == 255)
		return;
	if(o == Base::VIEW_ROTATE_180 && !Base::isIPad)
		return; // ignore upside-down orientation unless using iPad
	logMsg("new orientation %s", Base::orientationToStr(o));
	Base::mainWindow().preferedOrientation = o;
	Base::mainWindow().setOrientation(Base::mainWindow().preferedOrientation, true);
}
#endif

- (void)applicationWillResignActive:(UIApplication *)application
{
	logMsg("resign active");
	Base::mainScreen().unpostFrame();
	glFinish();
	Input::deinitKeyRepeatTimer();
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	using namespace Base;
	logMsg("became active");
	[Base::mainWindow().glView() bindDrawable];
	onSetAsDrawTarget(Base::mainWindow()); // update viewport after window is shown
	Base::appState = APP_RUNNING;
	//if(Base::displayLink)
	//	mainScreen().postFrame();
	Base::onResume(1);
	#ifdef CONFIG_INPUT_ICADE
	iCade.didBecomeActive();
	#endif
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	using namespace Base;
	logMsg("app exiting");
	Base::appState = APP_EXITING;
	Base::onExit(false);
	logMsg("app exited");
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
	using namespace Base;
	logMsg("entering background");
	appState = APP_PAUSED;
	Base::onExit(true);
	mainScreen().unpostFrame();
	#ifdef CONFIG_INPUT_ICADE
	iCade.didEnterBackground();
	#endif
	glFinish();
	[Base::mainWindow().glView() deleteDrawable];
	Input::deinitKeyRepeatTimer();
	logMsg("entered background");
}

@end

namespace Base
{

void nsLog(const char* str)
{
	NSLog(@"%s", str);
}

void nsLogv(const char* format, va_list arg)
{
	auto formatStr = [[NSString alloc] initWithBytesNoCopy:(void*)format length:strlen(format) encoding:NSUTF8StringEncoding freeWhenDone:false];
	NSLogv(formatStr, arg);
}

uint Screen::refreshRate()
{
	return 60;
}

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
	auto &win = mainWindow();
	win.updateContentRect(win.width(), win.height(), win.rotateView, sharedApp);
	win.postResize();
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
	//auto &win = mainWindow();
	
	//updateWindowSizeAndContentRect(win, win.realWidth(), win.realHeight(), sharedApp);
	//mainWindow().postResize();
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
		mainWindow().preferedOrientation = mainWindow().rotateView;
		[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
	}
}
#endif

void exit(int returnVal)
{
	appState = APP_EXITING;
	onExit(0);
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

static const char *docPath = 0;

const char *documentsPath()
{
	if(!docPath)
	{
		#ifdef CONFIG_BASE_IOS_JB
		return "/User/Library/Preferences";
		#else
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		NSString *documentsDirectory = [paths objectAtIndex:0];
		docPath = strdup([documentsDirectory cStringUsingEncoding: NSASCIIStringEncoding]);
		#endif
	}
	return docPath;
}

const char *storagePath()
{
	#ifdef CONFIG_BASE_IOS_JB
	return "/User/Media";
	#else
	return documentsPath();
	#endif
}

bool deviceIsIPad()
{
	return isIPad;
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

bool Screen::supportsFrameTime()
{
	return true;
}

}

double TimeMach::timebaseNSec = 0, TimeMach::timebaseUSec = 0,
	TimeMach::timebaseMSec = 0, TimeMach::timebaseSec = 0;

int main(int argc, char *argv[])
{
	using namespace Base;
	#ifdef CONFIG_BASE_IOS_SETUID
	setupUID();
	#endif
	
	engineInit();
	doOrAbort(logger_init());
	TimeMach::setTimebase();
	
	#ifdef CONFIG_BASE_IOS_SETUID
	logMsg("real UID %d, effective UID %d", realUID, effectiveUID);
	if(access("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", F_OK) == 0)
	{
		logMsg("manually loading Backgrounder.dylib");
		dlopen("/Library/MobileSubstrate/DynamicLibraries/Backgrounder.dylib", RTLD_LAZY | RTLD_GLOBAL);
	}
	#endif

	#ifdef CONFIG_FS
	FsPosix::changeToAppDir(argv[0]);
	#endif
	
	#if !defined(__ARM_ARCH_6K__) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 30200)
	if(UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
	{
		isIPad = 1;
		logMsg("running on iPad");
	}
	#endif
	
	#ifdef CONFIG_INPUT
	doOrAbort(Input::init());
	#endif
	
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
