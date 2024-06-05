#pragma once

#include <xcb/xproto.h>
#include <xcb/xinput.h>

// EWMH (Extended Window Manager Hints) support
// http://freedesktop.org/wiki/Specifications/wm-spec

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

// from xcb_event.h
#define XCB_EVENT_RESPONSE_TYPE_MASK (0x7f)

constexpr std::array xcbErrorStrings
{
	"Success",
	"BadRequest",
	"BadValue",
	"BadWindow",
	"BadPixmap",
	"BadAtom",
	"BadCursor",
	"BadFont",
	"BadMatch",
	"BadDrawable",
	"BadAccess",
	"BadAlloc",
	"BadColor",
	"BadGC",
	"BadIDChoice",
	"BadName",
	"BadLength",
	"BadImplementation",
	"Unknown"
};

constexpr std::array xcbProtocolRequestCodes
{
	"Null",
	"CreateWindow",
	"ChangeWindowAttributes",
	"GetWindowAttributes",
	"DestroyWindow",
	"DestroySubwindows",
	"ChangeSaveSet",
	"ReparentWindow",
	"MapWindow",
	"MapSubwindows",
	"UnmapWindow",
	"UnmapSubwindows",
	"ConfigureWindow",
	"CirculateWindow",
	"GetGeometry",
	"QueryTree",
	"InternAtom",
	"GetAtomName",
	"ChangeProperty",
	"DeleteProperty",
	"GetProperty",
	"ListProperties",
	"SetSelectionOwner",
	"GetSelectionOwner",
	"ConvertSelection",
	"SendEvent",
	"GrabPointer",
	"UngrabPointer",
	"GrabButton",
	"UngrabButton",
	"ChangeActivePointerGrab",
	"GrabKeyboard",
	"UngrabKeyboard",
	"GrabKey",
	"UngrabKey",
	"AllowEvents",
	"GrabServer",
	"UngrabServer",
	"QueryPointer",
	"GetMotionEvents",
	"TranslateCoords",
	"WarpPointer",
	"SetInputFocus",
	"GetInputFocus",
	"QueryKeymap",
	"OpenFont",
	"CloseFont",
	"QueryFont",
	"QueryTextExtents",
	"ListFonts",
	"ListFontsWithInfo",
	"SetFontPath",
	"GetFontPath",
	"CreatePixmap",
	"FreePixmap",
	"CreateGC",
	"ChangeGC",
	"CopyGC",
	"SetDashes",
	"SetClipRectangles",
	"FreeGC",
	"ClearArea",
	"CopyArea",
	"CopyPlane",
	"PolyPoint",
	"PolyLine",
	"PolySegment",
	"PolyRectangle",
	"PolyArc",
	"FillPoly",
	"PolyFillRectangle",
	"PolyFillArc",
	"PutImage",
	"GetImage",
	"PolyText8",
	"PolyText16",
	"ImageText8",
	"ImageText16",
	"CreateColormap",
	"FreeColormap",
	"CopyColormapAndFree",
	"InstallColormap",
	"UninstallColormap",
	"ListInstalledColormaps",
	"AllocColor",
	"AllocNamedColor",
	"AllocColorCells",
	"AllocColorPlanes",
	"FreeColors",
	"StoreColors",
	"StoreNamedColor",
	"QueryColors",
	"LookupColor",
	"CreateCursor",
	"CreateGlyphCursor",
	"FreeCursor",
	"RecolorCursor",
	"QueryBestSize",
	"QueryExtension",
	"ListExtensions",
	"ChangeKeyboardMapping",
	"GetKeyboardMapping",
	"ChangeKeyboardControl",
	"GetKeyboardControl",
	"Bell",
	"ChangePointerControl",
	"GetPointerControl",
	"SetScreenSaver",
	"GetScreenSaver",
	"ChangeHosts",
	"ListHosts",
	"SetAccessControl",
	"SetCloseDownMode",
	"KillClient",
	"RotateProperties",
	"ForceScreenSaver",
	"SetPointerMapping",
	"GetPointerMapping",
	"SetModifierMapping",
	"GetModifierMapping",
	"Unknown"
};

constexpr auto xEventTypeToString(int type)
{
	switch(type)
	{
		case XCB_EXPOSE: return "Expose";
		case XCB_CONFIGURE_NOTIFY: return "ConfigureNotify";
		case XCB_BUTTON_PRESS: return "ButtonPress";
		case XCB_BUTTON_RELEASE: return "ButtonRelease";
		case XCB_MOTION_NOTIFY: return "MotionNotify";
		case XCB_KEY_PRESS: return "KeyPress";
		case XCB_KEY_RELEASE: return "KeyRelease";
		case XCB_CLIENT_MESSAGE: return "ClientMessage";
		case XCB_PROPERTY_NOTIFY: return "PropertyNotify";
		case XCB_SELECTION_NOTIFY: return "SelectionNotify";
		case XCB_MAP_NOTIFY: return "MapNotify";
		case XCB_REPARENT_NOTIFY: return "ReparentNotify";
		case XCB_DESTROY_NOTIFY: return "DestroyNotify";
		case XCB_GE_GENERIC: return "GenericEvent";
		default: return "Unknown";
	}
}

constexpr auto xIEventTypeToStr(int type)
{
	switch(type)
	{
		case XCB_INPUT_KEY_PRESS: return "XI_KeyPress";
		case XCB_INPUT_KEY_RELEASE: return "XI_KeyRelease";
		case XCB_INPUT_BUTTON_PRESS: return "XI_ButtonPress";
		case XCB_INPUT_BUTTON_RELEASE: return "XI_ButtonRelease";
		case XCB_INPUT_MOTION: return "XI_Motion";
		case XCB_INPUT_FOCUS_IN: return "XI_FocusIn";
		case XCB_INPUT_ENTER: return "XI_Enter";
		case XCB_INPUT_FOCUS_OUT: return "XI_FocusOut";
		case XCB_INPUT_LEAVE: return "XI_Leave";
		default: return "Unknown";
	}
}

struct MallocDeleter
{
	void operator()(void *p) const noexcept { return std::free(p); }
};

#define XCB_REPLY_CONNECTION_ARG(connection, ...) connection

#define XCB_REPLY(call, ...) \
	std::unique_ptr<call##_reply_t, MallocDeleter>( \
		call##_reply(XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call(__VA_ARGS__), nullptr) \
	)

#define XCB_REPLY_UNCHECKED(call, ...) \
	std::unique_ptr<call##_reply_t, MallocDeleter>( \
		call##_reply(XCB_REPLY_CONNECTION_ARG(__VA_ARGS__), call##_unchecked(__VA_ARGS__), nullptr) \
	)

inline xcb_atom_t internAtom(xcb_connection_t& conn, std::string_view name, bool onlyIfExists = false)
{
	auto reply = XCB_REPLY(xcb_intern_atom, &conn, onlyIfExists, name.size(), name.data());
	if(!reply)
		return 0;
	return reply->atom;
}

inline auto internAtoms(xcb_connection_t& conn, const auto& names, bool onlyIfExists = false)
{
	constexpr size_t size = std::size(names);
	std::array<xcb_intern_atom_cookie_t, size> cookies;
	for(auto &&[n, c] : std::views::zip(names, cookies))
	{
		c = xcb_intern_atom(&conn, onlyIfExists, strlen(n), n);
	}
	std::array<xcb_atom_t, size> atoms;
	for(auto &&[a, c] : std::views::zip(atoms, cookies))
	{
		auto reply = xcb_intern_atom_reply(&conn, c, nullptr);
		a = reply ? reply->atom : 0;
		free(reply);
	}
	return atoms;
}

inline std::string_view atomNameString(xcb_get_atom_name_reply_t& reply)
{
	return {xcb_get_atom_name_name(&reply), size_t(xcb_get_atom_name_name_length(&reply))};
}

inline uint8_t xDrawableDepth(xcb_connection_t& conn, xcb_window_t window)
{
	auto reply = XCB_REPLY(xcb_get_geometry, &conn, window);
	if(!reply)
		return 0;
	return reply->depth;
}

static const xcb_visualtype_t* findVisualType(xcb_screen_t& screen, int depth, std::predicate<const xcb_visualtype_t&> auto &&func)
{
	auto depthIt = xcb_screen_allowed_depths_iterator(&screen);
	if(!depthIt.data)
		return {};
	for(; depthIt.rem; xcb_depth_next(&depthIt))
	{
		if(depth && depthIt.data->depth != depth)
			continue;
		for(auto visualIt = xcb_depth_visuals_iterator(depthIt.data);
			visualIt.rem; xcb_visualtype_next(&visualIt))
		{
			if(func(*visualIt.data))
				return visualIt.data;
		}
		if(depth)
			break;
	}
	return {};
}
