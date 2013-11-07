#define thisModuleName "base:x11"
#include <cstdlib>
#include <unistd.h>
#include <engine-globals.h>
#include <input/Input.hh>
#include <logger/interface.h>
#include <base/Base.hh>
#include <base/common/windowPrivate.hh>
#include <util/strings.h>
#include <util/time/sys.hh>
#include <util/fd-utils.h>
#include <util/string/generic.h>
#include <util/cLang.h>
#include <base/common/funcs.h>
#include <base/x11/x11.hh>
#include <base/x11/xdnd.hh>
#include <config/machine.hh>
#include <algorithm>

#ifdef CONFIG_FS
#include <fs/sys.hh>
#endif

#ifdef CONFIG_INPUT_EVDEV
#include <input/evdev/evdev.hh>
#endif

#include <sys/epoll.h>
#include <time.h>
#include <errno.h>

#include "input.hh"
#include "xlibutils.h"
#include "dbusInstance.hh"

namespace Base
{

const char *appPath = nullptr;
uint appState = APP_RUNNING;
int screen;
float dispXMM, dispYMM;
int dispX, dispY;
static int ePoll = -1;
static int msgPipe[2] {0};
int fbdev = -1;
Display *dpy;

static void cleanup()
{
	shutdownWindowSystem();
	XCloseDisplay(dpy);
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

uint refreshRate()
{
	auto conf = XRRGetScreenInfo(dpy, RootWindow(dpy, 0));
	auto rate = XRRConfigCurrentRate(conf);
	logMsg("refresh rate %d", (int)rate);
	return rate;
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

static void toggleFullScreen(Base::Window &win)
{
	logMsg("toggle fullscreen");
	ewmhFullscreen(dpy, win.xWin, _NET_WM_STATE_TOGGLE);
	win.displayNeedsUpdate();
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

/*void openURL(const char *url)
{
	// TODO
	logMsg("openURL called with %s", url);
}*/

const char *storagePath()
{
	if(Config::MACHINE_IS_PANDORA)
	{
		static FsSys::cPath sdPath {""};
		FsSys dir;
		// look for the first mounted SD card
		if(dir.openDir("/media", 0,
			[](const char *name, int type) -> int
			{
				return type == Fs::TYPE_DIR && strstr(name, "mmcblk");
			}
		) == OK)
		{
			if(dir.numEntries())
			{
				//logMsg("storage dir: %s", dir.entryFilename(0));
				string_printf(sdPath, "/media/%s", dir.entryFilename(0));
			}
			else
				sdPath[0] = 0;
			dir.closeDir();
			if(strlen(sdPath))
			{
				return sdPath;
			}
		}
		// fall back to appPath
	}
	return appPath;
}

static int eventHandler(XEvent event)
{
	//logMsg("got event type %s (%d)", xEventTypeToString(event.type), event.type);
	
	switch (event.type)
	{
		bcase Expose:
		{
			auto &win = *windowForXWindow(event.xexpose.window);
			if (event.xexpose.count == 0)
				win.displayNeedsUpdate();
		}
		bcase ConfigureNotify:
		{
			//logMsg("ConfigureNotify");
			auto &win = *windowForXWindow(event.xconfigure.window);
			if(event.xconfigure.width == win.w && event.xconfigure.height == win.h)
				break;
			win.updateSize(event.xconfigure.width, event.xconfigure.height);
			win.postResize();
		}
		bcase ClientMessage:
		{
			auto &win = *windowForXWindow(event.xclient.window);
			auto type = event.xclient.message_type;
			char *clientMsgName = XGetAtomName(dpy, type);
			//logDMsg("got client msg %s", clientMsgName);
			if(string_equal(clientMsgName, "WM_PROTOCOLS"))
			{
				logMsg("exiting via window manager");
				XFree(clientMsgName);
				exitVal(0);
			}
			else if(Config::Base::XDND && dndInit)
			{
				handleXDNDEvent(dpy, event.xclient, win.xWin, win.draggerXWin, win.dragAction);
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
				auto &win = *windowForXWindow(event.xselection.requestor);
				int format;
				unsigned long numItems;
				unsigned long bytesAfter;
				unsigned char *prop;
				Atom type;
				XGetWindowProperty(dpy, win.xWin, event.xselection.property, 0, 256, False, AnyPropertyType, &type, &format, &numItems, &bytesAfter, &prop);
				logMsg("property read %lu items, in %d format, %lu bytes left", numItems, format, bytesAfter);
				logMsg("property is %s", prop);
				sendDNDFinished(dpy, win.xWin, win.draggerXWin, win.dragAction);
				auto filename = (char*)prop;
				fileURLToPath(filename);
				onDragDrop(win, filename);
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
				auto destWin = windowForXWindow(ievent.event);
				if(!destWin)
				{
					// ignore events for other windows
					XFreeEventData(dpy, &event.xcookie);
					break;
				}
				auto &win = *destWin;
				//logMsg("device %d, event %s", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
				switch(ievent.evtype)
				{
					bcase XI_ButtonPress:
						handlePointerButton(win, ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::PUSHED, ievent.event_x, ievent.event_y);
					bcase XI_ButtonRelease:
						handlePointerButton(win, ievent.detail, Input::devIdToPointer(ievent.deviceid), Input::RELEASED, ievent.event_x, ievent.event_y);
					bcase XI_Motion:
						handlePointerMove(win, ievent.event_x, ievent.event_y, Input::devIdToPointer(ievent.deviceid));
					bcase XI_Enter:
						handlePointerEnter(win, Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y);
					bcase XI_Leave:
						handlePointerLeave(win, Input::devIdToPointer(ievent.deviceid), ievent.event_x, ievent.event_y);
					bcase XI_FocusIn:
						onFocusChange(win, 1);
					bcase XI_FocusOut:
						onFocusChange(win, 0);
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
							toggleFullScreen(win);
						}
						else
						{
							using namespace Input;
							if(!repeated || Input::allowKeyRepeats)
							{
								#ifdef CONFIG_INPUT_ICADE
								if(!dev->iCadeMode()
										|| (dev->iCadeMode() && !processICadeKey(decodeAscii(k, 0), Input::PUSHED, *dev, win)))
								#endif
									handleKeyEv(win, k, Input::PUSHED, ievent.mods.effective & ShiftMask, dev);
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
						handleKeyEv(win, k, Input::RELEASED, 0, dev);
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

void setupScreenSizeFromX11()
{
	dispXMM = DisplayWidthMM(dpy, screen);
	dispYMM = DisplayHeightMM(dpy, screen);
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

static int epollWaitWrapper(int epfd, struct epoll_event *events, int maxevents)
{
	x11FDHandler();  // must check X before entering epoll since some events may be
										// in memory queue and won't trigger the FD
	return epoll_wait(epfd, events, maxevents, getPollTimeout());
}

int main(int argc, char** argv)
{
	using namespace Base;

	doOrAbort(logger_init());
	engineInit();

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
	doOrAbort(Input::init());
	#endif
	#ifdef CONFIG_INPUT_EVDEV
	Input::initEvdev();
	#endif
	dispX = DisplayWidth(dpy, screen);
	dispY = DisplayHeight(dpy, screen);
	setupScreenSizeFromX11();

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

	doOrAbort(onInit(argc, argv));

	logMsg("entering event loop");
	for(;;)
	{
		struct epoll_event event[16];
		int events;
		while((events = epollWaitWrapper(ePoll, event, sizeofArray(event))) > 0)
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
		drawWindows(0);
	}
	return 0;
}
