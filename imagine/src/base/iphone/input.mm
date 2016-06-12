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
#define LOGTAG "Input"
#import <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <imagine/input/Input.hh>
#include <imagine/logger/logger.h>
#import "MainApp.hh"
#include "../../input/private.hh"
#include "../../input/apple/AppleGameDevice.hh"
#include "ios.hh"

@interface UIEvent ()
- (NSInteger*)_gsEvent;
@end

namespace Input
{

static constexpr int GSEVENT_TYPE = 2;
static constexpr int GSEVENT_FLAGS = 12;
static constexpr int GSEVENT_TYPE_KEYDOWN = 10;
static constexpr int GSEVENT_TYPE_KEYUP = 11;

static constexpr double MSEC_PER_SEC = 1000;

struct KeyboardDevice : public Device
{
	bool iCadeMode_ = false;

	KeyboardDevice(): Device{0, Event::MAP_SYSTEM,
		Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC,
		"Keyboard/iCade"}
	{}

	void setICadeMode(bool on) override
	{
		logMsg("set iCade mode %s", on ? "on" : "off");
		iCadeMode_ = on;
	}

	bool iCadeMode() const override
	{
		return iCadeMode_;
	}
};

static KeyboardDevice keyDev;
static bool hardwareKBAttached = false;

using GSEventIsHardwareKeyboardAttachedProto = BOOL(*)();
static GSEventIsHardwareKeyboardAttachedProto GSEventIsHardwareKeyboardAttached{};

#if defined IPHONE_VKEYBOARD

static CGAffineTransform makeTransformForOrientation(uint orientation)
{
	using namespace Base;
	switch(orientation)
	{
		default: return CGAffineTransformIdentity;
		case VIEW_ROTATE_270: return CGAffineTransformMakeRotation(3*M_PI / 2.0);
		case VIEW_ROTATE_90: return CGAffineTransformMakeRotation(M_PI / 2.0);
		case VIEW_ROTATE_180: return CGAffineTransformMakeRotation(M_PI);
	}
}

static CGRect toCGRect(const Base::Window &win, const IG::WindowRect &rect)
{
	using namespace Base;
	int x = rect.x, y = rect.y;
	if(win.softOrientation() == VIEW_ROTATE_90 || win.softOrientation() == VIEW_ROTATE_270)
	{
		std::swap(x, y);
	}
	if(win.softOrientation() == VIEW_ROTATE_90)
	{
		x = (win.height() - x) - rect.ySize();
	}
	int x2 = rect.xSize(), y2 = rect.ySize();
	if(win.softOrientation() == VIEW_ROTATE_90 || win.softOrientation() == VIEW_ROTATE_270)
		std::swap(x2, y2);
	logMsg("made CGRect %f,%f size %f,%f", x / win.pointScale, y / win.pointScale,
			x2 / win.pointScale, y2 / win.pointScale);
	return CGRectMake(x / win.pointScale, y / win.pointScale, x2 / win.pointScale, y2 / win.pointScale);
}

static void setupTextView(UITextField *vkbdField, NSString *text)
{
	// init input text field
	using namespace Base;

	/*vkbdField = [ [ UITextView alloc ] initWithFrame: CGRectMake(12, 24, 286, 24*4) ];
	vkbdField.backgroundColor = [UIColor whiteColor];
	vkbdField.autocorrectionType = UITextAutocorrectionTypeNo;
	vkbdField.autocapitalizationType = UITextAutocapitalizationTypeNone;
	vkbdField.keyboardType = UIKeyboardTypeASCIICapable; //UIKeyboardTypeDefault;
	vkbdField.enablesReturnKeyAutomatically = YES;*/

	//vkbdField = [ [ UITextField alloc ] initWithFrame: toCGRect(textRect) ];
	vkbdField.adjustsFontSizeToFitWidth = YES;
	//vkbdField.placeholder = @"";
	vkbdField.borderStyle = UITextBorderStyleRoundedRect;
	vkbdField.returnKeyType = UIReturnKeyDone;
	vkbdField.keyboardAppearance = UIKeyboardAppearanceAlert;
	vkbdField.clearButtonMode = UITextFieldViewModeNever;

	vkbdField.textColor = [UIColor blackColor];
	#ifdef __ARM_ARCH_6K__
	vkbdField.textAlignment = UITextAlignmentLeft; // deprecated on newer SDKs
	#else
	vkbdField.textAlignment = NSTextAlignmentLeft;
	#endif
	vkbdField.font = [UIFont systemFontOfSize:24.0];
	vkbdField.text = text;
	vkbdField.delegate = Base::mainApp;
	//[ vkbdField setEnabled: YES ];
	if(!Config::SYSTEM_ROTATES_WINDOWS)
		vkbdField.transform = makeTransformForOrientation(deviceWindow()->softOrientation());
	logMsg("init vkeyboard");
}

uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels)
{
	using namespace Base;
	logMsg("starting system text input");
	vKeyboardTextDelegate = callback;
	if(!vkbdField)
	{
		vkbdField = [[UITextField alloc] initWithFrame: toCGRect(*deviceWindow(), textRect)];
		setupTextView(vkbdField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
		[deviceWindow()->glView() addSubview: vkbdField];
	}
	else
	{
		vkbdField.frame = toCGRect(*deviceWindow(), textRect);
		setupTextView(vkbdField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
	}

	[vkbdField becomeFirstResponder];
	return 0;
}

void placeSysTextInput(IG::WindowRect rect)
{
	using namespace Base;
	textRect = rect;
	if(vkbdField)
	{
		vkbdField.frame = toCGRect(*deviceWindow(), textRect);
		/*#ifdef CONFIG_GFX_SOFT_ORIENTATION
		vkbdField.transform = makeTransformForOrientation(deviceWindow().softOrientation());
		#endif*/
	}
}

IG::WindowRect sysTextInputRect() { return textRect; }

void cancelSysTextInput()
{
	if(!vkbdField)
		return;
	logMsg("canceled system text input");
	vKeyboardTextDelegate = {};
	[vkbdField resignFirstResponder];
}

void finishSysTextInput()
{
	if(!vkbdField)
		return;
	logMsg("finished system text input");
	[vkbdField resignFirstResponder];
}

#endif

bool Device::anyTypeBitsPresent(uint typeBits)
{
	if((typeBits & TYPE_BIT_KEYBOARD) && hardwareKBAttached)
		return true;
	if(typeBits & TYPE_BIT_GAMEPAD)
	{
		// A gamepad is present if iCade mode is in use on the iCade device (always first device)
		// or the device list size is not 1 due to BTstack connections from other controllers
		return (hardwareKBAttached && devList.front()->iCadeMode()) || devList.size() != 1;
	}
	return false;
}

void setKeyRepeat(bool on)
{
	setAllowKeyRepeats(on);
}

void handleKeyEvent(UIEvent *event)
{
	const auto *eventMem = [event _gsEvent];
	if(!eventMem)
		return;
	auto eventType = eventMem[GSEVENT_TYPE];
	if(eventType != GSEVENT_TYPE_KEYDOWN && eventType != GSEVENT_TYPE_KEYUP)
		return;
	auto action = eventType == GSEVENT_TYPE_KEYDOWN ? Input::PUSHED : Input::RELEASED;
	Key key = eventMem[GSEVENTKEY_KEYCODE] & 0xFF; // only using key codes up to 255
	auto time = Time::makeWithSecsD((double)[event timestamp]);
	if(!keyDev.iCadeMode()
		|| (keyDev.iCadeMode() && !processICadeKey(key, action, time, keyDev, *Base::deviceWindow())))
	{
		Base::deviceWindow()->dispatchInputEvent({0, Event::MAP_SYSTEM, key, key, action, 0, time, &keyDev});
	}
}

Event::KeyString Event::keyString() const
{
	return {}; // TODO
}

Time TimeIOS::makeWithSecsD(double secs)
{
	Time time;
	time.t = secs;
	return time;
}

Time Time::makeWithNSecs(uint64_t nsecs)
{
	Time time;
	time.t = nsecs / (double)NSEC_PER_SEC;
	return time;
}

Time Time::makeWithUSecs(uint64_t usecs)
{
	return makeWithNSecs(usecs * NSEC_PER_USEC);
}

Time Time::makeWithMSecs(uint64_t msecs)
{
	return makeWithNSecs(msecs * NSEC_PER_MSEC);
}

Time Time::makeWithSecs(uint64_t secs)
{
	return makeWithNSecs(secs * NSEC_PER_SEC);
}

uint64_t Time::nSecs() const
{
	return t * (double)NSEC_PER_SEC;
}

uint64_t Time::uSecs() const
{
	return t * (double)USEC_PER_SEC;
}

uint64_t Time::mSecs() const
{
	return t * (double)MSEC_PER_SEC;
}

uint64_t Time::secs() const
{
	return t;
}

Time::operator IG::Time() const
{
	return IG::Time::makeWithNSecs(nSecs());
}

void setHandleVolumeKeys(bool on) {}

void showSoftInput() {}
void hideSoftInput() {}
bool softInputIsActive() { return false; }

CallResult init()
{
	addDevice(keyDev);
	GSEventIsHardwareKeyboardAttached = (GSEventIsHardwareKeyboardAttachedProto)dlsym(RTLD_DEFAULT, "GSEventIsHardwareKeyboardAttached");
	if(GSEventIsHardwareKeyboardAttached)
	{
		hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
		if(hardwareKBAttached)
			logMsg("hardware keyboard present");
		CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), nullptr,
			[](CFNotificationCenterRef, void *, CFStringRef, const void *, CFDictionaryRef)
			{
				hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
				logMsg("hardware keyboard %s", hardwareKBAttached ? "attached" : "detached");
				Device::Change change{hardwareKBAttached ? Device::Change::SHOWN : Device::Change::HIDDEN};
				onDeviceChange.callCopySafe(keyDev, change);
			},
			(__bridge CFStringRef)@"GSEventHardwareKeyboardAttached",
			nullptr, CFNotificationSuspensionBehaviorCoalesce);
	}
	#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
	initAppleGameControllers();
	#endif
	return OK;
}

}
