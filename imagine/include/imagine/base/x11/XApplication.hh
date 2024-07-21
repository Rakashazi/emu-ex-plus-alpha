#pragma once

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

#include <imagine/config/defs.hh>
#include <imagine/base/linux/LinuxApplication.hh>
#include <imagine/base/EventLoop.hh>
#include <string>
#include <array>

struct xcb_connection_t;
struct xcb_screen_t;
struct xcb_input_xi_device_info_t;
struct xcb_ge_generic_event_t;
struct xkb_state;

namespace Config
{
static constexpr bool XDND = !Config::MACHINE_IS_PANDORA;
}

namespace IG
{

enum class SupportedFrameTimer : uint8_t
{
	SIMPLE,
	#if CONFIG_PACKAGE_LIBDRM
	DRM,
	#endif
	FBDEV
};

class XApplication : public LinuxApplication
{
public:
	using XdndAtoms = std::array<uint32_t, 11>;

	XApplication(ApplicationInitParams);
	~XApplication();
	FDEventSource makeXDisplayConnection(EventLoop);
	xcb_connection_t& xConnection() const { return *xConn; }
	xcb_screen_t& xScreen() const { return *xScr; }

	void emplaceFrameTimer(FrameTimer&, Screen&, bool useVariableTime = {});
	void initPerWindowInputData(uint32_t xWin);
	void runX11Events(xcb_connection_t&);
	void runX11Events();
	bool hasPendingX11Events() const;
	void setXdnd(uint32_t win, bool on);
	std::string inputKeyString(Input::Key rawKey, uint32_t modifiers) const;
	void setWindowCursor(uint32_t xWin, bool on);
	SupportedFrameTimer supportedFrameTimerType() const { return supportedFrameTimer; }

private:
	xcb_connection_t* xConn{};
	xcb_screen_t* xScr{};
	SupportedFrameTimer supportedFrameTimer{};
	FDEventSource xEventSrc;
	XdndAtoms xdndAtom{};

	// Input state
	xkb_state* kbState{};
	Input::Device *vkbDevice{};
	uint32_t blankCursor{};
	uint32_t normalCursor{};
	int xI2opcode{};

	void initXInput2();
	bool eventHandler(xcb_ge_generic_event_t&);
	Window *windowForXWindow(uint32_t xWin) const;
	void initInputSystem();
	void deinitInputSystem();
	bool handleXI2GenericEvent(xcb_ge_generic_event_t&);
	void addXInputDevice(xcb_input_xi_device_info_t&, bool notify, bool isPointingDevice);
	const Input::Device *deviceForInputId(int osId) const;
	static SupportedFrameTimer testFrameTimers();
	bool initXdnd();
	bool xdndIsInit() const;
	void sendDNDFinished(uint32_t win, uint32_t srcWin, uint32_t action);
};

using ApplicationImpl = XApplication;

}
