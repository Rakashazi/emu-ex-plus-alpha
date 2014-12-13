#pragma once

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import "MainApp.hh"
#include <imagine/input/Input.hh>

namespace Base
{
	extern UIApplication *sharedApp;
	extern MainApp *mainApp;
	extern BOOL displayLinkActive;
	extern CADisplayLink *displayLink;
	extern bool isIPad;
	Window *windowForUIWindow(UIWindow *uiWin);
	Window *deviceWindow();
	bool hasAtLeastIOS7();
	bool hasAtLeastIOS8();
}

namespace Input
{
	extern UITextField *vkbdField;
	extern Input::InputTextDelegate vKeyboardTextDelegate;
	extern IG::WindowRect textRect;
	static constexpr int GSEVENTKEY_KEYCODE_IOS7 = 17;
	static constexpr int GSEVENTKEY_KEYCODE_64_BIT = 13;
	extern int GSEVENTKEY_KEYCODE;
	void handleKeyEvent(UIEvent *event);
}
