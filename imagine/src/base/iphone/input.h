#pragma once
#include <input/interface.h>
#include <input/common/common.h>
#include <input/DragPointer.hh>

namespace Input
{

static UITouch *activeTouch[maxCursors] = { 0 };
uint numCursors = maxCursors;
static PointerState m[maxCursors] = { { 0, 0, 0 } };
static DragPointer dragStateArr[maxCursors];

DragPointer *dragState(int p)
{
	return &dragStateArr[p];
}

int cursorX(int p) { return m[p].x; }
int cursorY(int p) { return m[p].y; }
int cursorIsInView(int p) { return m[p].inWin; }

#if defined(IPHONE_VKEYBOARD)

uint input_getVKeyboardString(InputTextCallback callback, void *user, const char *initialText)
{
	vKeyboardTextCallback = callback;
	vKeyboardTextCallbackUserPtr = user;
	inVKeyboard = 1;
	vkbdField.text = [NSString stringWithCString:initialText encoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/];
	[ window addSubview: vkbdField ];
	[ vkbdField becomeFirstResponder ];
	return 0;
}

void input_finishKeyboardInput()
{
	[vkbdField removeFromSuperview];
	NSString *text = vkbdField.text;
	//logMsg("calling text callback");
	vKeyboardTextCallback(vKeyboardTextCallbackUserPtr, [text cStringUsingEncoding: NSUTF8StringEncoding /*NSASCIIStringEncoding*/]);
	vkbdField.text = @"";
	inVKeyboard = 0;
}

#endif

void setKeyRepeat(bool on)
{
	// TODO
}

CallResult init()
{
	return OK;
}

}
