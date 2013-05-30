#define thisModuleName "base:x11"
#include <cstdlib>
#include <unistd.h>
#include <engine-globals.h>
#include <input/Input.hh>
#include <logger/interface.h>
#include <base/Base.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <util/fd-utils.h>
#include <util/string/generic.h>
#include <base/common/funcs.h>
#include <config/machine.hh>
#include <algorithm>

#ifdef CONFIG_FS
#include <fs/sys.hh>
#endif

#ifdef CONFIG_INPUT_EVDEV
#include <input/evdev/evdev.hh>
#endif

#define Time X11Time_
#define Pixmap X11Pixmap_
#define GC X11GC_
#define Window X11Window
#define BOOL X11BOOL
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>
#include <X11/cursorfont.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xfixes.h>
#undef Time
#undef Pixmap
#undef GC
#undef Window
#undef BOOL

// Choose GLX or EGL
#ifdef CONFIG_GFX_OPENGL_ES
#include "EglContext.hh"
#else
#include "GlxContext.hh"
#endif

#include <sys/epoll.h>
#include <time.h>
#include <errno.h>

namespace Base
{
	static Display *dpy;
	static X11Window win = None;
}

namespace Config
{
	namespace Base
	{
	#if defined CONFIG_MACHINE_PANDORA
	#define CONFIG_BASE_FBDEV_VSYNC
	static const bool FBDEV_VSYNC = true;
	#else
	static const bool FBDEV_VSYNC = false;
	#endif

	static const bool XDND = !Config::MACHINE_IS_PANDORA;
	}
}

#ifdef CONFIG_BASE_FBDEV_VSYNC
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#endif

#include "input.hh"
#include "xlibutils.h"
#include "dbusInstance.hh"
#include "xdnd.h"

namespace Base
{

const char *appPath = nullptr;
uint appState = APP_RUNNING;
static int screen;
static X11Window draggerWin = None;
static Atom dragAction = None;
static float dispXMM, dispYMM;
static int dispX, dispY;
static int ePoll = -1;
static int msgPipe[2] {0};
static int fbdev = -1;

#ifdef CONFIG_GFX_OPENGL_ES
static EglContext glCtx;
#else
static GlxContext glCtx;
#endif

static void cleanup()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void setWindowPixelBestColorHint(bool best)
{
	assert(win == None); // should only call before initial window is created
	glCtx.useMaxColorBits = best;
}

bool windowPixelBestColorHintDefault()
{
	return 1; // always prefer the best color format
}

// TODO: move into generic header after testing
static void fileURLToPath(char *url)
{
	char *pathStart = url;
	//lookup the 3rd slash which will signify the root directory of the file system
	for(uint i = 0; i < 3; i++)
	{
		pathStart = strchr(pathStart + 1, '/');
	}
	assert(pathStart != NULL);

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
			uchar msd = hexToInt(pathStart[srcPos]) << 4;
			srcPos++;
			uchar lsd = hexToInt(pathStart[srcPos]);
			url[destPos] = msd + lsd;
		}
	}
	url[destPos] = '\0';
}

void setAcceptDnd(bool on)
{
	if(!Config::Base::XDND)
		return;
	if(on)
		enableXdnd(dpy, win);
	else
		disableXdnd(dpy, win);
}

void setWindowTitle(const char *name)
{
	XTextProperty nameProp;
	char tempName[128];
	string_copy(tempName, name);
	char *tempNameArr[1] {tempName};
	if(XStringListToTextProperty(tempNameArr, 1, &nameProp))
	{
		XSetWMName(dpy, win, &nameProp);
		XFree(nameProp.value);
	}
	else
	{
		logWarn("unable to set window title, out of memory in XStringListToTextProperty()");
	}
}

static CallResult setupGLWindow(uint xres, uint yres, bool multisample)
{
	win = glCtx.init(dpy, screen, xres, yres, multisample, ExposureMask |
			PropertyChangeMask |
			StructureNotifyMask
			);
	glCtx.makeCurrent();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	Input::initPerWindowData(win);
	if(Config::MACHINE_IS_PANDORA)
	{
		auto wmState = XInternAtom(dpy, "_NET_WM_STATE", False);
		auto wmFullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
		XChangeProperty(dpy, win, wmState, XA_ATOM, 32, PropModeReplace, (unsigned char *)&wmFullscreen, 1);
	}
	else
	{
		auto wmDelete = XInternAtom(dpy, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(dpy, win, &wmDelete, 1);
		XSizeHints hints
		{
			PMinSize,
			0, 0, 0, 0,
			320, 240 // min size
		};
		XSetWMNormalHints(dpy, win, &hints);
	}
	logMsg("using depth %d", xDrawableDepth(dpy, win));

	// setup xinput2
	XIEventMask eventmask;
	uchar mask[2] = { 0 };
	eventmask.deviceid = XIAllMasterDevices;
	eventmask.mask_len = sizeof(mask); // always in bytes
	eventmask.mask = mask;
	XISetMask(mask, XI_ButtonPress);
	XISetMask(mask, XI_ButtonRelease);
	XISetMask(mask, XI_Motion);
	XISetMask(mask, XI_FocusIn);
	XISetMask(mask, XI_Enter);
	XISetMask(mask, XI_FocusOut);
	XISetMask(mask, XI_Leave);
	XISetMask(mask, XI_KeyPress);
	XISetMask(mask, XI_KeyRelease);
	XISelectEvents(dpy, win, &eventmask, 1);
	
	return OK;
}

CallResult openGLSetOutputVideoMode(const Base::Window &win)
{
	return setupGLWindow(win.w, win.h, 0);
}

CallResult openGLSetMultisampleVideoMode(const Base::Window &win)
{
	return setupGLWindow(win.w, win.h, 1);
}

void openGLUpdateScreen()
{
	#ifdef CONFIG_BASE_FBDEV_VSYNC
	if(fbdev >= 0)
	{
		int arg = 0;
		ioctl(fbdev, FBIO_WAITFORVSYNC, &arg);
	}
	#endif
	glCtx.swap();
}

void setVideoInterval(uint interval)
{
	glCtx.setSwapInterval(interval);
}

void setRefreshRate(uint rate)
{
	if(Config::MACHINE_IS_PANDORA)
	{
		if(rate == REFRESH_RATE_DEFAULT)
			rate = 60;
		if(rate != 50 && rate != 60)
		{
			logWarn("tried to set unsupported refresh rate: %u", rate);
		}
		char cmd[64];
		string_printf(cmd, "sudo /usr/pandora/scripts/op_lcdrate.sh %u", rate);
		int err = system(cmd);
		if(err)
		{
			logErr("error setting refresh rate, %d", err);
		}
	}
}

static void toggleFullScreen()
{
	logMsg("toggle fullscreen");
	ewmhFullscreen(dpy, win, _NET_WM_STATE_TOGGLE);
	displayNeedsUpdate();
}

void exitVal(int returnVal)
{
	onExit(0);
	cleanup();
	::exit(returnVal);
}

void abort() { ::abort(); }

static CallResult initX()
{
	/*if(!XInitThreads())
	{
		logErr("XInitThreads() failed");
		exit();
	}*/

	dpy = XOpenDisplay(0);
	if(!dpy)
	{
		logErr("couldn't open display");
		return INVALID_PARAMETER;
	}
	
	screen = DefaultScreen(dpy);
	logMsg("using default screen %d", screen);

	return OK;
}

void displayNeedsUpdate() { generic_displayNeedsUpdate(); }

/*void openURL(const char *url)
{
	// TODO
	logMsg("openURL called with %s", url);
}*/

const char *storagePath()
{
	if(Config::MACHINE_IS_PANDORA)
		return "/media";
	else
		return appPath;
}

static void updateViewSize()
{
	using namespace Gfx;
	assert(dispXMM);
	viewMMWidth_ = dispXMM * ((float)mainWin.w/(float)dispX);
	viewMMHeight_ = dispYMM * ((float)mainWin.h/(float)dispY);
	logMsg("view size in MM %dx%d", viewMMWidth_, viewMMHeight_);
}

static int eventHandler(XEvent event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);
	
	switch (event.type)
	{
		bcase Expose:
		{
			if (event.xexpose.count == 0)
				gfxUpdate = 1;
		}
		bcase ConfigureNotify:
		{
			//logMsg("ConfigureNotify");
			mainWin.rect.x2 = mainWin.w = event.xconfigure.width;
			mainWin.rect.y2 = mainWin.h = event.xconfigure.height;
			if(generic_resizeEvent(mainWin))
			{
				updateViewSize();
			}
		}
		bcase ClientMessage:
		{
			Atom type = event.xclient.message_type;
			char *clientMsgName = XGetAtomName(dpy, type);
			XClientMessageEvent *message = &event.xclient;
			//logDMsg("got client msg %s", clientMsgName);
			if(string_equal(clientMsgName, "WM_PROTOCOLS"))
			{
				logMsg("exiting via window manager");
				XFree(clientMsgName);
				exitVal(0);
			}
			else if(Config::Base::XDND && dndInit)
			{
				if(type == xdndAtom[XdndEnter])
				{
					//sendDNDStatus(dpy, win, message->data.l[dndMsgDropWindow]);
					draggerWin = message->data.l[dndMsgDropWindow];
				}
				else if(type == xdndAtom[XdndPosition])
				{
					if((Atom)message->data.l[4] == xdndAtom[XdndActionCopy]
						|| (Atom)message->data.l[4] == xdndAtom[XdndActionMove])
					{
						dragAction = (Atom)message->data.l[4];
						sendDNDStatus(dpy, win, message->data.l[dndMsgDropWindow], 1, dragAction);
					}
					else
					{
						char *action = XGetAtomName(dpy, message->data.l[4]);
						logMsg("rejecting drag & drop with unknown action %s", action);
						XFree(action);
						sendDNDStatus(dpy, win, message->data.l[dndMsgDropWindow], 0, dragAction);
					}
				}
				else if(type == xdndAtom[XdndDrop])
				{
					receiveDrop(dpy, win, message->data.l[dndMsgDropTimeStamp]);
				}
			}
			XFree(clientMsgName);
		}
		bcase PropertyNotify:
		{
			//logMsg("PropertyNotify");
		}
		bcase SelectionNotify:
		{
			logMsg("SelectionNotify");
			if(Config::Base::XDND && event.xselection.property != None)
			{
				int format;
				unsigned long numItems;
				unsigned long bytesAfter;
				unsigned char *prop;
				Atom type;
				XGetWindowProperty(dpy, win, event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				sendDNDFinished(dpy, win, draggerWin, dragAction);
				auto filename = (char*)prop;
				fileURLToPath(filename);
				onDragDrop(filename);
				XFree(prop);
			}
		}
		bcase MapNotify:
		{
			//logDMsg("MapNotfiy");
		}
		bcase ReparentNotify:
		{
			//logDMsg("ReparentNotify");
		}
		bcase GenericEvent:
		{
			if(event.xcookie.extension == Input::xI2opcode && XGetEventData(dpy, &event.xcookie))
			{
				XGenericEventCookie *cookie = &event.xcookie;
				auto &ievent = *((XIDeviceEvent*)cookie->data);
				//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
				switch(ievent.evtype)
				{
					bcase XI_ButtonPress:
						handlePointerButton(ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::PUSHED, ievent.event_x, ievent.event_y);
					bcase XI_ButtonRelease:
						handlePointerButton(ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::RELEASED, ievent.event_x, ievent.event_y);
					bcase XI_Motion:
						handlePointerMove(ievent.event_x, ievent.event_y, Input::devIdToPointer(ievent.deviceid));
					bcase XI_Enter:
						handlePointerEnter(Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y);
					bcase XI_Leave:
						handlePointerLeave(Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y);
					bcase XI_FocusIn:
						onFocusChange(1);
					bcase XI_FocusOut:
						onFocusChange(0);
					bcase XI_KeyPress:
					{
						auto dev = Input::deviceForInputId(ievent.sourceid);
						KeySym k;
						if(Input::translateKeycodes)
						{
							unsigned int modsReturn;
							XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
						}
						else
							 k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
						bool repeated = ievent.flags & XIKeyRepeat;
						//logMsg("press KeySym %d, KeyCode %d, repeat: %d", (int)k, ievent.detail, repeated);
						if(k == XK_Return && (ievent.mods.effective & Mod1Mask) && !repeated)
						{
							toggleFullScreen();
						}
						else
						{
							using namespace Input;
							if(!repeated || Input::allowKeyRepeats)
							{
								#ifdef CONFIG_INPUT_ICADE
								if(!dev->iCadeMode()
										|| (dev->iCadeMode() && !processICadeKey(decodeAscii(k, 0), Input::PUSHED, *dev)))
								#endif
									handleKeyEv(k, Input::PUSHED, ievent.mods.effective & ShiftMask, dev);
							}
						}
					}
					bcase XI_KeyRelease:
					{
						auto dev = Input::deviceForInputId(ievent.sourceid);
						KeySym k;
						if(Input::translateKeycodes)
						{
							unsigned int modsReturn;
							XkbTranslateKeyCode(Input::coreKeyboardDesc, ievent.detail, ievent.mods.effective, &modsReturn, &k);
						}
						else
							 k = XkbKeycodeToKeysym(dpy, ievent.detail, 0, 0);
						//logMsg("release KeySym %d, KeyCode %d", (int)k, ievent.detail);
						handleKeyEv(k, Input::RELEASED, 0, dev);
					}
					bcase XI_HierarchyChanged:
					{
						//logMsg("input device hierarchy changed");
						auto &ev = *((XIHierarchyEvent*)cookie->data);
						iterateTimes(ev.num_info, i)
						{
							if(ev.info[i].flags & XISlaveAdded)
							{
								int devices;
								XIDeviceInfo *device = XIQueryDevice(dpy, ev.info[i].deviceid, &devices);
								if(devices)
								{
									if(device->use == XISlaveKeyboard)
									{
										Input::addXInputDevice(*device, true);
									}
									XIFreeDeviceInfo(device);
								}
							}
							else if(ev.info[i].flags & XISlaveRemoved)
							{
								Input::removeXInputDevice(ev.info[i].deviceid);
							}
						}
					}
				}
				XFreeEventData(dpy, &event.xcookie);
				return 1;
			}
		}
		bdefault:
		{
			logDMsg("got unhandled message type %d", event.type);
		}
		break;
	}

	return 1;
}

void x11FDHandler()
{
	while(XPending(dpy) > 0)
	{
		XEvent event;
		XNextEvent(dpy, &event);
		eventHandler(event);
	}
}

void addPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	logMsg("adding fd %d to epoll", fd);
	struct epoll_event ev = { 0 };
	ev.data.ptr = &handler;
	ev.events = events;
	assert(ePoll != -1);
	epoll_ctl(ePoll, EPOLL_CTL_ADD, fd, &ev);
}

void modPollEvent(int fd, PollEventDelegate &handler, uint events)
{
	struct epoll_event ev = { 0 };
	ev.data.ptr = &handler;
	ev.events = /*EPOLLET|*/events;
	assert(ePoll != -1);
	epoll_ctl(ePoll, EPOLL_CTL_MOD, fd, &ev);
}

void removePollEvent(int fd)
{
	logMsg("removing fd %d from epoll", fd);
	epoll_ctl(ePoll, EPOLL_CTL_DEL, fd, nullptr);
}

static void setupScreenSizeFromX11()
{
	dispXMM = DisplayWidthMM(dpy, screen);
	dispYMM = DisplayHeightMM(dpy, screen);
}

void setDPI(float dpi)
{
	assert(dispX);
	if(dpi == 0)
	{
		setupScreenSizeFromX11();
	}
	else
	{
		logMsg("requested DPI %f", (double)dpi);
		dispXMM = ((float)dispX / dpi) * 25.4;
		dispYMM = ((float)dispY / dpi) * 25.4;
	}

	updateViewSize();
	Gfx::setupScreenSize();
}

void sendMessageToMain(int type, int shortArg, int intArg, int intArg2)
{
	uint16 shortArg16 = shortArg;
	int32 msg[3] = { (shortArg16 << 16) | type, intArg, intArg2 };
	logMsg("sending msg type %d with args %d %d %d", msg[0] & 0xFFFF, msg[0] >> 16, msg[1], msg[2]);
	if(::write(msgPipe[1], &msg, sizeof(msg)) != sizeof(msg))
	{
		logErr("unable to write message to pipe: %s", strerror(errno));
	}
}

void sendMessageToMain(ThreadPThread &, int type, int shortArg, int intArg, int intArg2)
{
	sendMessageToMain(type, shortArg, intArg, intArg2);
}

void registerInstance(const char *name, int argc, char** argv)
{
	if(!bus)
	{
		logErr("DBUS not init");
		return;
	}

	if(uniqueInstanceRunning(bus, name))
	{
		if(argc < 2)
		{
			dbus_connection_close(bus);
			Base::exit();
		}
		// send msg
		auto path = argv[1];
		char realPath[PATH_MAX];
		if(argv[1][0] != '/') // is path absolute?
		{
			if(!realpath(path, realPath))
			{
				logErr("error in realpath()");
				Base::exitVal(1);
			}
			path = realPath;
		}
		logMsg("sending dbus signal to other instance with arg: %s", path);
		DBusMessage *message = dbus_message_new_signal(DBUS_APP_OBJECT_PATH, name, "openPath");
		assert(message);
		dbus_message_append_args(message, DBUS_TYPE_STRING, &path, DBUS_TYPE_INVALID);
		dbus_connection_send(bus, message, nullptr);
		dbus_message_unref(message);
		dbus_connection_flush(bus);
		dbus_connection_close(bus);
		Base::exit();
	}
	else
	{
		// listen to dbus events
		if(setupDbusListener(name))
		{
			if (!dbus_connection_set_watch_functions(bus, addDbusWatch,
					removeDbusWatch, toggleDbusWatch, bus, nullptr))
			{
				logErr("dbus_connection_set_watch_functions failed");
				deinitDBus();
			}
			else
			{
				logMsg("setup dbus listener");
			}
		}
	}
}

}

static int epollWaitWrapper(int epfd, struct epoll_event *events,
	int maxevents, int timeout)
{
	x11FDHandler();  // must check X before entering epoll since some events may be
										// in memory queue and won't trigger the FD
	return epoll_wait(epfd, events, maxevents, timeout);
}

int main(int argc, char** argv)
{
	using namespace Base;

	doOrExit(logger_init());

	#ifdef CONFIG_FS
	FsSys::changeToAppDir(argv[0]);
	#endif

	initDBus();
	ePoll = epoll_create(8);

	if(Config::Base::FBDEV_VSYNC)
	{
		fbdev = open("/dev/fb0", O_RDONLY);
		if(fbdev == -1)
		{
			logWarn("unable to open framebuffer device");
		}
	}

	doOrElse(initX(), return 1);
	#ifdef CONFIG_INPUT
	doOrExit(Input::init());
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	Input::initEvdev();
	#endif
	doOrExit(onInit(argc, argv));
	dispX = DisplayWidth(dpy, screen);
	dispY = DisplayHeight(dpy, screen);
	{
		#ifdef CONFIG_MACHINE_PANDORA
		int x = 800;
		int y = 480;
		#else
		int x = 1024;
		int y = 768;

		// try to crop window to workable desktop area
		long *workArea;
		int format;
		unsigned long items;
		unsigned long bytesAfter;
		uchar *prop;
		Atom type;
		Atom _NET_WORKAREA = XInternAtom(dpy, "_NET_WORKAREA", 0);
		if(XGetWindowProperty(dpy, DefaultRootWindow(dpy),
			_NET_WORKAREA, 0, ~0, False,
			XA_CARDINAL, &type, &format, &items, &bytesAfter, (uchar **)&workArea) || !workArea)
		{
			logWarn("error getting desktop work area");
		}
		else
		{
			logMsg("%ld %ld work area: %ld:%ld:%ld:%ld", items, bytesAfter, workArea[0], workArea[1], workArea[2], workArea[3]);
			x = std::min(x, (int)workArea[2]);
			y = std::min(y, (int)workArea[3]);
			XFree(workArea);
		}
		#endif

		mainWin.w = mainWin.rect.x2 = x;
		mainWin.h = mainWin.rect.y2 = y;
	}

	setupScreenSizeFromX11();
	updateViewSize();

	{
		int res = pipe(msgPipe);
		assert(res == 0);
	}

	PollEventDelegate msgPoll =
		[](int events)
		{
			while(fd_bytesReadable(msgPipe[0]))
			{
				uint32 msg[3];
				if(read(msgPipe[0], msg, sizeof(msg)) == -1)
				{
					logErr("error reading from pipe");
					return 1;
				}
				auto cmd = msg[0] & 0xFFFF, shortArg = msg[0] >> 16;
				logMsg("got msg type %d with args %d %d %d", cmd, shortArg, msg[1], msg[2]);
				Base::processAppMsg(cmd, shortArg, msg[1], msg[2]);
			}
			return 1;
		};
	addPollEvent(msgPipe[0], msgPoll);
	
	PollEventDelegate x11Poll =
		[](int events)
		{
			x11FDHandler();
			return 1;
		};
	addPollEvent(ConnectionNumber(dpy), x11Poll);
	openGLSetOutputVideoMode(mainWin);
	engineInit();
	XMapRaised(dpy, win);

	logMsg("entering event loop");
	for(;;)
	{
		struct epoll_event event[16];
		int events;
		while((events = epollWaitWrapper(ePoll, event, sizeofArray(event), getPollTimeout())) > 0)
		{
			//logMsg("%d events ready", events);
			iterateTimes(events, i)
			{
				auto &e = *((PollEventDelegate*)event[i].data.ptr);
				e(event[i].events);
			}
		}
		if(events == -1)
		{
			if(errno == EINTR)
			{
				logMsg("epoll_wait interrupted by signal");
				continue;
			}
			else
				bug_exit("epoll_wait failed with errno %d", errno);
		}
		runEngine(0);
	}
	return 0;
}
