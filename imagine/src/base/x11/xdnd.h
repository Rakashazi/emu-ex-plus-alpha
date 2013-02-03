#pragma once
#include <util/bits.h>
#include <util/preprocessor/enum.h>

static int dndInit = 0;

enum
{
	// DNDStatus
	dndMsgStatusWindow = 0,	// target window (sender)
	dndMsgStatusFlags,	// flags returned by target
	dndMsgStatusPt,		// x,y of "no msg" rectangle (root window coords)
	dndMsgStatusArea,	// w,h of "no msg" rectangle
	dndMsgStatusAction,	// action accepted by target

	// DNDStatus flags
	dndMsgStatusAcceptDropFlag = 1,	// set if target will accept the drop
	dndMsgStatusSendHereFlag   = 2,	// set if target wants stream of XdndPosition
	
	// DNDDrop
	dndMsgDropWindow = 0,	// source window (sender)
	dndMsgDropFlags,	// reserved
	dndMsgDropTimeStamp,	// timestamp for requesting data
	
	// DNDFinished
	dndMsgFinishedWindow = 0,	// target window (sender)
	dndMsgFinishedFlags,		// reserved
	dndMsgFinishedAction
};

// TODO: fix extra string added to end
ENUM(xdndAtomStr, 11,
     (
      XdndAware,
      XdndEnter,
      XdndPosition,
      XdndDrop,
      XdndStatus,
      XdndActionCopy,
      XdndActionMove,
      XdndActionPrivate,
      XdndSelection,
      XdndFinished,
      iSelectionProperty
     ),
     static char xdndAtomStr[][sizeof("iSelectionProperty")]);

static Atom xdndAtom[sizeofArray(xdndAtomStr)];
static const Atom currentDNDVersion = 5;

static void registerXdndAtoms(Display *dpy)
{
	uint atoms = sizeofArray(xdndAtomStr);
	char *strPtr[atoms];
	iterateTimes(atoms, i)
	{
		strPtr[i] = xdndAtomStr[i];
	}
	//logMsg("%d atoms, last %s", atoms, xdndAtomStr[atoms-1]);
	if(!XInternAtoms(dpy, strPtr, atoms, False, xdndAtom))
	{
		logWarn("XDND atoms could not be created, drag & drop may not function");
	}
}

static void enableXdnd(Display *dpy, X11Window win)
{
	if(!dndInit)
	{
		logMsg("setting up Xdnd");
		registerXdndAtoms(dpy);
		dndInit = 1;
	}
	logMsg("enabling Xdnd on window");
	XChangeProperty(dpy, win, xdndAtom[XdndAware], XA_ATOM, 32, PropModeReplace,
		(unsigned char*)&currentDNDVersion, 1);
}

static void disableXdnd(Display *dpy, X11Window win)
{
	logMsg("disabling Xdnd on window");
	XDeleteProperty(dpy, win, xdndAtom[XdndAware]);
}

static void sendDNDStatus(Display *dpy, X11Window win, X11Window srcWin, int willAcceptDrop, Atom action)
{
	XEvent xEvent = { 0 };
	xEvent.xclient.type = ClientMessage;
	xEvent.xclient.display = dpy;
	xEvent.xclient.window = srcWin;
	xEvent.xclient.message_type = xdndAtom[XdndStatus];
	xEvent.xclient.format = 32;

	xEvent.xclient.data.l[ dndMsgStatusWindow ] = win;
	xEvent.xclient.data.l[ dndMsgStatusFlags  ] = willAcceptDrop ? dndMsgStatusAcceptDropFlag : 0;
	xEvent.xclient.data.l[ dndMsgStatusPt     ] = 0;	// large dummy rectangle
	xEvent.xclient.data.l[ dndMsgStatusArea   ] = (100 << 16) | 100;
	xEvent.xclient.data.l[ dndMsgStatusAction ] = action;

	XSendEvent(dpy, srcWin, 0, 0, &xEvent);
	logMsg("sent xdnd status");
}

void sendDNDFinished(Display *dpy, X11Window win, X11Window srcWin, Atom action)
{
	if (srcWin != None)
	{
		XEvent xEvent = { 0 };
		xEvent.xclient.type = ClientMessage;
		xEvent.xclient.display = dpy;
		xEvent.xclient.window = srcWin;
		xEvent.xclient.message_type = xdndAtom[XdndFinished];
		xEvent.xclient.format = 32;

		xEvent.xclient.data.l[ dndMsgFinishedWindow ] = win;
		xEvent.xclient.data.l[ dndMsgFinishedFlags ]  = BIT(0);
		xEvent.xclient.data.l[ dndMsgFinishedAction ]  = action;

		XSendEvent(dpy, srcWin, 0, 0, &xEvent);
		logMsg("sent xdnd finished");
	}
}

static void receiveDrop(Display *dpy, X11Window win, X11Time_ time)
{
	XConvertSelection(dpy, xdndAtom[XdndSelection], XA_STRING, xdndAtom[iSelectionProperty], win, time);
}
