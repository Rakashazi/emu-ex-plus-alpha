#import "MainApp.h"
#import "EAGLView.hh"
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#include <base/common/windowPrivate.hh>
#include <base/Base.hh>
#include <gfx/Gfx.hh>
#ifdef CONFIG_INPUT_ICADE
#include "ICadeHelper.hh"
#endif

@interface ImagineUIViewController : UIViewController

@end

@implementation ImagineUIViewController

- (BOOL)shouldAutorotate
{
	return NO;
}

@end

namespace Base
{

extern BOOL displayLinkActive;
extern EAGLContext *mainContext;
extern CADisplayLink *displayLink;
extern bool useMaxColorBits;
extern bool isIPad;
#ifdef CONFIG_INPUT_ICADE
extern ICadeHelper iCade;
#endif
#ifdef __ARM_ARCH_6K__
extern bool usingiOS4;
#else
static const bool usingiOS4 = 1; // always on iOS 4.3+ when compiled for ARMv7
#endif

void startAnimation();
void stopAnimation();
void updateSizeForStatusbar(Window &win, int width, int height, UIApplication *sharedApp);

static void initGLContext()
{
	mainContext = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES1];
	assert(mainContext);
	int ret = [EAGLContext setCurrentContext:mainContext];
	assert(ret);
	Gfx::init();
	//[mainContext retain];
}

void Window::displayNeedsUpdate()
{
	if(appState != APP_RUNNING)
	{
		logMsg("can't post window redraw when app isn't running");
		return;
	}
	drawPosted = true;
	if(Base::displayLinkActive == NO)
	{
		Base::startAnimation();
	}
}

void Window::unpostDraw()
{
	drawPosted = false;
	if(Base::displayLinkActive == NO)
	{
		Base::stopAnimation();
	}
}

void Window::setPixelBestColorHint(bool best)
{
	assert(!mainContext); // should only call before initial window is created
	useMaxColorBits = best;
}

bool Window::pixelBestColorHintDefault()
{
	return Config::MACHINE_IS_GENERIC_ARMV7;
}

void Window::swapBuffers()
{
	//logMsg("doing swap");
	//glBindRenderbufferOES(GL_RENDERBUFFER_OES, viewRenderbuffer);
	[mainContext presentRenderbuffer:GL_RENDERBUFFER_OES];
}

void Window::setVideoInterval(uint interval)
{
	logMsg("setting frame interval %d", (int)interval);
	assert(interval >= 1);
	[displayLink setFrameInterval:interval];
}

void Window::updateSize(int width, int height)
{
	w = width;
	h = height;
	viewRect = contentRect;
	viewPixelWidth_ = viewRect.xSize();
	viewPixelHeight_ = viewRect.ySize();
	calcPhysicalSize();
}

IG::Rect2<int> Window::untransformedViewBounds() const
{
	return contentRect;
}

void Window::calcPhysicalSize()
{
	uint dpi = 163 * pointScale;
	#if !defined(__ARM_ARCH_6K__) && (__IPHONE_OS_VERSION_MAX_ALLOWED >= 30200)
	if(isIPad)
	{
		// based on iPad DPI of 132 (264 retina)
		dpi = 132 * pointScale;
	}
	#endif
	viewMMWidth_ = std::round((viewPixelWidth_ / (float)dpi) * 25.4);
	viewMMHeight_ = std::round((viewPixelHeight_ / (float)dpi) * 25.4);
	logMsg("set screen MM size %fx%f", (double)viewMMWidth_, (double)viewMMHeight_);
}

CallResult Window::init(IG::Point2D<int> pos, IG::Point2D<int> size)
{
	if(mainWin)
	{
		bug_exit("created multiple windows");
	}
	if(!mainContext)
	{
		initGLContext();
	}
	CGRect rect = [[UIScreen mainScreen] bounds];
	mainWin = this;
	// Create a full-screen window
	uiWin = [[UIWindow alloc] initWithFrame:rect];

	#if !defined(__ARM_ARCH_6K__)
	logMsg("testing for Retina Display");
	if([UIScreen mainScreen].scale == 2.0)
	{
		logMsg("running on Retina Display");
		pointScale = 2;
		updateSizeForStatusbar(*this, rect.size.width * 2, rect.size.height * 2, [UIApplication sharedApplication]);
	}
	else
	#endif
	{
		updateSizeForStatusbar(*this, rect.size.width, rect.size.height, [UIApplication sharedApplication]);
	}
	Gfx::setViewport(*this);
	Gfx::setProjector(*this);
	
	// Create the OpenGL ES view and add it to the Window
	glView = [[EAGLView alloc] initWithFrame:rect
		#if !defined(__ARM_ARCH_6K__)
	  isRetina:pointScale == 2
		#endif
	];
	[glView createFramebuffer];
	#ifdef CONFIG_INPUT_ICADE
	iCade.init(glView);
	#endif
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	Base::setAutoOrientation(1);

	// view controller init
	if(usingiOS4)
	{
		UIViewController *viewCtrl = [[ImagineUIViewController alloc] init];
		viewCtrl.view = glView;
		[glView release];
		uiWin.rootViewController = viewCtrl;
		[viewCtrl release];
	}
	else
	{
		[uiWin addSubview:glView];
		[glView release];
	}
	[uiWin makeKeyAndVisible];
	onWindowInit(*this);
	return OK;
}

void Window::deinit() {}

void Window::show()
{
	displayNeedsUpdate();
}

}
