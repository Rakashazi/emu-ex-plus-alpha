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
#include <imagine/base/FrameTimer.hh>
#include <imagine/base/EventLoop.hh>
#include <imagine/input/Device.hh>
#include <memory>

struct _XDisplay;
union _XEvent;

namespace Config
{
	namespace Base
	{
	static constexpr bool XDND = !Config::MACHINE_IS_PANDORA;
	}
}

namespace Base
{

class Screen;
class Window;
class FrameTimer;
class FDEventSource;
class XIDeviceInfo;
class XkbDescRec;

enum class SupportedFrameTimer : uint8_t
{
	SIMPLE, DRM, FBDEV
};

struct XInputDevice : public Input::Device
{
	int id = -1;
	bool iCadeMode_ = false;

	XInputDevice();
	XInputDevice(uint32_t typeBits, const char *name);
	XInputDevice(XIDeviceInfo, int enumId, bool isPointingDevice, bool isPowerButton);
	void setICadeMode(bool on) final;
	bool iCadeMode() const final;
};

class XApplication : public LinuxApplication
{
public:
	using XdndAtoms = std::array<unsigned long, 11>;

	XApplication(ApplicationInitParams);
	~XApplication();
	FDEventSource makeXDisplayConnection(EventLoop);
	::_XDisplay *xDisplay() const;
	FrameTimer makeFrameTimer(Screen &);
	void initPerWindowInputData(unsigned long xWin);
	void runX11Events(_XDisplay *);
	void runX11Events();
	void setXdnd(unsigned long win, bool on);
	Input::EventKeyString inputKeyString(Input::Key rawKey, uint32_t modifiers) const;
	void setWindowCursor(unsigned long xWin, bool on);

private:
	::_XDisplay *dpy{};
	FDEventSource xEventSrc{};
	SupportedFrameTimer supportedFrameTimer{};
	XdndAtoms xdndAtom{};

	// Input state
	std::vector<std::unique_ptr<XInputDevice>> xDevice{};
	XkbDescRec *coreKeyboardDesc{};
	Input::Device *vkbDevice{};
	unsigned long blankCursor{};
	unsigned long normalCursor{};
	unsigned numCursors{};
	int xI2opcode{};
	int xPointerMapping[Config::Input::MAX_POINTERS]{};

	void initXInput2();
	bool eventHandler(_XEvent);
	Window *windowForXWindow(unsigned long xWin) const;
	void initInputSystem();
	void deinitInputSystem();
	bool handleXI2GenericEvent(_XEvent);
	void addXInputDevice(XIDeviceInfo, bool notify, bool isPointingDevice);
	void removeXInputDevice(int xDeviceId);
	const Input::Device *deviceForInputId(int osId) const;
	int devIdToPointer(int id) const;
	static SupportedFrameTimer testFrameTimers();
	bool initXdnd();
	bool xdndIsInit() const;
	void sendDNDFinished(unsigned long win, unsigned long srcWin, unsigned long action);
};

using ApplicationImpl = XApplication;

}
