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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/WindowConfig.hh>
#include <imagine/base/Viewport.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/utility.h>

namespace IG::Input
{
class Event;
class KeyEvent;
}

namespace IG
{

class Screen;
class ApplicationContext;
class Application;
class PixelFormat;

class Window : public WindowImpl
{
public:
	using FrameTimeSource = WindowFrameTimeSource;

	Window(ApplicationContext, WindowConfig, InitDelegate);
	void show();
	void dismiss();
	void setAcceptDnd(bool on);
	void setTitle(const char *name);
	bool setNeedsDraw(bool needsDraw);
	bool needsDraw() const;
	void postDraw(int8_t priority = 0);
	void unpostDraw();
	void postFrameReady();
	void postDrawToMainThread(int8_t priority = 0);
	void postFrameReadyToMainThread();
	int8_t setDrawEventPriority(int8_t = 0);
	int8_t drawEventPriority() const;
	void drawNow(bool needsSync = false);
	Screen *screen() const;
	NativeWindow nativeObject() const;
	void setIntendedFrameRate(double rate);
	void setFormat(NativeWindowFormat);
	void setFormat(IG::PixelFormat);
	IG::PixelFormat pixelFormat() const;
	bool operator ==(Window const &rhs) const;
	bool addOnFrame(OnFrameDelegate del, FrameTimeSource src = {}, int priority = 0);
	bool removeOnFrame(OnFrameDelegate del, FrameTimeSource src = {});
	bool moveOnFrame(Window &srcWin, OnFrameDelegate, FrameTimeSource src = {});
	void resetAppData();
	void resetRendererData();
	bool isMainWindow() const;
	ApplicationContext appContext() const;
	Application &application() const;
	void setCursorVisible(bool);
	void setSystemGestureExclusionRects(std::span<const WRect>);

	template <class T>
	T &makeAppData(auto &&... args)
	{
		appDataPtr = std::make_shared<T>(IG_forward(args)...);
		return *appData<T>();
	}

	template<class T>
	T *appData() const
	{
		return static_cast<T*>(appDataPtr.get());
	}

	template <class T>
	T &makeRendererData(auto &&... args)
	{
		rendererDataPtr = std::make_shared<T>(IG_forward(args)...);
		return *rendererData<T>();
	}

	template<class T>
	T *rendererData() const
	{
		return static_cast<T*>(rendererDataPtr.get());
	}

	// Called when the state of the window's drawing surface changes,
	// such as a re-size or if it becomes the current drawing target
	void setOnSurfaceChange(SurfaceChangeDelegate del);
	// Called during a Screen frame callback if the window needs to be drawn
	void setOnDraw(DrawDelegate del);
	// Called to process an event from an input device
	void setOnInputEvent(InputEventDelegate del);
	// Called when app window enters/exits focus
	void setOnFocusChange(FocusChangeDelegate del);
	// Called when a file is dropped into into the app's window
	// if app enables setAcceptDnd()
	void setOnDragDrop(DragDropDelegate del);
	// Called when the user performs an action indicating to
	// to the window manager they wish to dismiss the window
	// (clicking the close button for example),
	// by default it will exit the app
	void setOnDismissRequest(DismissRequestDelegate del);
	// Called when the window is dismissed
	void setOnDismiss(DismissDelegate del);

	int realWidth() const;
	int realHeight() const;
	int width() const;
	int height() const;
	IG::Point2D<int> realSize() const;
	IG::Point2D<int> size() const;
	bool isPortrait() const;
	bool isLandscape() const;
	IG::Point2D<float> sizeMM() const;
	IG::Point2D<float> sizeScaledMM() const;
	int widthMMInPixels(float mm) const;
	int heightMMInPixels(float mm) const;
	int widthScaledMMInPixels(float mm) const;
	int heightScaledMMInPixels(float mm) const;
	IG::WindowRect bounds() const;
	IG::Point2D<int> transformInputPos(IG::Point2D<int> srcPos) const;
	Viewport viewport(WindowRect rect) const;
	Viewport viewport() const;

	// content in these bounds isn't blocked by system overlays and receives pointer input
	IG::WindowRect contentBounds() const;

	Rotation softOrientation() const;
	bool requestOrientationChange(Rotation o);
	bool setValidOrientations(OrientationMask);

	bool updateSize(IG::Point2D<int> surfaceSize);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM);
	bool updatePhysicalSizeWithCurrentSize();
	bool hasSurface() const;
	bool dispatchInputEvent(Input::Event event);
	bool dispatchRepeatableKeyInputEvent(Input::KeyEvent event);
	void dispatchFocusChange(bool in);
	void dispatchDragDrop(const char *filename);
	void dispatchDismissRequest();
	void dispatchOnDraw(bool needsSync = false);
	void dispatchOnFrame();
	void dispatchSurfaceCreated();
	void dispatchSurfaceChanged();
	void dispatchSurfaceDestroyed();
	void signalSurfaceChanged(uint8_t surfaceChangeFlags);

private:
	IG::Point2D<float> pixelSizeAsMM(IG::Point2D<int> size);
	IG::Point2D<float> pixelSizeAsScaledMM(IG::Point2D<int> size);
	void draw(bool needsSync = false);
};

}
