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

#define LOGTAG "Input"
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/x11/XInputDevice.hh>
#include <imagine/input/Event.hh>
#include <imagine/logger/logger.h>
#include <imagine/util/algorithm.h>
#include <imagine/util/ScopeGuard.hh>
#include <imagine/util/bit.hh>
#include "xlibutils.h"
#include <xcb/xinput.h>
#include <xkbcommon/xkbcommon-x11.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <memory>

static std::string_view xiDeviceInfoName(const xcb_input_xi_device_info_t& info)
{
	return {xcb_input_xi_device_info_name(&info), size_t(xcb_input_xi_device_info_name_length(&info))};
}

namespace IG::Input
{

XInputDevice::XInputDevice(Input::DeviceTypeFlags typeFlags, std::string name):
	BaseDevice{0, Input::Map::SYSTEM, typeFlags, std::move(name)} {}

XInputDevice::XInputDevice(xcb_input_xi_device_info_t& info, bool isPointingDevice, bool isPowerButton):
	BaseDevice{info.deviceid, Input::Map::SYSTEM, {}, std::string{xiDeviceInfoName(info)}}
{
	if(isPointingDevice)
	{
		typeFlags_.mouse = true;
	}
	else
	{
		typeFlags_.keyboard = true;
		typeFlags_.powerButton = isPowerButton;
	}
}

bool Device::anyTypeFlagsPresent(ApplicationContext, DeviceTypeFlags typeFlags)
{
	// TODO
	if(typeFlags.keyboard)
	{
		return true;
	}
	return false;
}

std::string KeyEvent::keyString(ApplicationContext ctx) const
{
	return ctx.application().inputKeyString(rawKey, metaState);
}

}

namespace IG
{

constexpr SystemLogger log{"X11"};
constexpr int XC_left_ptr = 68; // from X11/cursorfont.h

static bool isXInputDevice(Input::Device &d)
{
	return d.map() == Input::Map::SYSTEM && (d.typeFlags().mouse || d.typeFlags().keyboard);
}

static bool hasXInputDeviceId(Input::Device &d, int id)
{
	return isXInputDevice(d) && d.id() == id;
}

static inline float fixed1616ToFloat(xcb_input_fp1616_t val)
{
	return float(val) / 0x10000;
}

struct XIEventMaskData
{
	xcb_input_event_mask_t header;
	xcb_input_xi_event_mask_t mask;
};

const Input::Device *XApplication::deviceForInputId(int osId) const
{
	for(auto &devPtr : inputDev)
	{
		if(hasXInputDeviceId(*devPtr, osId))
		{
			return devPtr.get();
		}
	}
	if(!vkbDevice)
		log.error("device id {} doesn't exist", osId);
	return vkbDevice;
}

constexpr XIEventMaskData windowXIEventMaskData()
{
	XIEventMaskData data;
	data.header.deviceid = XCB_INPUT_DEVICE_ALL_MASTER;
	data.header.mask_len = sizeof(data.mask) / sizeof(uint32_t);
	data.mask = xcb_input_xi_event_mask_t(
		to_underlying(XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_MOTION) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_FOCUS_IN) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_ENTER) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_FOCUS_OUT) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_LEAVE) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_KEY_PRESS) |
		to_underlying(XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE));
	return data;
}

void XApplication::initPerWindowInputData(xcb_window_t win)
{
	if constexpr(Config::MACHINE_IS_PANDORA)
	{
		xcb_xfixes_hide_cursor(xConn, win);
	}
	else
	{
		if(!blankCursor)
		{
			// make a blank cursor
			auto blank = xcb_generate_id(xConn);
			xcb_create_pixmap(xConn, 1, blank, win, 1, 1);
			blankCursor = xcb_generate_id(xConn);
			xcb_create_cursor(xConn, blankCursor, blank, blank, 0, 0, 0, 0, 0, 0, 0, 0);
			xcb_free_pixmap(xConn, blank);
			// make left pointer cursor
			auto font = xcb_generate_id(xConn);
			std::string_view cursorStr{"cursor"};
			xcb_open_font(xConn, font, cursorStr.size(), cursorStr.data());
			normalCursor = xcb_generate_id(xConn);
			xcb_create_glyph_cursor(xConn, normalCursor, font, font, XC_left_ptr, XC_left_ptr + 1, 0, 0, 0, 0, 0, 0);
			xcb_close_font(xConn, font);
		}
	}
	auto evMaskData = windowXIEventMaskData();
	xcb_input_xi_select_events(xConn, win, 1, &evMaskData.header);
}

void XApplication::initXInput2()
{
	const xcb_query_extension_reply_t *extReply = xcb_get_extension_data(xConn, &xcb_input_id);
	if(!extReply || !extReply->present)
	{
		log.error("XInput extension not available");
		::exit(-1);
	}
	xI2opcode = extReply->major_opcode;
	auto verReply = XCB_REPLY(xcb_input_xi_query_version, xConn, 2, 0);
	if(!verReply || verReply->major_version < 2)
	{
		log.error("required XInput 2.x version not available, server supports {}.{}", verReply->major_version, verReply->minor_version);
		::exit(-1);
	}
}

static bool isPowerButtonName(std::string_view name)
{
	return name.contains("Power Button")
		|| (Config::MACHINE_IS_PANDORA && name.contains("power-button"));
}

void XApplication::addXInputDevice(xcb_input_xi_device_info_t& xDevInfo, bool notify, bool isPointingDevice)
{
	auto devName = xiDeviceInfoName(xDevInfo);
	for(auto &e : inputDev)
	{
		if(hasXInputDeviceId(*e, xDevInfo.deviceid))
		{
			log.info("X input device {} ({}) is already present", xDevInfo.deviceid, devName);
			return;
		}
	}
	log.info("adding X input device {} ({}) to device list", xDevInfo.deviceid, devName);
	auto devPtr = std::make_unique<Input::Device>(std::in_place_type<Input::XInputDevice>, xDevInfo, isPointingDevice, isPowerButtonName(devName));
	if(Config::MACHINE_IS_PANDORA && (devName == "gpio-keys" || devName == "keypad"))
	{
		devPtr->setSubtype(Input::DeviceSubtype::PANDORA_HANDHELD);
	}
	addInputDevice(ApplicationContext{static_cast<Application&>(*this)}, std::move(devPtr), notify);
}

constexpr auto xInputDeviceTypeToStr(int type)
{
	switch(type)
	{
		case XCB_INPUT_DEVICE_TYPE_MASTER_POINTER: return "Master Pointer";
		case XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER: return "Slave Pointer";
		case XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD: return "Master Keyboard";
		case XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD: return "Slave Keyboard";
		case XCB_INPUT_DEVICE_TYPE_FLOATING_SLAVE: return "Floating Slave";
		default: return "Unknown";
	}
}

static Input::Key keysymToKey(xcb_keysym_t k)
{
	// if the keysym fits in 2 bytes leave as is,
	// otherwise use only first 15-bits to match
	// definition in Keycode namespace
	return k <= 0xFFFF ? k : k & 0xEFFF;
}

static xkb_state* initXkb(xcb_connection_t& conn)
{
	if(!xkb_x11_setup_xkb_extension(&conn, XKB_X11_MIN_MAJOR_XKB_VERSION, XKB_X11_MIN_MINOR_XKB_VERSION,
		XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS, nullptr, nullptr, nullptr, nullptr))
	{
		log.error("error setting up xkb extension");
		return {};
	}
	auto xkbCtx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbCtx)
	{
		log.error("error getting xkb context");
		return {};
	}
	auto coreKbId = xkb_x11_get_core_keyboard_device_id(&conn);
	if(coreKbId == -1)
	{
		log.error("error getting core keyboard device id");
		return {};
	}
	auto keymap = xkb_x11_keymap_new_from_device(xkbCtx, &conn, coreKbId, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if(!keymap)
	{
		log.error("error getting core keyboard keymap");
		return {};
	}
	auto kbState = xkb_x11_state_new_from_device(keymap, &conn, coreKbId);
	if(!kbState)
	{
		log.error("error creating core keyboard state");
		return {};
	}
	xkb_keymap_unref(keymap);
	xkb_context_unref(xkbCtx);
	return kbState;
}

void XApplication::initInputSystem()
{
	initXInput2();

	// request input device changes events
	{
		XIEventMaskData data;
		data.header.deviceid = XCB_INPUT_DEVICE_ALL;
		data.header.mask_len = sizeof(data.mask) / sizeof(uint32_t);
		data.mask = XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
		auto rootWin = xScr->root;
		xcb_input_xi_select_events(xConn, rootWin, 1, &data.header);
	}

	// setup device list
	vkbDevice = &addInputDevice(ApplicationContext{static_cast<Application&>(*this)},
		std::make_unique<Input::Device>(std::in_place_type<Input::XInputDevice>, Input::virtualDeviceFlags, "Virtual"));
	auto queryDevReply = XCB_REPLY(xcb_input_xi_query_device, xConn, XCB_INPUT_DEVICE_ALL);
	if(queryDevReply)
	{
		auto it = xcb_input_xi_query_device_infos_iterator(queryDevReply.get());
		for (; it.rem; xcb_input_xi_device_info_next(&it))
		{
			xcb_input_xi_device_info_t& d = *it.data;
			if(d.type != XCB_INPUT_DEVICE_TYPE_MASTER_POINTER && d.type != XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD)
				continue;
			log.info("Device {} (id: {}) {} paired to id {}",
				xiDeviceInfoName(d), d.deviceid, xInputDeviceTypeToStr(d.type), d.attachment);
			addXInputDevice({d}, false, d.type == XCB_INPUT_DEVICE_TYPE_MASTER_POINTER);
		}
	}

	kbState = initXkb(*xConn);
	if(!kbState)
	{
		::exit(-1);
	}
}

void XApplication::deinitInputSystem()
{
	if(blankCursor)
		xcb_free_cursor(xConn, blankCursor);
	if(normalCursor)
		xcb_free_cursor(xConn, normalCursor);
	if(kbState)
		xkb_state_unref(kbState);
}

static xcb_window_t eventXWindow(const xcb_ge_generic_event_t& e)
{
	switch(e.event_type)
	{
		case XCB_INPUT_BUTTON_PRESS: return reinterpret_cast<const xcb_input_button_press_event_t&>(e).event;
		case XCB_INPUT_BUTTON_RELEASE: return reinterpret_cast<const xcb_input_button_release_event_t&>(e).event;
		case XCB_INPUT_MOTION: return reinterpret_cast<const xcb_input_motion_event_t&>(e).event;
		case XCB_INPUT_ENTER: return reinterpret_cast<const xcb_input_enter_event_t&>(e).event;
		case XCB_INPUT_LEAVE: return reinterpret_cast<const xcb_input_leave_event_t&>(e).event;
		case XCB_INPUT_FOCUS_IN: return reinterpret_cast<const xcb_input_focus_in_event_t&>(e).event;
		case XCB_INPUT_FOCUS_OUT: return reinterpret_cast<const xcb_input_focus_out_event_t&>(e).event;
		case XCB_INPUT_KEY_PRESS: return reinterpret_cast<const xcb_input_key_press_event_t&>(e).event;
		case XCB_INPUT_KEY_RELEASE: return reinterpret_cast<const xcb_input_key_release_event_t&>(e).event;
	}
	return {};
}

// returns true if event is XI2, false otherwise
bool XApplication::handleXI2GenericEvent(xcb_ge_generic_event_t& event)
{
	if(event.extension != xI2opcode)
	{
		return false;
	}
	// XI_HierarchyChanged isn't window-specific
	if(event.event_type == XCB_INPUT_HIERARCHY)
	{
		//log.debug("input device hierarchy changed");
		auto &ev = reinterpret_cast<xcb_input_hierarchy_event_t&>(event);
		for(auto &info : std::span<xcb_input_hierarchy_info_t>{xcb_input_hierarchy_infos(&ev),
			(size_t)xcb_input_hierarchy_infos_length(&ev)})
		{
			if(info.flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED)
			{
				auto queryDevReply = XCB_REPLY(xcb_input_xi_query_device, xConn, info.deviceid);
				if(!queryDevReply)
					continue;
				auto it = xcb_input_xi_query_device_infos_iterator(queryDevReply.get());
				xcb_input_xi_device_info_t& device = *it.data;
				if(device.type == XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD)
				{
					addXInputDevice(device, true, false);
				}
			}
			else if(info.flags & XCB_INPUT_HIERARCHY_MASK_SLAVE_REMOVED)
			{
				removeInputDeviceIf(ApplicationContext{static_cast<Application&>(*this)}, [&](auto &devPtr){ return hasXInputDeviceId(*devPtr, info.deviceid); }, true);
			}
		}
		return true;
	}
	// others events are for specific windows
	auto destWin = windowForXWindow(eventXWindow(event));
	if(!destWin) [[unlikely]]
	{
		log.warn("ignored event for unknown window");
		return true;
	}
	auto &win = *destWin;
	auto updatePointer =
		[&](const auto &event, Input::Action action)
		{
			auto time = SteadyClockTimePoint{Milliseconds{event.time}}; // X11 timestamps are in ms
			Input::Key key{};
			bool sendKeyEvent{};
			if(action == Input::Action::PUSHED || action == Input::Action::RELEASED)
			{
				if(event.detail == 4)
					action = Input::Action::SCROLL_UP;
				else if(event.detail == 5)
					action = Input::Action::SCROLL_DOWN;
				else
					key = IG::bit(event.detail - 1);
				if(event.detail > 5)
					sendKeyEvent = true;
			}
			else
			{
				if constexpr(requires {xcb_input_button_press_button_mask(event);})
				{
					// mask of currently pressed buttons
					key = makePointerButtonState(event) >> 1;
				}
			}
			auto dev = deviceForInputId(event.sourceid);
			if(sendKeyEvent)
			{
				auto ev = Input::KeyEvent{Input::Map::POINTER, key, action, (uint32_t)event.mods.effective,
					0, Input::Source::MOUSE, time, dev};
				dispatchKeyInputEvent(ev, win);
			}
			else
			{
				auto pos = win.transformInputPos({fixed1616ToFloat(event.event_x), fixed1616ToFloat(event.event_y)});
				Input::PointerId p = event.deviceid;
				win.dispatchInputEvent(Input::MotionEvent{Input::Map::POINTER, (Input::Key)key, (uint32_t)event.mods.effective,
					action, pos.x, pos.y, p, Input::Source::MOUSE, time, dev});
			}
		};
	auto handleKeyEvent =
		[&](const xcb_input_key_press_event_t& event, bool pushed)
		{
			auto destWin = windowForXWindow(event.event);
			if(!destWin) [[unlikely]]
			{
				log.warn("ignored event for unknown window");
				return;
			}
			auto &win = *destWin;
			auto time = SteadyClockTimePoint{Milliseconds{event.time}}; // X11 timestamps are in ms
			auto action = pushed ? Input::Action::PUSHED : Input::Action::RELEASED;
			if(pushed)
				cancelKeyRepeatTimer();
			auto dev = deviceForInputId(event.sourceid);
			auto k = xkb_state_key_get_one_sym(kbState, event.detail);
			bool repeated = event.flags & XCB_INPUT_KEY_EVENT_FLAGS_KEY_REPEAT;
			if(pushed && k == XKB_KEY_Return && (event.mods.effective & (XCB_MOD_MASK_1 | XCB_MOD_MASK_5)) && !repeated)
			{
				win.toggleFullScreen();
			}
			else
			{
				auto key = keysymToKey(k);
				//log.info("KeySym:{}, KeyCode:{}, repeat:{}", k, key, repeated);
				auto ev = Input::KeyEvent{Input::Map::SYSTEM, key, action, (uint32_t)event.mods.effective,
					repeated, Input::Source::KEYBOARD, time, dev};
				ev.setX11RawKey(event.detail);
				dispatchKeyInputEvent(ev, win);
			}
		};
	//log.info("device:{}, event:{}", ievent.deviceid, xIEventTypeToStr(ievent.evtype));
	switch(event.event_type)
	{
		case XCB_INPUT_BUTTON_PRESS:
			updatePointer(reinterpret_cast<xcb_input_button_press_event_t&>(event), Input::Action::PUSHED); break;
		case XCB_INPUT_BUTTON_RELEASE:
			updatePointer(reinterpret_cast<xcb_input_button_press_event_t&>(event), Input::Action::RELEASED); break;
		case XCB_INPUT_MOTION:
			updatePointer(reinterpret_cast<xcb_input_motion_event_t&>(event), Input::Action::MOVED); break;
		case XCB_INPUT_ENTER:
			updatePointer(reinterpret_cast<xcb_input_enter_event_t&>(event), Input::Action::ENTER_VIEW); break;
		case XCB_INPUT_LEAVE:
			updatePointer(reinterpret_cast<xcb_input_leave_event_t&>(event), Input::Action::EXIT_VIEW); break;
		case XCB_INPUT_FOCUS_IN:
			win.dispatchFocusChange(true); break;
		case XCB_INPUT_FOCUS_OUT:
			win.dispatchFocusChange(false);
			deinitKeyRepeatTimer();
			break;
		case XCB_INPUT_KEY_PRESS:
			handleKeyEvent(reinterpret_cast<xcb_input_key_press_event_t&>(event), true); break;
		case XCB_INPUT_KEY_RELEASE:
			handleKeyEvent(reinterpret_cast<xcb_input_key_press_event_t&>(event), false); break;
	}
	return true;
}

std::string XApplication::inputKeyString(Input::Key rawKey, uint32_t modifiers) const
{
	std::array<char, 4> str;
	xkb_state_update_mask(kbState, modifiers, modifiers, modifiers, 0, 0, 0);
	size_t size = xkb_state_key_get_utf8(kbState, rawKey, str.data(), str.size());
	return {str.data(), size};
}

bool XApplication::hasPendingX11Events() const
{
	return xcb_poll_for_queued_event(xConn);
}

}
