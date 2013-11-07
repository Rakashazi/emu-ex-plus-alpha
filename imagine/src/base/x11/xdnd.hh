#pragma once

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define BOOL X11BOOL
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#undef Time
#undef Pixmap
#undef GC
#undef BOOL

extern bool dndInit;

void registerXdndAtoms(Display *dpy);
void enableXdnd(Display *dpy, ::Window win);
void disableXdnd(Display *dpy, ::Window win);
void sendDNDStatus(Display *dpy, ::Window win, ::Window srcWin, int willAcceptDrop, Atom action);
void sendDNDFinished(Display *dpy, ::Window win, ::Window srcWin, Atom action);
void receiveDrop(Display *dpy, ::Window win, X11Time_ time);
void handleXDNDEvent(Display *dpy, const XClientMessageEvent &e, ::Window win, ::Window &draggerWin, Atom &dragAction);
