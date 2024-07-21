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

#define LOGTAG "Window"
#include <imagine/base/Screen.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Application.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include "xlibutils.h"
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>

namespace IG
{

constexpr SystemLogger log{"X11Win"};

void Window::setAcceptDnd(bool on)
{
	if(!Config::XDND)
		return;
	application().setXdnd(xWin, on);
}

void Window::setTitle(const char *name)
{
	xcb_change_property(xConn, XCB_PROP_MODE_REPLACE, xWin, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
		strlen(name), name);
	xcb_flush(xConn);
}

bool Window::hasSurface() const
{
	return true;
}

WindowRect Window::contentBounds() const
{
	return bounds();
}

Point2D<float> Window::pixelSizeAsMM(Point2D<int> size)
{
	auto &s = *screen();
	auto [xMM, yMM] = s.mmSize();
	assert(xMM);
	return {xMM * ((float)size.x/(float)s.width()), yMM * ((float)size.y/(float)s.height())};
}

static WindowRect makeWindowRectWithConfig(xcb_connection_t& conn, const WindowConfig &config, xcb_window_t rootWindow)
{
	WindowRect workAreaRect;
	{
		auto _NET_WORKAREA = internAtom(conn, "_NET_WORKAREA");
		auto reply = XCB_REPLY(xcb_get_property, &conn, false, rootWindow, _NET_WORKAREA, XCB_ATOM_CARDINAL, 0, 1024);
		if(reply && xcb_get_property_value_length(reply.get()) >= 16)
		{
			auto workArea = (int32_t*)xcb_get_property_value(reply.get());
			log.info("work area: {}:{}:{}:{}", workArea[0], workArea[1], workArea[2], workArea[3]);
			workAreaRect = {{workArea[0], workArea[1]}, {workArea[2], workArea[3]}};
		}
		else
		{
			log.warn("error getting desktop work area, using root window size");
			auto reply = XCB_REPLY(xcb_get_geometry, &conn, rootWindow);
			if(reply)
			{
				workAreaRect = {{}, {reply->width, reply->height}};
			}
		}
	}

	WindowRect winRect;

	// set window size
	if(config.isDefaultSize())
	{
		winRect.x2 = workAreaRect.xSize()/2;
		winRect.y2 = workAreaRect.ySize()/2;
	}
	else
	{
		winRect.x2 = config.size.x;
		winRect.y2 = config.size.y;
	}

	// reduce size to work area if too big
	if(winRect.xSize() > workAreaRect.xSize())
	{
		winRect.x2 = workAreaRect.xSize();
	}
	if(winRect.ySize() > workAreaRect.ySize())
	{
		winRect.y2 = workAreaRect.ySize();
	}
	assert(winRect.xSize() > 0);
	assert(winRect.ySize() > 0);

	// set window position
	if(config.isDefaultPosition())
	{
		// move to center of work area
		winRect.setPos(workAreaRect.pos(C2DO), C2DO);
	}
	else
	{
		winRect.setPos(config.position, LT2DO);
	}

	// crop right & bottom to work area if overflowing
	if(winRect.x2 > workAreaRect.x2)
	{
		winRect.x2 = workAreaRect.x2;
	}
	if(winRect.y2 > workAreaRect.y2)
	{
		winRect.y2 = workAreaRect.y2;
	}
	log.info("made window rect {}:{}:{}:{}", winRect.x, winRect.y, winRect.x2, winRect.y2);
	return winRect;
}

struct VisualConfig
{
	xcb_visualid_t visual{};
	int depth{};
};

static VisualConfig defaultVisualConfig(xcb_screen_t& screen)
{
	return {screen.root_visual, screen.root_depth};
}

static VisualConfig defaultTranslucentVisualConfig(xcb_screen_t& screen)
{
	VisualConfig conf = defaultVisualConfig(screen);
	findVisualType(screen, 32, [&](const xcb_visualtype_t& v)
	{
		conf = {v.visual_id, 32};
		return true;
	});
	return conf;
}

Window::Window(ApplicationContext ctx, WindowConfig config, InitDelegate):
	XWindow{ctx, config}
{
	screen_ = config.screen(ctx);
	auto &xScreen = ctx.application().xScreen();
	auto rootWindow = xScreen.root;
	auto &conn = ctx.application().xConnection();
	auto winRect = Config::MACHINE_IS_PANDORA ? WindowRect{{}, {800, 480}} :
		makeWindowRectWithConfig(conn, config, rootWindow);
	updateSize({winRect.xSize(), winRect.ySize()});
	{
		xcb_create_window_value_list_t attr{};
		uint32_t valueMask = XCB_CW_EVENT_MASK | XCB_CW_BORDER_PIXEL;
		attr.event_mask = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_PROPERTY_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;
		attr.border_pixel = 0;
		VisualConfig visualConf;
		if(config.nativeFormat)
		{
			visualConf.visual = config.nativeFormat;
		}
		else
		{
			if(config.translucent)
				visualConf = defaultTranslucentVisualConfig(xScreen);
			else
				visualConf = defaultVisualConfig(xScreen);
		}
		colormap = attr.colormap = xcb_generate_id(&conn);
		xcb_create_colormap(&conn, 0, colormap, rootWindow, visualConf.visual);
		valueMask |= XCB_CW_COLORMAP;
		xWin = xcb_generate_id(&conn);
		xcb_create_window_aux(&conn, visualConf.depth, xWin, rootWindow, winRect.x, winRect.y, width(), height(),
			0, XCB_WINDOW_CLASS_INPUT_OUTPUT, visualConf.visual, valueMask, &attr);
		if(!xWin)
		{
			log.error("error initializing window");
			return;
		}
	}
	if(Config::DEBUG_BUILD)
		log.info("made window with XID {}, drawable depth {}", (int)xWin, xDrawableDepth(conn, xWin));
	ctx.application().initPerWindowInputData(xWin);
	if(Config::MACHINE_IS_PANDORA)
	{
		auto wmState = internAtom(conn, "_NET_WM_STATE");
		auto wmFullscreen = internAtom(conn, "_NET_WM_STATE_FULLSCREEN");
		xcb_change_property(&conn, XCB_PROP_MODE_REPLACE, xWin, wmState, XCB_ATOM_ATOM, 32, 1, &wmFullscreen);
	}
	else
	{
		auto wmDelete = internAtom(conn, "WM_DELETE_WINDOW", true);
		auto wmProtocols = internAtom(conn, "WM_PROTOCOLS");
		xcb_change_property(&conn, XCB_PROP_MODE_REPLACE, xWin, wmProtocols, XCB_ATOM_ATOM, 32, 1, &wmDelete);
		if(config.minimumSize.x || config.minimumSize.y)
		{
			xcb_size_hints_t hints{};
			hints.x = winRect.x;
			hints.y = winRect.y;
			hints.min_width = config.minimumSize.x;
			hints.min_height = config.minimumSize.y;
			hints.flags = XCB_ICCCM_SIZE_HINT_P_POSITION | XCB_ICCCM_SIZE_HINT_P_MIN_SIZE;
			xcb_icccm_set_wm_size_hints(&conn, xWin, XCB_ATOM_WM_NORMAL_HINTS, &hints);
		}
	}
	xConn = &conn;
	if(config.title)
		setTitle(config.title);
	xcb_flush(&conn);
}

XWindow::~XWindow()
{
	if(xWin)
	{
		log.info("destroying window with ID:{}", (int)xWin);
		xcb_destroy_window(xConn, xWin);
	}
	if(colormap)
	{
		xcb_free_colormap(xConn, colormap);
	}
	xcb_flush(xConn);
}

void Window::show()
{
	assert(xWin);
	xcb_map_window(xConn, xWin);
	xcb_flush(xConn);
	postDraw();
}

NativeWindow Window::nativeObject() const
{
	return xWin;
}

void Window::setIntendedFrameRate(FrameRate rate)
{
	screen()->setFrameRate(rate);
}

void Window::setFormat(NativeWindowFormat) {}

void Window::setFormat(PixelFormat) {}

PixelFormat Window::pixelFormat() const
{
	auto xScreen = screen()->nativeObject();
	if(xScreen->root_depth == 16)
		return PixelFmtRGB565;
	return PixelFmtRGBA8888;
}

std::pair<uint32_t, uint32_t> XWindow::xdndData() const
{
	return {draggerXWin, dragAction};
}

bool Window::operator ==(Window const &rhs) const
{
	return xWin == rhs.xWin;
}

XWindow::operator bool() const
{
	return xWin != 0L;
}

void Window::setCursorVisible(bool on)
{
	if constexpr(Config::MACHINE_IS_PANDORA)
	{
		if(on)
			xcb_xfixes_show_cursor(xConn, xWin);
		else
			xcb_xfixes_hide_cursor(xConn, xWin);
		xcb_flush(xConn);
	}
	else
	{
		application().setWindowCursor(xWin, on);
	}
}

static void ewmhFullscreen(xcb_connection_t& conn, xcb_window_t win, int action)
{
	assert(action == _NET_WM_STATE_REMOVE || action == _NET_WM_STATE_ADD || action == _NET_WM_STATE_TOGGLE);
	xcb_client_message_event_t xev{};
	xev.response_type = XCB_CLIENT_MESSAGE;
	xev.window = win;
	xev.type = internAtom(conn, "_NET_WM_STATE");
	xev.format = 32;
	xev.data.data32[0] = action;
	xev.data.data32[1] = (long)internAtom(conn, "_NET_WM_STATE_FULLSCREEN");
	auto geomReply = XCB_REPLY(xcb_get_geometry, &conn, win);
	if(!geomReply)
		return;
	xcb_send_event(&conn, true, geomReply->root, XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY, (const char*)&xev);
	xcb_flush(&conn);
}

void Window::toggleFullScreen()
{
	log.info("toggling fullscreen");
	ewmhFullscreen(*xConn, xWin, _NET_WM_STATE_TOGGLE);
}

void WindowConfig::setFormat(PixelFormat) {}

struct MotifWMHints
{
	uint32_t flags{};
	uint32_t functions{};
	uint32_t decorations{};
	int32_t input_mode{};
	uint32_t status{};
};

void Window::setDecorations(bool on)
{
	log.info("setting window decorations:{}", on ? "on" : "off");
  auto wmHintsAtom = internAtom(*xConn, "_MOTIF_WM_HINTS");
  MotifWMHints hints{.flags = bit(1), .decorations = on};
	xcb_change_property(xConn, XCB_PROP_MODE_REPLACE, xWin, wmHintsAtom, wmHintsAtom, 32, 5, &hints);
	xcb_flush(xConn);
}

void Window::setPosition(WPt pos)
{
	xcb_configure_window_value_list_t vals{};
	vals.x = pos.x; vals.y = pos.y;
	xcb_configure_window_aux(xConn, xWin, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, &vals);
	xcb_flush(xConn);
}

void Window::setSize(WSize size)
{
	xcb_configure_window_value_list_t vals{};
	vals.width = size.x; vals.height = size.y;
	xcb_configure_window_aux(xConn, xWin, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, &vals);
	xcb_flush(xConn);
}

}
