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
#include <gio/gio.h>
#include "dbus.hh"
#include "../common/basePrivate.hh"
#include <imagine/base/EventLoop.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/string.h>

#define DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER 1
static const char *appObjectPath = "/com/explusalpha/imagine";
static GDBusConnection *gbus{};
static guint openPathSub = 0;

bool initDBus()
{
	if(gbus)
		return true;
	GError *err{};
	gbus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &err);
	if(!gbus)
	{
		logErr("error getting DBUS session: %s", err->message);
		g_error_free(err);
		return false;
	}
	return true;
}

void deinitDBus()
{
	if(!gbus)
		return;
	g_object_unref(gbus);
	gbus = nullptr;
	openPathSub = 0;
}

static guint setOpenPathListener(GDBusConnection *bus, const char *name)
{
	return g_dbus_connection_signal_subscribe(bus,
		name, name, "openPath", appObjectPath,
		nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
		[](GDBusConnection *connection, const gchar *name, const gchar *path, const gchar *interface,
			const gchar *signal, GVariant *param, gpointer)
		{
			if(!g_variant_is_of_type(param, G_VARIANT_TYPE("(s)")))
			{
				logErr("invalid arg type for signal openPath");
				return;
			}
			gchar *openPath;
			g_variant_get(param, "(s)", &openPath);
			Base::dispatchOnInterProcessMessage(openPath);
		},
		nullptr,
		nullptr);
}

bool uniqueInstanceRunning(GDBusConnection *bus, const char *name)
{
	GError *err{};
	auto retVar = g_dbus_connection_call_sync(bus,
		"org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
		"RequestName", g_variant_new("(su)", name, (uint32_t)G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE),
		G_VARIANT_TYPE("(u)"),
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		&err);
	if(err)
	{
		logErr("error calling RequestName: %s", err->message);
		g_error_free(err);
		return false;
	}
	uint32_t reply;
	g_variant_get(retVar, "(u)", &reply);
	g_variant_unref(retVar);
	if(reply == DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
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

static bool allowScreenSaver = true;
static uint32_t screenSaverCookie = 0;

void setIdleDisplayPowerSave(bool wantsAllowScreenSaver)
{
	if(allowScreenSaver == wantsAllowScreenSaver)
		return;
	if(!initDBus())
		return;
	if(wantsAllowScreenSaver && screenSaverCookie)
	{
		g_dbus_connection_call(gbus,
			"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver",
			"UnInhibit", g_variant_new("(u)", screenSaverCookie),
			nullptr,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			nullptr,
			nullptr,
			nullptr);
	}
	else if(!wantsAllowScreenSaver)
	{
		GError *err{};
		auto retVar = g_dbus_connection_call_sync(gbus,
			"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver",
			"Inhibit", g_variant_new("(ss)", "Imagine app", "App request"),
			G_VARIANT_TYPE("(u)"),
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			nullptr,
			&err);
		if(err)
		{
			logErr("error calling Inhibit: %s", err->message);
			g_error_free(err);
			return;
		}
		g_variant_get(retVar, "(u)", &screenSaverCookie);
		g_variant_unref(retVar);
		logMsg("Got screensaver inhibit cookie:%u", screenSaverCookie);
	}
	allowScreenSaver = wantsAllowScreenSaver;
}

void endIdleByUserActivity()
{
	if(!allowScreenSaver)
		return;
	if(!initDBus())
		return;
	g_dbus_connection_call(gbus,
		"org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", "org.freedesktop.ScreenSaver",
		"SimulateUserActivity", nullptr,
		nullptr,
		G_DBUS_CALL_FLAGS_NONE,
		-1,
		nullptr,
		nullptr,
		nullptr);
}

void registerInstance(const char *name, int argc, char** argv)
{
	if(!initDBus())
		return;
	if(!uniqueInstanceRunning(gbus, name))
	{
		// no running intance
		return;
	}
	if(argc < 2)
	{
		Base::exit();
	}
	// send signal
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
	g_dbus_connection_emit_signal(gbus,
		name, appObjectPath, name,
		"openPath", g_variant_new("(s)", path),
		nullptr);
	g_dbus_connection_flush_sync(gbus, nullptr, nullptr);
	Base::exit();
}

void setAcceptIPC(const char *name, bool on)
{
	assert(on);
	if(!initDBus())
		return;
	// listen to openPath events
	if(!openPathSub)
	{
		openPathSub = setOpenPathListener(gbus, name);
	}
}

}
