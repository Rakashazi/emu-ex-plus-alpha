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

#include <imagine/logger/logger.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include "xdnd.hh"
#include "xlibutils.h"
#include <imagine/util/ranges.hh>
#include <xcb/xfixes.h>

constexpr char ASCII_LF = 0xA;
constexpr char ASCII_CR = 0xD;

namespace IG
{

constexpr SystemLogger log{"X11"};

static void initXFixes(xcb_connection_t& conn);

XApplication::XApplication(ApplicationInitParams initParams):
	LinuxApplication{initParams},
	supportedFrameTimer{testFrameTimers()},
	xEventSrc{makeXDisplayConnection(initParams.eventLoop)}
{
	ApplicationContext appCtx{static_cast<Application&>(*this)};
	addScreen(appCtx, std::make_unique<Screen>(appCtx, Screen::InitParams{*xConn, *xScr}), false);
	initXFixes(*xConn);
	initInputSystem();
}

XApplication::~XApplication()
{
	deinitWindows();
	deinitInputSystem();
	log.info("closing X display");
	xcb_disconnect(xConn);
}

static std::array<char, 2> charToStringArr(char c)
{
	return {c, '\0'};
}

static int char_hexToInt(char c)
{
	int hex = -1;
	sscanf(charToStringArr(c).data(), "%x", &hex);
	return hex;
}

// TODO: move into generic header after testing
static void fileURLToPath(char *url)
{
	char *pathStart = url;
	//lookup the 3rd slash which will signify the root directory of the file system
	for(int i = 0; i < 3; i++)
	{
		pathStart = strchr(pathStart + 1, '/');
	}
	assert(pathStart);

	// strip trailing new line junk at the end, needed for Nautilus
	char *pathEnd = &pathStart[strlen(pathStart)-1];
	assert(pathEnd >= pathStart);
	for(; *pathEnd == ASCII_LF || *pathEnd == ASCII_CR || *pathEnd == ' '; pathEnd--)
	{
		*pathEnd = '\0';
	}

	// copy the path over the old string
	size_t destPos = 0;
	for(size_t srcPos = 0; pathStart[srcPos] != '\0'; ({srcPos++; destPos++;}))
	{
		if(pathStart[srcPos] != '%') // plain copy case
		{
			url[destPos] = pathStart[srcPos];
		}
		else // decode the next 2 chars as hex digits
		{
			srcPos++;
			int msd = char_hexToInt(pathStart[srcPos]) << 4;
			srcPos++;
			int lsd = char_hexToInt(pathStart[srcPos]);
			url[destPos] = msd + lsd;
		}
	}
	url[destPos] = '\0';
}

Window *XApplication::windowForXWindow(xcb_window_t xWin) const
{
	for(auto &w : windows())
	{
		if(w->nativeObject() == xWin)
			return w.get();
	}
	return nullptr;
}

static bool shouldBypassCompositor(const Window& win)
{
	return win.size() == win.screen()->sizePx();
}

bool XApplication::eventHandler(xcb_ge_generic_event_t& event_)
{
	union XcbEvents
	{
		xcb_ge_generic_event_t generic;
		xcb_expose_event_t xexpose;
		xcb_configure_notify_event_t xconfigure;
		xcb_client_message_event_t xclient;
		xcb_selection_notify_event_t xselection;
	};

	auto &event = reinterpret_cast<XcbEvents&>(event_);
	//log.info("got event type {} ({})", xEventTypeToString(event.type), event.type);
	switch(event.generic.response_type & XCB_EVENT_RESPONSE_TYPE_MASK)
	{
		case XCB_EXPOSE:
		{
			auto &win = *windowForXWindow(event.xexpose.window);
			if (event.xexpose.count == 0)
				win.postDraw();
			break;
		}
		case XCB_CONFIGURE_NOTIFY:
		{
			//log.info("ConfigureNotify");
			auto &win = *windowForXWindow(event.xconfigure.window);
			if(event.xconfigure.width == win.width() && event.xconfigure.height == win.height())
				break;
			win.updateSize({event.xconfigure.width, event.xconfigure.height});
			if(auto newShouldBypassCompositorState = shouldBypassCompositor(win);
				win.shouldBypassCompositorState != newShouldBypassCompositorState)
			{
				// allow bypassing compositor on WMs without full screen unredirect support like KWin, needed for VRR
				log.info("setting bypass compositor hint:{}", newShouldBypassCompositorState);
				win.shouldBypassCompositorState = newShouldBypassCompositorState;
				auto wmBypassCompositor = internAtom(*xConn, "_NET_WM_BYPASS_COMPOSITOR");
				int32_t val = newShouldBypassCompositorState ? 1 : 0;
				xcb_change_property(xConn, XCB_PROP_MODE_REPLACE, win.nativeObject(), wmBypassCompositor, XCB_ATOM_CARDINAL, 32, 1, &val);
				// work around mutter 44 issue that prevents full-screen redirection when decorations are present
				win.setDecorations(!newShouldBypassCompositorState);
			}
			break;
		}
		case XCB_CLIENT_MESSAGE:
		{
			auto &win = *windowForXWindow(event.xclient.window);
			auto type = event.xclient.type;
			auto atomNameReply = XCB_REPLY(xcb_get_atom_name, xConn, type);
			auto clientMsgName = atomNameString(*atomNameReply);
			//log.debug("got client msg {}", clientMsgName);
			if(clientMsgName == "WM_PROTOCOLS")
			{
				if(event.xclient.data.data32[0] == internAtom(*xConn, "WM_DELETE_WINDOW", true))
				{
					//log.info("got window manager delete window message");
					win.dispatchDismissRequest();
				}
				else
				{
					log.info("unknown WM_PROTOCOLS message");
				}
			}
			else if(Config::XDND && xdndIsInit())
			{
				auto [draggerXWin, dragAction] = win.xdndData();
				handleXDNDEvent(*xConn, xdndAtom, event.xclient, win.nativeObject(), draggerXWin, dragAction);
			}
			break;
		}
		case XCB_PROPERTY_NOTIFY:
		{
			//log.info("PropertyNotify");
			break;
		}
		case XCB_SELECTION_NOTIFY:
		{
			log.info("SelectionNotify");
			if(Config::XDND && event.xselection.property)
			{
				auto &win = *windowForXWindow(event.xselection.requestor);
				auto propReply = XCB_REPLY(xcb_get_property, xConn, false, win.nativeObject(), event.xselection.property, XCB_ATOM_ANY, 0, 256);
				if(!propReply)
					break;
				auto filename = (char*)xcb_get_property_value(propReply.get());
				auto numItems = xcb_get_property_value_length(propReply.get());
				log.info("property read {} items, in {} format, {} bytes left", numItems, propReply->format, propReply->bytes_after);
				log.info("property is {}", filename);
				auto [draggerXWin, dragAction] = win.xdndData();
				sendDNDFinished(win.nativeObject(), draggerXWin, dragAction);
				fileURLToPath(filename);
				win.dispatchDragDrop(filename);
			}
			break;
		}
		case XCB_MAP_NOTIFY:
		{
			//log.debug("MapNotfiy");
			break;
		}
		case XCB_REPARENT_NOTIFY:
		{
			//log.debug("ReparentNotify");
			break;
		}
		case XCB_GE_GENERIC:
		{
			handleXI2GenericEvent(event.generic); break;
		}
		default:
		{
			log.debug("got unhandled message type {}", event.generic.response_type);
		}
		break;
	}

	return true;
}


void printXcbError(const xcb_generic_error_t& error)
{
	size_t errorCode = std::min(size_t(error.error_code), xcbErrorStrings.size() - 1);
	size_t majorCode = std::min(size_t(error.major_code), xcbProtocolRequestCodes.size() - 1);
	log.error("X Error: {} ({}), sequence:{}, resource id:{}, major code:{} ({}), minor code:{}",
		error.error_code, xcbErrorStrings[errorCode], error.sequence, error.resource_id,
		error.major_code, xcbProtocolRequestCodes[majorCode], error.minor_code);
}

void XApplication::runX11Events(xcb_connection_t& conn)
{
	xcb_generic_event_t* event;
	while((event = xcb_poll_for_event(&conn)))
	{
		if(!(event->response_type & XCB_EVENT_RESPONSE_TYPE_MASK)) [[unlikely]]
		{
			printXcbError(reinterpret_cast<xcb_generic_error_t&>(*event));
		}
		else
		{
			eventHandler(reinterpret_cast<xcb_ge_generic_event_t&>(*event));
		}
		free(event);
	}
}

void XApplication::runX11Events()
{
	runX11Events(*xConn);
}

void XApplication::setWindowCursor(xcb_window_t xWin, bool on)
{
	uint32_t mask = XCB_CW_CURSOR;
	xcb_change_window_attributes_value_list_t vals{};
	vals.cursor = on ? normalCursor : blankCursor;
	xcb_change_window_attributes_aux(xConn, xWin, mask, &vals);
}

static void initXFixes(xcb_connection_t& conn)
{
	const xcb_query_extension_reply_t *extReply = xcb_get_extension_data(&conn, &xcb_xfixes_id);
	if(!extReply || !extReply->present)
	{
		log.warn("XFixes extension not available");
		return;
	}
	auto verReply = XCB_REPLY(xcb_xfixes_query_version, &conn, 4, 0);
	if(!verReply || verReply->major_version < 4)
	{
		log.warn("required XFixes 4.x version not available, server supports {}.{}", verReply->major_version, verReply->minor_version);
	}
}

FDEventSource XApplication::makeXDisplayConnection(EventLoop loop)
{
	auto &conn = *xcb_connect(nullptr, nullptr);
	xConn = &conn;
	if(xcb_connection_has_error(&conn))
	{
		log.error("couldn't open X connection");
		return {};
	}
	auto &screen = *xcb_setup_roots_iterator(xcb_get_setup(&conn)).data;
	xScr = &screen;
	log.info("created X connection:{} screen:{}", (void*)&conn, (void*)&screen);
	FDEventSource x11Src
	{
		xcb_get_file_descriptor(&conn),
		{.debugLabel = "XServer", .eventLoop = loop},
		[this, &conn](int, int)
		{
			runX11Events(conn);
			return true;
		}
	};
	return x11Src;
}

}
