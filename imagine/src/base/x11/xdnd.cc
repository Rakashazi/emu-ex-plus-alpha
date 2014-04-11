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

#include <imagine/util/bits.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/preprocessor/enum.h>
#include "xdnd.hh"
#include <imagine/logger/logger.h>

using namespace IG;

bool dndInit = false;

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
static constexpr Atom currentDNDVersion = 5;

void registerXdndAtoms(Display *dpy)
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

void enableXdnd(Display *dpy, ::Window win)
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

void disableXdnd(Display *dpy, ::Window win)
{
	logMsg("disabling Xdnd on window");
	XDeleteProperty(dpy, win, xdndAtom[XdndAware]);
}

void sendDNDStatus(Display *dpy, ::Window win, ::Window srcWin, int willAcceptDrop, Atom action)
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

void sendDNDFinished(Display *dpy, ::Window win, ::Window srcWin, Atom action)
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
		xEvent.xclient.data.l[ dndMsgFinishedFlags ]  = bit(0);
		xEvent.xclient.data.l[ dndMsgFinishedAction ]  = action;

		XSendEvent(dpy, srcWin, 0, 0, &xEvent);
		logMsg("sent xdnd finished");
	}
}

void receiveDrop(Display *dpy, ::Window win, Time time)
{
	XConvertSelection(dpy, xdndAtom[XdndSelection], XA_STRING, xdndAtom[iSelectionProperty], win, time);
}

void handleXDNDEvent(Display *dpy, const XClientMessageEvent &message, ::Window win, ::Window &draggerWin, Atom &dragAction)
{
	auto type = message.message_type;
	if(type == xdndAtom[XdndEnter])
	{
		//sendDNDStatus(dpy, win, message->data.l[dndMsgDropWindow]);
		draggerWin = message.data.l[dndMsgDropWindow];
	}
	else if(type == xdndAtom[XdndPosition])
	{
		if((Atom)message.data.l[4] == xdndAtom[XdndActionCopy]
			|| (Atom)message.data.l[4] == xdndAtom[XdndActionMove])
		{
			dragAction = (Atom)message.data.l[4];
			sendDNDStatus(dpy, win, message.data.l[dndMsgDropWindow], 1, dragAction);
		}
		else
		{
			char *action = XGetAtomName(dpy, message.data.l[4]);
			logMsg("rejecting drag & drop with unknown action %s", action);
			XFree(action);
			sendDNDStatus(dpy, win, message.data.l[dndMsgDropWindow], 0, dragAction);
		}
	}
	else if(type == xdndAtom[XdndDrop])
	{
		receiveDrop(dpy, win, message.data.l[dndMsgDropTimeStamp]);
	}
}
