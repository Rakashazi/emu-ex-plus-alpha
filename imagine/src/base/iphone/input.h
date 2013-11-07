#pragma once
#include <input/Input.hh>
#include <input/common/common.h>
#include <input/DragPointer.hh>

namespace Input
{

static struct TouchState
{
	constexpr TouchState() {}
	UITouch *touch = nil;
	PointerState s;
	DragPointer dragState;
} m[maxCursors];
uint numCursors = maxCursors;

#ifdef CONFIG_INPUT_ICADE
struct ICadeDevice : public Device
{
	bool iCadeMode_ = false;

	ICadeDevice(): Device(0, Event::MAP_ICADE, Device::TYPE_BIT_KEY_MISC, "iCade Controller")
	{}

	void setICadeMode(bool on) override
	{
		logMsg("set iCade mode %s", on ? "on" : "off");
		iCadeMode_ = on;
		Base::iCade.setActive(on);
	}

	bool iCadeMode() const override
	{
		return iCadeMode_;
	}
};
#endif

static ICadeDevice icadeDev;

DragPointer *dragState(int p)
{
	return &m[p].dragState;
}

#if defined(IPHONE_VKEYBOARD)

static CGRect toCGRect(const Base::Window &win, const IG::Rect2<int> &rect)
{
	using namespace Base;
	int x = rect.x, y = rect.y;
	if(win.rotateView == VIEW_ROTATE_90 || win.rotateView == VIEW_ROTATE_270)
	{
		std::swap(x, y);
	}
	if(win.rotateView == VIEW_ROTATE_90)
	{
		x = (win.viewPixelHeight() - x) - rect.ySize();
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
	vkbdField.transform = makeTransformForOrientation(Base::mainWindow().rotateView);
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
		[ Base::mainWindow().uiWin addSubview: vkbdField ];
		[ vkbdField release ];
	}
	else
	{
		vkbdField.frame = toCGRect(Base::mainWindow(), textRect);
		setupTextView(vkbdField, [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
	}

	[ vkbdField becomeFirstResponder ];
	return 0;
}

void placeSysTextInput(const IG::Rect2<int> &rect)
{
	textRect = rect;
	if(vkbdField)
	{
		vkbdField.frame = toCGRect(Base::mainWindow(), textRect);
	}
}

const IG::Rect2<int> &sysTextInputRect() { return textRect; }

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
	// TODO
}

CallResult init()
{
	#if defined CONFIG_INPUT_ICADE
	addDevice(icadeDev);
	#endif
	return OK;
}

}
