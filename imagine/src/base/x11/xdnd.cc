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

#include <imagine/base/Application.hh>
#include <imagine/util/bitset.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/utility.h>
#include "xdnd.hh"
#include <imagine/logger/logger.h>

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

enum
{
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
};

static constexpr Atom currentDNDVersion = 5;

namespace Base
{

using XdndAtoms = XApplication::XdndAtoms;

void XApplication::setXdnd(::Window win, bool on)
{
	if(!xdndIsInit())
	{
		logMsg("initializing Xdnd");
		if(!initXdnd())
			return;
	}
	if(on)
	{
		logMsg("enabling Xdnd on window:%lu", win);
		XChangeProperty(dpy, win, xdndAtom[XdndAware], XA_ATOM, 32, PropModeReplace,
			(unsigned char*)&currentDNDVersion, 1);
	}
	else
	{
		logMsg("disabling Xdnd on window:%lu", win);
		XDeleteProperty(dpy, win, xdndAtom[XdndAware]);
	}
}

bool XApplication::initXdnd()
{
	static constexpr int xdndAtomsSize = XdndAtoms().size();
	static_assert(iSelectionProperty == xdndAtomsSize - 1);
	char atomStr[xdndAtomsSize][sizeof("iSelectionProperty")]
	{
		"XdndAware",
		"XdndEnter",
		"XdndPosition",
		"XdndDrop",
		"XdndStatus",
		"XdndActionCopy",
		"XdndActionMove",
		"XdndActionPrivate",
		"XdndSelection",
		"XdndFinished",
		"iSelectionProperty",
	};
	char *atomStrPtr[xdndAtomsSize];
	std::copy_n(atomStr, xdndAtomsSize, atomStrPtr);
	if(!XInternAtoms(dpy, atomStrPtr, xdndAtomsSize, False, xdndAtom.data()))
	{
		logErr("error making XDND atoms");
		return false;
	}
	return true;
}

bool XApplication::xdndIsInit() const
{
	return xdndAtom[0];
}

void XApplication::sendDNDFinished(::Window win, ::Window srcWin, Atom action)
{
	if(srcWin == None)
		return;
	XEvent xEvent{};
	xEvent.xclient.type = ClientMessage;
	xEvent.xclient.display = dpy;
	xEvent.xclient.window = srcWin;
	xEvent.xclient.message_type = xdndAtom[XdndFinished];
	xEvent.xclient.format = 32;

	xEvent.xclient.data.l[ dndMsgFinishedWindow ] = win;
	xEvent.xclient.data.l[ dndMsgFinishedFlags ]  = IG::bit(0);
	xEvent.xclient.data.l[ dndMsgFinishedAction ]  = action;

	XSendEvent(dpy, srcWin, 0, 0, &xEvent);
	logMsg("sent xdnd finished");
}

static void sendDNDStatus(Display *dpy, XdndAtoms xdndAtom, ::Window win, ::Window srcWin, int willAcceptDrop, Atom action)
{
	XEvent xEvent{};
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

static void receiveDrop(Display *dpy, XdndAtoms xdndAtom, ::Window win, ::Time time)
{
	XConvertSelection(dpy, xdndAtom[XdndSelection], XA_STRING, xdndAtom[iSelectionProperty], win, time);
}

void handleXDNDEvent(Display *dpy, XdndAtoms xdndAtom, const XClientMessageEvent &message, ::Window win, ::Window &draggerWin, Atom &dragAction)
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
			sendDNDStatus(dpy, xdndAtom, win, message.data.l[dndMsgDropWindow], 1, dragAction);
		}
		else
		{
			char *action = XGetAtomName(dpy, message.data.l[4]);
			logMsg("rejecting drag & drop with unknown action %s", action);
			XFree(action);
			sendDNDStatus(dpy, xdndAtom, win, message.data.l[dndMsgDropWindow], 0, dragAction);
		}
	}
	else if(type == xdndAtom[XdndDrop])
	{
		receiveDrop(dpy, xdndAtom, win, message.data.l[dndMsgDropTimeStamp]);
	}
}

}
