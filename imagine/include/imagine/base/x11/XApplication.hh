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

struct _XDisplay;
union _XEvent;
struct _XIM;
struct _XIC;

namespace Config
{
static constexpr bool XDND = !Config::MACHINE_IS_PANDORA;
}

namespace IG
{

struct XIDeviceInfo;

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
	using XdndAtoms = std::array<unsigned long, 11>;

	XApplication(ApplicationInitParams);
	~XApplication();
	FDEventSource makeXDisplayConnection(EventLoop);
	::_XDisplay *xDisplay() const;
	void emplaceFrameTimer(FrameTimer&, Screen&, bool useVariableTime = {});
	void initPerWindowInputData(unsigned long xWin);
	void runX11Events(_XDisplay *);
	void runX11Events();
	bool hasPendingX11Events() const;
	void setXdnd(unsigned long win, bool on);
	std::string inputKeyString(Input::Key rawKey, uint32_t modifiers) const;
	void setWindowCursor(unsigned long xWin, bool on);
	SupportedFrameTimer supportedFrameTimerType() const { return supportedFrameTimer; }

private:
	::_XDisplay *dpy{};
	FDEventSource xEventSrc{};
	SupportedFrameTimer supportedFrameTimer{};
	XdndAtoms xdndAtom{};

	// Input state
	_XIM *im{};
	_XIC *ic{};
	Input::Device *vkbDevice{};
	unsigned long blankCursor{};
	unsigned long normalCursor{};
	int xI2opcode{};

	void initXInput2();
	bool eventHandler(_XEvent);
	Window *windowForXWindow(unsigned long xWin) const;
	void initInputSystem();
	void deinitInputSystem();
	bool handleXI2GenericEvent(_XEvent);
	void addXInputDevice(XIDeviceInfo, bool notify, bool isPointingDevice);
	const Input::Device *deviceForInputId(int osId) const;
	static SupportedFrameTimer testFrameTimers();
	bool initXdnd();
	bool xdndIsInit() const;
	void sendDNDFinished(unsigned long win, unsigned long srcWin, unsigned long action);
};

using ApplicationImpl = XApplication;

}
