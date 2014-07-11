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
#import <imagine/base/iphone/EAGLView.hh>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#include "../common/windowPrivate.hh"
#include "private.hh"
#include <imagine/base/Base.hh>
#include <imagine/base/GLContext.hh>
#include <imagine/gfx/Gfx.hh>
#include "ios.hh"

#ifndef GL_RENDERBUFFER
#define GL_RENDERBUFFER GL_RENDERBUFFER_OES
#endif

@interface ImagineUIViewController : UIViewController

@end

namespace Base
{

#ifndef CONFIG_GFX_SOFT_ORIENTATION
static uint validO = UIInterfaceOrientationMaskAllButUpsideDown;
#endif

UIInterfaceOrientation gfxOrientationToUIInterfaceOrientation(uint orientation);

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
static uint defaultValidOrientationMask()
{
	return Base::isIPad ? UIInterfaceOrientationMaskAll : UIInterfaceOrientationMaskAllButUpsideDown;
}

uint Window::setValidOrientations(uint oMask, bool preferAnimated)
{
	validO = 0;
	if(oMask == VIEW_ROTATE_AUTO)
	{
		validO = defaultValidOrientationMask();
	}
	else
	{
		if(oMask & VIEW_ROTATE_0)
			validO |= UIInterfaceOrientationMaskPortrait;
		if(oMask & VIEW_ROTATE_90)
			validO |= UIInterfaceOrientationMaskLandscapeLeft;
		if(oMask & VIEW_ROTATE_180)
			validO |= UIInterfaceOrientationMaskPortraitUpsideDown;
		if(oMask & VIEW_ROTATE_270)
			validO |= UIInterfaceOrientationMaskLandscapeRight;
	}
	auto currO = [sharedApp statusBarOrientation];
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
	return 1;
}
#endif

Window *deviceWindow()
{
	return Window::window(0);
}

void Window::postDraw()
{
	setNeedsDraw(true);
	screen().postFrame();
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
}

uint GLConfigAttributes::defaultColorBits()
{
	return Config::BASE_IOS_GLKIT ? 24 : 16;
}

bool Window::hasSurface()
{
	return true;
}

IG::WindowRect Window::contentBounds() const
{
	return contentRect;
}

void IOSWindow::updateContentRect(int width, int height, uint rotateView, UIApplication *sharedApp_)
{
	using namespace Gfx;
	contentRect.x = contentRect.y = 0;
	contentRect.x2 = width;
	contentRect.y2 = height;
	bool hasStatusBar = deviceWindow()->uiWin_ == uiWin_ && !sharedApp.statusBarHidden;
	//logMsg("has status bar %d", hasStatusBar);
	if(hasStatusBar)
	{
		#ifdef CONFIG_GFX_SOFT_ORIENTATION
		bool isSideways = rotateView == VIEW_ROTATE_90 || rotateView == VIEW_ROTATE_270;
		auto statusBarHeight = (isSideways ? sharedApp.statusBarFrame.size.width : sharedApp.statusBarFrame.size.height) * pointScale;
		bool statusBarBeginsOnWindowOrigin = rotateView == VIEW_ROTATE_0 || rotateView == VIEW_ROTATE_270;
		if(statusBarBeginsOnWindowOrigin)
			contentRect.y = statusBarHeight;
		else
			contentRect.y2 -= statusBarHeight;
		#else
		auto statusBarO = [sharedApp statusBarOrientation];
		bool isSideways = statusBarO == UIInterfaceOrientationLandscapeLeft || statusBarO == UIInterfaceOrientationLandscapeRight;
		auto statusBarHeight = (isSideways ? sharedApp.statusBarFrame.size.width : sharedApp.statusBarFrame.size.height) * pointScale;
		contentRect.y = statusBarHeight;
		#endif
		logMsg("adjusted content rect to %d:%d:%d:%d for status bar height %d",
			contentRect.x, contentRect.y, contentRect.x2, contentRect.y2, (int)statusBarHeight);
	}
	else
	{
		logMsg("using full window size for content rect %d,%d", contentRect.x2, contentRect.y2);
	}
}

IG::Point2D<float> Window::pixelSizeAsMM(IG::Point2D<int> size)
{
	uint dpi = 163 * pointScale;
	#if !defined __ARM_ARCH_6K__
	if(isIPad)
	{
		// based on iPad DPI of 132 (264 retina)
		dpi = 132 * pointScale;
	}
	#endif
	return {(size.x / (float)dpi) * 25.4f, (size.y / (float)dpi) * 25.4f};
}

CallResult Window::init(const WindowConfig &config)
{
	if(uiWin_)
		return OK;
	initDelegates();
	if(!Config::BASE_MULTI_WINDOW && windows())
	{
		bug_exit("no multi-window support");
	}
	#ifdef CONFIG_BASE_MULTI_SCREEN
	screen_ = &config.screen();
	#endif

	#ifdef CONFIG_BASE_MULTI_WINDOW
	window_.push_back(this);
	if(window_.size() > 1)
	{
		logMsg("making additional window");
	}
	#else
	mainWin = this;
	#endif
	CGRect rect = screen().uiScreen().bounds;
	// Create a full-screen window
	uiWin_ = (void*)CFBridgingRetain([[UIWindow alloc] initWithFrame:rect]);
	#ifdef CONFIG_BASE_IOS_RETINA_SCALE
	pointScale = [screen().uiScreen() scale];
	if(pointScale > 1.)
		logMsg("using Retina scaling");
	#endif
	#ifndef CONFIG_GFX_SOFT_ORIENTATION
	validO = defaultValidOrientationMask();
	#endif
	updateWindowSizeAndContentRect(*this, rect.size.width * pointScale, rect.size.height * pointScale, sharedApp);
	
	// Create the OpenGL ES view and add it to the Window
	assert(GLContext::current());
	glView_ = (void*)CFBridgingRetain([[EAGLView alloc] initWithFrame:rect context:GLContext::current()->context()]);
	#ifdef CONFIG_BASE_IOS_GLKIT
	glView().enableSetNeedsDisplay = NO;
	#endif
	if(config.glConfig().useRGB565)
	{
		#ifdef CONFIG_BASE_IOS_GLKIT
		[glView() setDrawableColorFormat:GLKViewDrawableColorFormatRGB565];
		#else
		[glView() setDrawableColorFormat:kEAGLColorFormatRGB565];
		#endif
	}
	if(screen() == mainScreen())
	{
		glView().multipleTouchEnabled = YES;
		#ifdef CONFIG_INPUT_ICADE
		Input::iCade.init(glView());
		#endif
		#ifdef CONFIG_GFX_SOFT_ORIENTATION
		setAutoOrientation(1);
		#endif
	}
	else
	{
		uiWin().screen = screen().uiScreen();
	}
	//logMsg("setting root view controller");
	auto rootViewCtrl = [[ImagineUIViewController alloc] init];
	rootViewCtrl.wantsFullScreenLayout = YES; // for iOS < 7.0
	rootViewCtrl.view = glView();
	uiWin().rootViewController = rootViewCtrl;
	return OK;
}

void IOSWindow::deinit()
{
	assert(this != deviceWindow());
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(uiWin_)
	{
		logMsg("deinit window %p", uiWin_);
		if(GLContext::drawable() == this)
			GLContext::setDrawable(nullptr);
		CFRelease(glView_);
		glView_ = nullptr;
		CFRelease(uiWin_);
		uiWin_ = nullptr;
	}
	#endif
}

void Window::show()
{
	logMsg("showing window");
	if(this == deviceWindow())
		[uiWin() makeKeyAndVisible];
	else
		uiWin().hidden = NO;
}

Window *windowForUIWindow(UIWindow *uiWin)
{
	iterateTimes(Window::windows(), i)
	{
		auto w = Window::window(i);
		if(w->uiWin() == uiWin)
			return w;
	}
	return nullptr;
}

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

- (BOOL)shouldAutorotate
{
	//logMsg("reporting if should autorotate");
	if(self.view.window == Base::deviceWindow()->uiWin())
		return YES;
	else
		return NO;
}

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
