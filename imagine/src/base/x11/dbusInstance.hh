#pragma once

#include <dbus/dbus.h>

static DBusConnection *bus = nullptr;
//static const char *instanceName = CONFIG_APP_ID "-instance";

#define DBUS_APP_OBJECT_PATH "/com/explusalpha/imagine"
//#define DBUS_APP_INTERFACE CONFIG_APP_ID
static const char *dbusAppInterface = nullptr;

static bool initDBus()
{
	if(bus)
		return 1;
	DBusError err {0};
	bus = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error getting DBUS session");
		dbus_error_free(&err);
		return 0;
	}
	return 1;
}

static void deinitDBus()
{
	assert(bus);
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
				Base::onInterProcessMessage(path);
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

struct DBusWatchHandler
{
	DBusWatchHandler(DBusConnection *conn, DBusWatch *watch): conn(conn), watch(watch) { }
	Base::PollEventDelegate pollEvDel
	{
		[this](int event)
		{
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
		}
	};
	DBusConnection *conn;
	DBusWatch *watch;
};

static dbus_bool_t addDbusWatch(DBusWatch *watch, void *conn)
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
		addPollEvent(fd, handler->pollEvDel, events);
	}
	else
	{
		dbus_watch_set_data(watch, nullptr, nullptr);
	}
	return 1;
}

static void toggleDbusWatch(DBusWatch *watch, void *data)
{
	logMsg("toggle dbus watch %p", watch);
}

static void removeDbusWatch(DBusWatch *watch, void *conn)
{
	logMsg("removing dbus watch %p", watch);
	removePollEvent(dbus_watch_get_unix_fd(watch));
	auto handler = (DBusWatchHandler*)dbus_watch_get_data(watch);
	delete handler;
}

static bool setupDbusListener(const char *name)
{
	DBusError err {0};
	dbusAppInterface = name;
	char ruleStr[128];
	string_printf(ruleStr, "type='signal',interface='%s',path='" DBUS_APP_OBJECT_PATH "'", name);
	dbus_bus_add_match(bus, ruleStr, &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error registering rule: %s", err.message);
		dbus_error_free(&err);
		return false;
	}
	return dbus_connection_add_filter(bus, dbusSignalHandler, nullptr, nullptr);
}

static int uniqueInstanceRunning(DBusConnection *bus, const char *name)
{
	DBusError err {0};
	auto ret = dbus_bus_request_name(bus, name, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);
	if(dbus_error_is_set(&err))
	{
		logWarn("error in dbus_bus_request_name");
		dbus_error_free(&err);
		return 0;
	}

	if(DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER == ret)
	{
		logMsg("no running instance detected");
		return 0;
	}
	else
	{
		logMsg("running instance detected");
		return 1;
	}
}
