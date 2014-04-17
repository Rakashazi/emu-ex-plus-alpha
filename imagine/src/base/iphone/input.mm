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

#include <imagine/input/Input.hh>
#include <imagine/input/DragPointer.hh>
#import "MainApp.hh"
#include "../../input/private.hh"
#include "ios.hh"

namespace Input
{

#ifdef CONFIG_INPUT_ICADE
struct ICadeDevice : public Device
{
	bool iCadeMode_ = false;

	ICadeDevice(): Device{0, Event::MAP_ICADE, Device::TYPE_BIT_KEY_MISC, "iCade Controller"}
	{}

	void setICadeMode(bool on) override
	{
		logMsg("set iCade mode %s", on ? "on" : "off");
		iCadeMode_ = on;
		iCade.setActive(on);
	}

	bool iCadeMode() const override
	{
		return iCadeMode_;
	}
};
#endif

static ICadeDevice icadeDev;

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
	if(win.rotateView == VIEW_ROTATE_90 || win.rotateView == VIEW_ROTATE_270)
	{
		std::swap(x, y);
	}
	if(win.rotateView == VIEW_ROTATE_90)
	{
		x = (win.height() - x) - rect.ySize();
	}
	int x2 = rect.xSize(), y2 = rect.ySize();
	if(win.rotateView == VIEW_ROTATE_90 || win.rotateView == VIEW_ROTATE_270)
		std::swap(x2, y2);
	logMsg("made CGRect %d,%d size %d,%d", x / win.pointScale, y / win.pointScale,
			x2 / win.pointScale, y2 / win.pointScale);
	return CGRectMake(x / win.pointScale, y / win.pointScale, x2 / win.pointScale, y2 / win.pointScale);
}

static void setupTextView(UITextField *vkbdField, NSString *text)
{
	// init input text field
	using namespace Input;

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
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	vkbdField.transform = makeTransformForOrientation(Base::mainWindow().rotateView);
	#endif
	logMsg("init vkeyboard");
}

uint startSysTextInput(InputTextDelegate callback, const char *initialText, const char *promptText, uint fontSizePixels)
{
	logMsg("starting system text input");
	vKeyboardTextDelegate = callback;
	if(!vkbdField)
	{
		vkbdField = [ [ UITextField alloc ] initWithFrame: toCGRect(Base::mainWindow(), textRect) ];
		setupTextView(vkbdField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
		[Base::mainWindow().glView() addSubview: vkbdField];
	}
	else
	{
		vkbdField.frame = toCGRect(Base::mainWindow(), textRect);
		setupTextView(vkbdField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
	}

	[vkbdField becomeFirstResponder];
	return 0;
}

void placeSysTextInput(const IG::WindowRect &rect)
{
	textRect = rect;
	if(vkbdField)
	{
		vkbdField.frame = toCGRect(Base::mainWindow(), textRect);
		/*#ifdef CONFIG_GFX_SOFT_ORIENTATION
		vkbdField.transform = makeTransformForOrientation(Base::mainWindow().rotateView);
		#endif*/
	}
}

const IG::WindowRect &sysTextInputRect() { return textRect; }

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
	if(typeBits & TYPE_BIT_GAMEPAD)
	{
		#ifdef CONFIG_INPUT_ICADE
		// A gamepad is present if iCade mode is in use on the iCade device (always first device)
		// or the device list size is not 1 due to BTstack connections from other controllers
		return devList.front()->iCadeMode() || devList.size() != 1;
		#else
		return devList.size();
		#endif
	}
	// no other device types supported
	return false;
}

void setKeyRepeat(bool on)
{
	setAllowKeyRepeats(on);
}

CallResult init()
{
	#if defined CONFIG_INPUT_ICADE
	addDevice(icadeDev);
	#endif
	return OK;
}

}
