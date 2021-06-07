#pragma once

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "MainApp.hh"
#include <imagine/input/Input.hh>

namespace Base
{
	class ApplicationContext;

	extern MainApp *mainApp;
	extern bool isIPad;
	Window *windowForUIWindow(ApplicationContext, UIWindow *);
	bool hasAtLeastIOS5();
	bool hasAtLeastIOS7();
	bool hasAtLeastIOS8();
}

namespace Input
{
	static constexpr int GSEVENTKEY_KEYCODE_IOS7 = 17;
	static constexpr int GSEVENTKEY_KEYCODE_64_BIT = 13;
	extern int GSEVENTKEY_KEYCODE;
	void handleKeyEvent(Base::ApplicationContext, UIEvent *);
}
