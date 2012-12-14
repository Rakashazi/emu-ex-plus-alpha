#pragma once

// EWMH (Extended Window Manager Hints) support
// http://freedesktop.org/wiki/Specifications/wm-spec

#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */

static void ewmhFullscreen(Display *dpy, X11Window win, int action)
{
	assert(action == _NET_WM_STATE_REMOVE || action == _NET_WM_STATE_ADD || action == _NET_WM_STATE_TOGGLE);

	XEvent xev = { 0 };
	xev.xclient.type = ClientMessage;
	xev.xclient.send_event = True;
	xev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xev.xclient.window = win;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = action;
	xev.xclient.data.l[1] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	if (!XSendEvent(dpy, DefaultRootWindow(dpy), False,
	     SubstructureRedirectMask | SubstructureNotifyMask, &xev))
	{
		logWarn("couldn't send root window NET_WM_STATE message");
	}
}

static const char *xEventTypeToString(int type)
{
	switch(type)
	{
		case Expose: return "Expose";
		case ConfigureNotify: return "ConfigureNotify";
		case ButtonPress: return "ButtonPress";
		case ButtonRelease: return "ButtonRelease";
		case MotionNotify: return "MotionNotify";
		case KeyPress: return "KeyPress";
		case KeyRelease: return "KeyRelease";
		case ClientMessage: return "ClientMessage";
		case PropertyNotify: return "PropertyNotify";
		case SelectionNotify: return "SelectionNotify";
		case MapNotify: return "MapNotify";
		case ReparentNotify: return "ReparentNotify";
		case DestroyNotify: return "DestroyNotify";
		case GenericEvent: return "GenericEvent";
		default: return "Unknown";
	}
}

static const char *xIEventTypeToStr(int type)
{
	switch(type)
	{
		case XI_KeyPress: return "XI_KeyPress";
		case XI_KeyRelease: return "XI_KeyRelease";
		case XI_ButtonPress: return "XI_ButtonPress";
		case XI_ButtonRelease: return "XI_ButtonRelease";
		case XI_Motion: return "XI_Motion";
		case XI_FocusIn: return "XI_FocusIn";
		case XI_Enter: return "XI_Enter";
		case XI_FocusOut: return "XI_FocusOut";
		case XI_Leave: return "XI_Leave";
		default: return "Unknown";
	}
}

// same as XGetGeometry but can pass null args for unwanted values
static Status safeXGetGeometry(Display *display, Drawable d, X11Window *root_return, int *x_return, int *y_return,
		unsigned int *width_return, unsigned int *height_return, unsigned int *border_width_return, unsigned int *depth_return)
{
	uint borderDummy;
	X11Window rootDummy;
	int xDummy, yDummy;
	uint widthDummy, heightDummy;
	uint depthDummy;

	return XGetGeometry(display, d, root_return ? root_return : &rootDummy,
		x_return ? x_return : &xDummy, y_return ? y_return : &yDummy,
		width_return ? width_return : &widthDummy, height_return ? height_return : &heightDummy,
		border_width_return ? border_width_return : &borderDummy, depth_return ? depth_return : &depthDummy);
}

static uint xDrawableDepth(Display *dpy, Drawable d)
{
	uint depth;
	safeXGetGeometry(dpy, d, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &depth);
	return depth;
}
