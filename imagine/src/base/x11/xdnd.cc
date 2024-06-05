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
#include <imagine/util/bit.hh>
#include <imagine/util/algorithm.h>
#include <imagine/util/utility.h>
#include "xdnd.hh"
#include "xlibutils.h"
#include <ranges>
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

constexpr xcb_atom_t currentDNDVersion = 5;

namespace IG
{

constexpr SystemLogger log{"X11"};

using XdndAtoms = XApplication::XdndAtoms;

void XApplication::setXdnd(xcb_window_t win, bool on)
{
	if(!xdndIsInit())
	{
		log.info("initializing Xdnd");
		if(!initXdnd())
			return;
	}
	if(on)
	{
		log.info("enabling Xdnd on window:{}", win);
		xcb_change_property(xConn, XCB_PROP_MODE_REPLACE, win, xdndAtom[XdndAware], XCB_ATOM_ATOM, 32, 1, &currentDNDVersion);
	}
	else
	{
		log.info("disabling Xdnd on window:{}", win);
		xcb_delete_property(xConn, win, xdndAtom[XdndAware]);
	}
	xcb_flush(xConn);
}

bool XApplication::initXdnd()
{
	static constexpr int xdndAtomsSize = XdndAtoms().size();
	static_assert(iSelectionProperty == xdndAtomsSize - 1);
	xdndAtom = internAtoms(*xConn, std::array{
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
	});
	return true;
}

bool XApplication::xdndIsInit() const
{
	return xdndAtom[0];
}

void XApplication::sendDNDFinished(xcb_window_t win, xcb_window_t srcWin, xcb_atom_t action)
{
	if(!srcWin)
		return;
	xcb_client_message_event_t xev{};
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.window = srcWin;
	xev.type = xdndAtom[XdndFinished];
	xev.format = 32;
	xev.data.data32[ dndMsgFinishedWindow ] = win;
	xev.data.data32[ dndMsgFinishedFlags  ] = IG::bit(0);
	xev.data.data32[ dndMsgFinishedAction ] = action;
	xcb_send_event(xConn, false, srcWin, 0, (const char*)&xev);
	log.info("sent xdnd finished");
}

static void sendDNDStatus(xcb_connection_t& conn, XdndAtoms xdndAtom, xcb_window_t win, xcb_window_t srcWin, int willAcceptDrop, xcb_atom_t action)
{
	xcb_client_message_event_t xev{};
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.window = srcWin;
	xev.type = xdndAtom[XdndStatus];
	xev.format = 32;
	xev.data.data32[ dndMsgStatusWindow ] = win;
	xev.data.data32[ dndMsgStatusFlags  ] = willAcceptDrop ? dndMsgStatusAcceptDropFlag : 0;
	xev.data.data32[ dndMsgStatusPt     ] = 0;	// large dummy rectangle
	xev.data.data32[ dndMsgStatusArea   ] = (100 << 16) | 100;
	xev.data.data32[ dndMsgStatusAction ] = action;
	xcb_send_event(&conn, false, srcWin, 0, (const char*)&xev);
	log.info("sent xdnd status");
}

static void receiveDrop(xcb_connection_t& conn, XdndAtoms xdndAtom, xcb_window_t win, xcb_timestamp_t time)
{
	xcb_convert_selection(&conn, win,
		xdndAtom[XdndSelection], XCB_ATOM_STRING, xdndAtom[iSelectionProperty], time);
}

void handleXDNDEvent(xcb_connection_t& conn, XdndAtoms xdndAtom, const xcb_client_message_event_t& message,
	xcb_window_t win, xcb_window_t &draggerWin, xcb_atom_t &dragAction)
{
	auto type = message.type;
	if(type == xdndAtom[XdndEnter])
	{
		//sendDNDStatus(conn, xdndAtom, win, message.data.data32[dndMsgDropWindow]);
		draggerWin = message.data.data32[dndMsgDropWindow];
	}
	else if(type == xdndAtom[XdndPosition])
	{
		if((xcb_atom_t)message.data.data32[4] == xdndAtom[XdndActionCopy]
			|| (xcb_atom_t)message.data.data32[4] == xdndAtom[XdndActionMove])
		{
			dragAction = (xcb_atom_t)message.data.data32[4];
			sendDNDStatus(conn, xdndAtom, win, message.data.data32[dndMsgDropWindow], 1, dragAction);
		}
		else
		{
			if(Config::DEBUG_BUILD)
			{
				auto atomNameReply = XCB_REPLY(xcb_get_atom_name, &conn, message.data.data32[4]);
				if(atomNameReply)
					log.info("rejecting drag & drop with unknown action {}", atomNameString(*atomNameReply));
			}
			sendDNDStatus(conn, xdndAtom, win, message.data.data32[dndMsgDropWindow], 0, dragAction);
		}
	}
	else if(type == xdndAtom[XdndDrop])
	{
		receiveDrop(conn, xdndAtom, win, message.data.data32[dndMsgDropTimeStamp]);
	}
}

}
