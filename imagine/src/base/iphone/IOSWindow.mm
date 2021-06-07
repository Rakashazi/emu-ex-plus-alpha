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
#define LOGTAG "IOSWindow"
#import "MainApp.hh"
#import <QuartzCore/QuartzCore.h>
#include "private.hh"
#include <imagine/base/Application.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Screen.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "ios.hh"

#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER GL_RENDERBUFFER_OES
#endif

namespace Base
{

#ifndef CONFIG_GFX_SOFT_ORIENTATION
static Orientation validO = UIInterfaceOrientationMaskAllButUpsideDown;
#endif

#ifndef CONFIG_BASE_IOS_RETINA_SCALE
constexpr CGFloat IOSWindow::pointScale;
#endif

UIInterfaceOrientation gfxOrientationToUIInterfaceOrientation(Orientation orientation);

const char *uiInterfaceOrientationToStr(UIInterfaceOrientation o)
{
	switch(o)
	{
		case UIInterfaceOrientationPortrait: return "Portrait";
		case UIInterfaceOrientationPortraitUpsideDown: return "Portrait Upside Down";
		case UIInterfaceOrientationLandscapeLeft: return "Landscape Left";
		case UIInterfaceOrientationLandscapeRight: return "Landscape Right";
		default: return "Unknown";
	}
}

#ifndef CONFIG_GFX_SOFT_ORIENTATION
static Orientation defaultValidOrientationMask()
{
	return Base::isIPad ? UIInterfaceOrientationMaskAll : UIInterfaceOrientationMaskAllButUpsideDown;
}

bool Window::setValidOrientations(Orientation oMask)
{
	oMask = appContext().validateOrientationMask(oMask);
	validO = 0;
	if(oMask & VIEW_ROTATE_0)
		validO |= UIInterfaceOrientationMaskPortrait;
	if(oMask & VIEW_ROTATE_90)
		validO |= UIInterfaceOrientationMaskLandscapeLeft;
	if(oMask & VIEW_ROTATE_180)
		validO |= UIInterfaceOrientationMaskPortraitUpsideDown;
	if(oMask & VIEW_ROTATE_270)
		validO |= UIInterfaceOrientationMaskLandscapeRight;
	auto currO = [uiApp() statusBarOrientation];
	logMsg("set valid orientation mask 0x%X, current orientation: %s", validO, uiInterfaceOrientationToStr(currO));
	if(!(validO & (1 << currO)))
	{
		logMsg("current orientation no longer valid, resetting root view controller");
		auto rootViewCtrl = uiWin().rootViewController;
		uiWin().rootViewController = nil;
		uiWin().rootViewController = rootViewCtrl;
	}
	else
		[UIViewController attemptRotationToDeviceOrientation];
	return true;
}

bool Window::requestOrientationChange(Orientation o)
{
	// no-op, OS manages orientation changes
	return false;
}
#endif

Window *IOSApplication::deviceWindow() const
{
	if(windows().size()) [[likely]]
		return windows()[0].get();
	return nullptr;
}

Window *IOSApplicationContext::deviceWindow() const
{
	return application().deviceWindow();
}

IG::PixelFormat ApplicationContext::defaultWindowPixelFormat() const
{
	return Config::MACHINE_IS_GENERIC_ARMV6 ? PIXEL_RGB565 : PIXEL_RGBA8888;
}

bool Window::hasSurface() const
{
	return true;
}

IG::WindowRect Window::contentBounds() const
{
	return contentRect;
}

void IOSWindow::updateContentRect(int width, int height, uint32_t softOrientation)
{
	contentRect.x = contentRect.y = 0;
	contentRect.x2 = width;
	contentRect.y2 = height;
	auto sharedApp = uiApp();
	bool hasStatusBar = isDeviceWindow() && !sharedApp.statusBarHidden;
	//logMsg("has status bar %d", hasStatusBar);
	if(hasStatusBar)
	{
		CGFloat statusBarHeight;
		if(!Config::SYSTEM_ROTATES_WINDOWS)
		{
			bool isSideways = softOrientation == VIEW_ROTATE_90 || softOrientation == VIEW_ROTATE_270;
			statusBarHeight = (isSideways ? sharedApp.statusBarFrame.size.width : sharedApp.statusBarFrame.size.height) * pointScale;
			bool statusBarBeginsOnWindowOrigin = softOrientation == VIEW_ROTATE_0 || softOrientation == VIEW_ROTATE_270;
			if(statusBarBeginsOnWindowOrigin)
				contentRect.y = statusBarHeight;
			else
				contentRect.y2 -= statusBarHeight;
		}
		else
		{
			statusBarHeight = sharedApp.statusBarFrame.size.height * pointScale;
			contentRect.y = statusBarHeight;
		}
		logMsg("adjusted content rect to %d:%d:%d:%d for status bar height %d",
			contentRect.x, contentRect.y, contentRect.x2, contentRect.y2, (int)statusBarHeight);
	}
	else
	{
		logMsg("using full window size for content rect %d,%d", contentRect.x2, contentRect.y2);
	}
	surfaceChangeFlags |= WindowSurfaceChange::CONTENT_RECT_RESIZED;
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	uint32_t dpi = 163 * pointScale;
	#if !defined __ARM_ARCH_6K__
	if(isIPad)
	{
		// based on iPad DPI of 132 (264 retina)
		dpi = 132 * pointScale;
	}
	#endif
	return {(size.x / (float)dpi) * 25.4f, (size.y / (float)dpi) * 25.4f};
}

Window::Window(ApplicationContext ctx, WindowConfig config, InitDelegate):
	IOSWindow{ctx, config}
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	this->screen_ = &config.screen(ctx);
	#endif
	CGRect rect = screen()->uiScreen().bounds;
	// Create a full-screen window
	uiWin_ = (void*)CFBridgingRetain([[UIWindow alloc] initWithFrame:rect]);
	#ifdef CONFIG_BASE_IOS_RETINA_SCALE
	pointScale = hasAtLeastIOS8() ? [screen()->uiScreen() nativeScale] : [screen()->uiScreen() scale];
	if(pointScale > 1.)
		logMsg("using point scale: %f", (double)pointScale);
	#endif
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	validO = defaultValidOrientationMask();
	#endif
	updateWindowSizeAndContentRect(rect.size.width * pointScale, rect.size.height * pointScale);
	if(*screen() != ctx.mainScreen())
	{
		uiWin().screen = screen()->uiScreen();
	}
	//logMsg("setting root view controller");
	auto rootViewCtrl = [[ImagineUIViewController alloc] init];
	#if __IPHONE_OS_VERSION_MIN_REQUIRED < 70000
	rootViewCtrl.wantsFullScreenLayout = YES;
	#endif
	uiWin().rootViewController = rootViewCtrl;
}

IOSWindow::~IOSWindow()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(uiWin_)
	{
		logMsg("deinit window %p", uiWin_);
		CFRelease(uiWin_);
	}
	#endif
}

void Window::show()
{
	logMsg("showing window");
	if(isDeviceWindow())
		[uiWin() makeKeyAndVisible];
	else
		uiWin().hidden = NO;
	postDraw();
}

bool Window::operator ==(Window const &rhs) const
{
	return uiWin_ == rhs.uiWin_;
}

Window *windowForUIWindow(ApplicationContext ctx, UIWindow *uiWin)
{
	for(auto &w : ctx.windows())
	{
		if(w->uiWin() == uiWin)
			return w.get();
	}
	return nullptr;
}

void Window::setTitle(const char *name) {}

void Window::setAcceptDnd(bool on) {}

NativeWindow Window::nativeObject() const
{
	return uiWin_;
}

void Window::setFormat(NativeWindowFormat) {}

void Window::setFormat(IG::PixelFormat) {}

void IOSWindow::updateWindowSizeAndContentRect(int width, int height)
{
	auto &asWindow = *static_cast<Window*>(this);
	asWindow.updateSize({width, height});
	asWindow.updateContentRect(asWindow.width(), asWindow.height(), asWindow.softOrientation());
}

bool IOSWindow::isDeviceWindow() const
{
	auto deviceWinPtr = static_cast<const Window*>(this)->appContext().deviceWindow();
	return !deviceWinPtr || deviceWinPtr->uiWin_ == uiWin_;
}

UIApplication *IOSWindow::uiApp() const
{
	return static_cast<const Window*>(this)->appContext().uiApp();
}

IG::PixelFormat Window::pixelFormat() const
{
	return IG::PIXEL_FMT_RGBA8888;
}

void Window::setIntendedFrameRate(double rate) {}

void WindowConfig::setFormat(IG::PixelFormat) {}

}

@implementation ImagineUIViewController

#ifdef CONFIG_GFX_SOFT_ORIENTATION

- (BOOL)shouldAutorotate
{
	return NO;
}

	#ifndef NDEBUG
	// for iOS 6 and up (testing-only, this OS should use GLKit for orientations)
	- (NSUInteger)supportedInterfaceOrientations
	{
		return 1 << UIInterfaceOrientationPortrait;
	}
	
	// for iOS 5 (testing-only, this OS should use GLKit for orientations)
	- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
	{
		//logMsg("reporting if should autorotate to: %s", Base::uiInterfaceOrientationToStr(interfaceOrientation));
		return interfaceOrientation == UIInterfaceOrientationPortrait;
	}
	#endif

#else

/*- (BOOL)shouldAutorotate
{
	//logMsg("reporting if should autorotate");
	return YES;
}*/

// for iOS 6 and up
- (NSUInteger)supportedInterfaceOrientations
{
	//logMsg("reporting supported orientations");
	return Base::validO;
}

// for iOS 5
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	//logMsg("reporting if should autorotate to: %s", Base::uiInterfaceOrientationToStr(interfaceOrientation));
	return (Base::validO & (1 << interfaceOrientation)) ? YES : NO;
}

#ifndef NDEBUG
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration
{
	logMsg("animating to new orientation");
}
#endif

//- (UIStatusBarStyle)preferredStatusBarStyle
//{
//	logMsg("reporting preferred status bar style");
//	return UIStatusBarStyleLightContent;
//}
//
//- (BOOL)prefersStatusBarHidden
//{
//	logMsg("reporting prefers status bar hidden");
//	return Base::hideStatusBar;
//}

#endif

@end
