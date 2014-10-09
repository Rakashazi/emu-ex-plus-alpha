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

#define LOGTAG "DBus"
#include <unistd.h>
#include <sys/param.h>
#include <dbus/dbus.h>
#include "dbus.hh"
#include "../common/basePrivate.hh"
#include <imagine/base/EventLoopFileSource.hh>
#ifdef CONFIG_BASE_GLIB
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#endif

static DBusConnection *bus{};
#define DBUS_APP_OBJECT_PATH "/com/explusalpha/imagine"
static const char *dbusAppInterface{};

bool initDBus()
{
	if(bus)
		return true;
	#ifdef CONFIG_BASE_GLIB
	GError *err = nullptr;
	auto gbus = dbus_g_bus_get(DBUS_BUS_SESSION, &err);
	if(!gbus)
	{
		logWarn("error getting DBUS session: %s", err->message);
		g_error_free(err);
		return false;
	}
	bus = dbus_g_connection_get_connection(gbus);
	#else
	DBusError err{};
	bus = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error getting DBUS session");
		dbus_error_free(&err);
		return false;
	}
	#endif
	return true;
}

void deinitDBus()
{
	if(!bus)
		return;
	dbus_connection_close(bus);
	bus = nullptr;
}

static DBusHandlerResult dbusSignalHandler(DBusConnection *connection, DBusMessage *message, void *user_data)
{
	logMsg("got dbus signal");
	if(dbus_message_is_signal(message, dbusAppInterface, "openPath"))
	{
		DBusError error;
		const char *path;
		dbus_error_init (&error);
		if(dbus_message_get_args(message, &error, DBUS_TYPE_STRING, &path, DBUS_TYPE_INVALID))
		{
			logMsg("signal openPath: %s", path);
			if(path)
			{
				Base::dispatchOnInterProcessMessage(path);
			}
    }
		else
		{
			logErr("signal openPath but error getting args: %s", error.message);
      dbus_error_free(&error);
    }
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

#ifndef CONFIG_BASE_GLIB
struct DBusWatchHandler
{
	Base::EventLoopFileSource fdSrc;

	void setupFDEvents(int fd, int events)
	{
		fdSrc.init(fd,
			[this](int fd, int event)
			{
				using namespace Base;
				uint flags = 0;
				if (event & POLLEV_IN)
					flags |= DBUS_WATCH_READABLE;
				if (event & POLLEV_OUT)
					flags |= DBUS_WATCH_WRITABLE;
				if (event & POLLEV_HUP)
					flags |= DBUS_WATCH_HANGUP;
				if (event & POLLEV_ERR)
					flags |= DBUS_WATCH_ERROR;

				while(!dbus_watch_handle(watch, flags))
				{
					logWarn("dbus_watch_handle needs more memory");
					return 1;
				}

				//dbus_connection_ref(conn);
				int messages = 0;
				while(dbus_connection_dispatch(conn) == DBUS_DISPATCH_DATA_REMAINS)
				{
					messages++;
				}
				//dbus_connection_unref(conn);
				logMsg("dispatched %d dbus messages for watch %p", messages, watch);
				return 1;
			}, events);
	}

	DBusConnection *conn;
	DBusWatch *watch;

	DBusWatchHandler(DBusConnection *conn, DBusWatch *watch): conn(conn), watch(watch) {}
};

static dbus_bool_t addDBusWatch(DBusWatch *watch, void *conn)
{
	using namespace Base;
	auto flags = dbus_watch_get_flags(watch);
	auto fd = dbus_watch_get_unix_fd(watch);
	uint events = 0;
	if (flags & DBUS_WATCH_READABLE)
		events |= POLLEV_IN;
	if (flags & DBUS_WATCH_WRITABLE)
		events |= POLLEV_OUT;
	if(flags & DBUS_WATCH_READABLE)
	{
		logMsg("adding dbus readable watch %p with fd %d", watch, fd);
		auto handler = new DBusWatchHandler((DBusConnection*)conn, watch);
		dbus_watch_set_data(watch, handler, nullptr);
		handler->setupFDEvents(fd, events);
	}
	else
	{
		dbus_watch_set_data(watch, nullptr, nullptr);
	}
	return 1;
}

static void toggleDBusWatch(DBusWatch *watch, void *data)
{
	logMsg("toggle dbus watch %p", watch);
}

static void removeDBusWatch(DBusWatch *watch, void *conn)
{
	logMsg("removing dbus watch %p", watch);
	auto handler = (DBusWatchHandler*)dbus_watch_get_data(watch);
	if(handler)
	{
		handler->fdSrc.deinit();
		delete handler;
	}
}

bool setDefaultDBusWatchFuncs()
{
	return dbus_connection_set_watch_functions(bus, addDBusWatch,
		removeDBusWatch, toggleDBusWatch, bus, nullptr);
}
#endif

static bool setupDBusListener(const char *name)
{
	DBusError err{};
	dbusAppInterface = name;
	auto ruleStr = string_makePrintf<128>("type='signal',interface='%s',path='" DBUS_APP_OBJECT_PATH "'", name);
	dbus_bus_add_match(bus, ruleStr.data(), &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error registering rule: %s", err.message);
		dbus_error_free(&err);
		return false;
	}
	return dbus_connection_add_filter(bus, dbusSignalHandler, nullptr, nullptr);
}

bool uniqueInstanceRunning(DBusConnection *bus, const char *name)
{
	DBusError err{};
	auto ret = dbus_bus_request_name(bus, name, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error in dbus_bus_request_name");
		dbus_error_free(&err);
		return false;
	}

	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret)
	{
		logMsg("no running instance detected");
		return false;
	}
	else
	{
		logMsg("running instance detected");
		return true;
	}
}

namespace Base
{

void registerInstance(const char *name, int argc, char** argv)
{
	if(!initDBus())
	{
		logErr("can't init DBUS");
		return;
	}

	if(!uniqueInstanceRunning(bus, name))
	{
		// no running intance
		return;
	}

	if(argc < 2)
	{
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
			Base::exit(1);
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
	Base::exit();
}

void setAcceptIPC(const char *name, bool on)
{
	assert(on);
	if(!initDBus())
	{
		logErr("can't init DBUS");
		return;
	}

	// listen to dbus events
	if(setupDBusListener(name))
	{
		#ifndef CONFIG_BASE_GLIB
		if(!setDefaultDBusWatchFuncs())
		{
			logErr("dbus_connection_set_watch_functions failed");
			deinitDBus();
		}
		else
		#endif
		{
			logMsg("setup dbus listener");
		}
	}
}

}
