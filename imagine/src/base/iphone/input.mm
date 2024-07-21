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
#include <imagine/input/Event.hh>
#include <imagine/input/TextField.hh>
#include <imagine/input/Device.hh>
#include <imagine/time/Time.hh>
#include <imagine/logger/logger.h>
#include "ios.hh"

@interface UIEvent ()
- (NSInteger*)_gsEvent;
@end

@interface IGAppTextField : NSObject <UITextFieldDelegate>
@property (nonatomic, retain) UITextField *uiTextField;
@property (nonatomic) IG::TextFieldDelegate textDelegate;
-(id)initWithTextField:(UITextField*)field textDelegate:(IG::TextFieldDelegate)del;
@end

@implementation IGAppTextField

@synthesize uiTextField;
@synthesize textDelegate;

-(id)initWithTextField:(UITextField*)field textDelegate:(IG::TextFieldDelegate)del
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
	std::string text{[textField.text UTF8String]};
	[textField removeFromSuperview];
	uiTextField = nil;
	if(delegate)
	{
		logMsg("running text entry callback");
		delegate(text.data());
	}
}

@end

namespace IG::Input
{

constexpr inline int GSEVENT_TYPE = 2;
constexpr inline int GSEVENT_FLAGS = 12;
constexpr inline int GSEVENT_TYPE_KEYDOWN = 10;
constexpr inline int GSEVENT_TYPE_KEYUP = 11;

static Input::Device *keyDevPtr;
static bool hardwareKBAttached = false;

using GSEventIsHardwareKeyboardAttachedProto = BOOL(*)();
static GSEventIsHardwareKeyboardAttachedProto GSEventIsHardwareKeyboardAttached{};

static CGAffineTransform makeTransformForOrientation(Rotation orientation)
{
	switch(orientation)
	{
		default: return CGAffineTransformIdentity;
		case Rotation::LEFT: return CGAffineTransformMakeRotation(3 * M_PI / 2.0);
		case Rotation::RIGHT: return CGAffineTransformMakeRotation(M_PI / 2.0);
		case Rotation::DOWN: return CGAffineTransformMakeRotation(M_PI);
	}
}

static CGRect toCGRect(const Window &win, const IG::WindowRect &rect)
{
	int x = rect.x, y = rect.y;
	if(win.softOrientation() == Rotation::RIGHT || win.softOrientation() == Rotation::LEFT)
	{
		std::swap(x, y);
	}
	if(win.softOrientation() == Rotation::RIGHT)
	{
		x = (win.height() - x) - rect.ySize();
	}
	int x2 = rect.xSize(), y2 = rect.ySize();
	if(win.softOrientation() == Rotation::RIGHT || win.softOrientation() == Rotation::LEFT)
		std::swap(x2, y2);
	logMsg("made CGRect %f,%f size %f,%f", x / win.pointScale, y / win.pointScale,
			x2 / win.pointScale, y2 / win.pointScale);
	return CGRectMake(x / win.pointScale, y / win.pointScale, x2 / win.pointScale, y2 / win.pointScale);
}

static void setupTextView(ApplicationContext ctx, UITextField *vkbdField, NSString *text)
{
	// init input text field

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

UIKitTextField::UIKitTextField(ApplicationContext ctx, TextFieldDelegate del, CStringView initialText,
	[[maybe_unused]] CStringView promptText, [[maybe_unused]] int fontSizePixels):
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

bool Device::anyTypeFlagsPresent(ApplicationContext ctx, DeviceTypeFlags typeFlags)
{
	if((typeFlags.keyboard) && hardwareKBAttached)
		return true;
	if(typeFlags.gamepad)
	{
		// A gamepad is present if iCade mode is in use on the iCade device (always first device)
		// or the device list size is not 1 due to BTstack connections from other controllers
		auto &devList = ctx.application().inputDevices();
		return (hardwareKBAttached && devList.front()->iCadeMode()) || devList.size() != 1;
	}
	return false;
}

void handleKeyEvent(ApplicationContext ctx, UIEvent *event)
{
	const auto *eventMem = [event _gsEvent];
	if(!eventMem)
		return;
	auto eventType = eventMem[GSEVENT_TYPE];
	if(eventType != GSEVENT_TYPE_KEYDOWN && eventType != GSEVENT_TYPE_KEYUP)
		return;
	auto action = eventType == GSEVENT_TYPE_KEYDOWN ? Input::Action::PUSHED : Input::Action::RELEASED;
	Key key = eventMem[GSEVENTKEY_KEYCODE] & 0xFF; // only using key codes up to 255
	auto time = fromSeconds<SteadyClockTime>([event timestamp]);
	auto &app = ctx.application();
	auto &keyDev = *keyDevPtr;
	auto src = Input::Source::KEYBOARD;
	app.dispatchKeyInputEvent({Map::SYSTEM, key, action, 0, 0, src, SteadyClockTimePoint{time}, &keyDev});
}

std::string KeyEvent::keyString(ApplicationContext) const
{
	return {}; // TODO
}

void init(ApplicationContext ctx)
{
	keyDevPtr = &ctx.application().addInputDevice(ctx, std::make_unique<Input::Device>(std::in_place_type<Input::KeyboardDevice>));
	GSEventIsHardwareKeyboardAttached = (GSEventIsHardwareKeyboardAttachedProto)dlsym(RTLD_DEFAULT, "GSEventIsHardwareKeyboardAttached");
	if(GSEventIsHardwareKeyboardAttached)
	{
		hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
		if(hardwareKBAttached)
			logMsg("hardware keyboard present");
		CFNotificationCenterAddObserver(CFNotificationCenterGetDarwinNotifyCenter(), std::bit_cast<void*>(ctx),
			[](CFNotificationCenterRef, void *observer, CFStringRef, const void *, CFDictionaryRef)
			{
				hardwareKBAttached = GSEventIsHardwareKeyboardAttached();
				logMsg("hardware keyboard %s", hardwareKBAttached ? "attached" : "detached");
				auto change = hardwareKBAttached ? DeviceChange::shown : DeviceChange::hidden;
				auto ctx = std::bit_cast<ApplicationContext>(observer);
				ctx.application().dispatchInputDeviceChange(ctx, *keyDevPtr, change);
			},
			(__bridge CFStringRef)@"GSEventHardwareKeyboardAttached",
			nullptr, CFNotificationSuspensionBehaviorCoalesce);
	}
	#ifdef CONFIG_INPUT_APPLE_GAME_CONTROLLER
	initAppleGameControllers(ctx);
	#endif
}

}
