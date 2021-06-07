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
#include <imagine/base/Application.hh>
#include <imagine/input/Input.hh>
#include <imagine/input/TextField.hh>
#include <imagine/input/Device.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>
#include "../../input/apple/AppleGameDevice.hh"
#include "ios.hh"

@interface UIEvent ()
- (NSInteger*)_gsEvent;
@end

@interface IGAppTextField : NSObject <UITextFieldDelegate>
@property (nonatomic, retain) UITextField *uiTextField;
@property (nonatomic) Input::TextFieldDelegate textDelegate;
-(id)initWithTextField:(UITextField*)field textDelegate:(Input::TextFieldDelegate)del;
@end

@implementation IGAppTextField

@synthesize uiTextField;
@synthesize textDelegate;

-(id)initWithTextField:(UITextField*)field textDelegate:(Input::TextFieldDelegate)del
{
	self = [super init];
	uiTextField = field;
	textDelegate = del;
	return self;
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
	logMsg("pushed return");
	[textField resignFirstResponder];
	return YES;
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
	logMsg("editing ended");
	auto delegate = std::exchange(textDelegate, {});
	char text[256];
	string_copy(text, [textField.text UTF8String]);
	[textField removeFromSuperview];
	uiTextField = nil;
	if(delegate)
	{
		logMsg("running text entry callback");
		delegate(text);
	}
}

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

	KeyboardDevice(): Device{0, Map::SYSTEM,
		Device::TYPE_BIT_VIRTUAL | Device::TYPE_BIT_KEYBOARD | Device::TYPE_BIT_KEY_MISC, "Keyboard/iCade"}
	{}

	void setICadeMode(bool on) final
	{
		logMsg("set iCade mode %s", on ? "on" : "off");
		iCadeMode_ = on;
	}

	bool iCadeMode() const final
	{
		return iCadeMode_;
	}
};

static KeyboardDevice keyDev;
static bool hardwareKBAttached = false;

using GSEventIsHardwareKeyboardAttachedProto = BOOL(*)();
static GSEventIsHardwareKeyboardAttachedProto GSEventIsHardwareKeyboardAttached{};

static CGAffineTransform makeTransformForOrientation(Base::Orientation orientation)
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

static void setupTextView(Base::ApplicationContext ctx, UITextField *vkbdField, NSString *text)
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
	//[ vkbdField setEnabled: YES ];
	if(!Config::SYSTEM_ROTATES_WINDOWS)
		vkbdField.transform = makeTransformForOrientation(ctx.deviceWindow()->softOrientation());
	logMsg("init vkeyboard");
}

UIKitTextField::UIKitTextField(Base::ApplicationContext ctx, TextFieldDelegate del, const char *initialText, const char *promptText, int fontSizePixels):
	ctx{ctx}
{
	auto uiTextField = [[UITextField alloc] initWithFrame: toCGRect(*ctx.deviceWindow(), textRect)];
	setupTextView(ctx, uiTextField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding]);
	auto appTextField = [[IGAppTextField alloc] initWithTextField:uiTextField textDelegate:del];
	uiTextField.delegate = appTextField;
	textField_ = (void*)CFBridgingRetain(appTextField);
	[ctx.deviceWindow()->uiWin().rootViewController.view addSubview: uiTextField];
	logMsg("starting system text input");
	[uiTextField becomeFirstResponder];
}

UIKitTextField::~UIKitTextField()
{
	textField().textDelegate = {};
	[textField().uiTextField resignFirstResponder];
	CFRelease(textField_);
}

void TextField::place(IG::WindowRect rect)
{
	textRect = rect;
	if(!textField().uiTextField)
		return;
	textField().uiTextField.frame = toCGRect(*ctx.deviceWindow(), textRect);
}

IG::WindowRect TextField::windowRect() const { return textRect; }

void TextField::cancel()
{
	if(!textField().uiTextField)
		return;
	logMsg("canceled system text input");
	textField().textDelegate = {};
	[textField().uiTextField resignFirstResponder];
}

void TextField::finish()
{
	if(!textField().uiTextField)
		return;
	logMsg("finished system text input");
	[textField().uiTextField resignFirstResponder];
}

bool Device::anyTypeBitsPresent(Base::ApplicationContext ctx, uint32_t typeBits)
{
	if((typeBits & TYPE_BIT_KEYBOARD) && hardwareKBAttached)
		return true;
	if(typeBits & TYPE_BIT_GAMEPAD)
	{
		// A gamepad is present if iCade mode is in use on the iCade device (always first device)
		// or the device list size is not 1 due to BTstack connections from other controllers
		auto &devList = ctx.application().systemInputDevices();
		return (hardwareKBAttached && devList.front()->iCadeMode()) || devList.size() != 1;
	}
	return false;
}

void handleKeyEvent(Base::ApplicationContext ctx, UIEvent *event)
{
	const auto *eventMem = [event _gsEvent];
	if(!eventMem)
		return;
	auto eventType = eventMem[GSEVENT_TYPE];
	if(eventType != GSEVENT_TYPE_KEYDOWN && eventType != GSEVENT_TYPE_KEYUP)
		return;
	auto action = eventType == GSEVENT_TYPE_KEYDOWN ? Input::Action::PUSHED : Input::Action::RELEASED;
	Key key = eventMem[GSEVENTKEY_KEYCODE] & 0xFF; // only using key codes up to 255
	auto time = IG::FloatSeconds((double)[event timestamp]);
	auto &app = ctx.application();
	if(!keyDev.iCadeMode()
		|| (keyDev.iCadeMode() && !app.processICadeKey(key, action, time, keyDev, *ctx.deviceWindow())))
	{
		auto src = keyDev.iCadeMode() ? Input::Source::GAMEPAD : Input::Source::KEYBOARD;
		app.dispatchKeyInputEvent({0, Map::SYSTEM, key, key, action, 0, 0, src, time, &keyDev});
	}
}

Event::KeyString Event::keyString(Base::ApplicationContext) const
{
	return {}; // TODO
}

void init(Base::ApplicationContext ctx)
{
	ctx.application().addSystemInputDevice(keyDev);
	GSEventIsHardwareKeyboardAttached = (GSEventIsHardwareKeyboardAttachedProto)dlsym(RTLD_DEFAULT, "GSEventIsHardwareKeyboardAttached");
	if(GSEventIsHardwareKeyboardAttached)
	{
		hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
		if(hardwareKBAttached)
			logMsg("hardware keyboard present");
		CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), &ctx.application(),
			[](CFNotificationCenterRef, void *observer, CFStringRef, const void *, CFDictionaryRef)
			{
				hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
				logMsg("hardware keyboard %s", hardwareKBAttached ? "attached" : "detached");
				DeviceAction change{hardwareKBAttached ? DeviceAction::SHOWN : DeviceAction::HIDDEN};
				auto &app = *((Base::Application*)observer);
				app.dispatchInputDeviceChange(keyDev, change);
			},
			(__bridge CFStringRef)@"GSEventHardwareKeyboardAttached",
			nullptr, CFNotificationSuspensionBehaviorCoalesce);
	}
	#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
	initAppleGameControllers(ctx);
	#endif
}

}
