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
#include <imagine/base/baseDefs.hh>
#include <imagine/base/WindowConfig.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>
#include <imagine/util/bits.h>
#include <imagine/input/Input.hh>
#include <imagine/base/Error.hh>

namespace Base
{

class Screen;

class Window : public WindowImpl
{
public:
	constexpr Window() {}

	IG::ErrorCode init(const WindowConfig &config);
	void show();
	void dismiss();
	void setAcceptDnd(bool on);
	void setTitle(const char *name);
	bool setNeedsDraw(bool needsDraw);
	void setNeedsCustomViewportResize(bool needsResize);
	bool needsDraw() const;
	void postDraw();
	void unpostDraw();
	void deferredDrawComplete();
	void drawNow(bool needsSync = false);
	void dispatchOnDraw(bool needsSync = false);
	Screen *screen() const;
	static uint32_t windows();
	static Window *window(uint32_t idx);
	static PixelFormat defaultPixelFormat();
	NativeWindow nativeObject() const;
	void setCustomData(void *data);
	template<class T>
	T *customData() const
	{
		return static_cast<T*>(customDataPtr);
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
	float widthMM() const;
	float heightMM() const;
	float widthSMM() const;
	float heightSMM() const;
	int widthMMInPixels(float mm) const;
	int heightMMInPixels(float mm) const;
	int widthSMMInPixels(float mm) const;
	int heightSMMInPixels(float mm) const;
	IG::WindowRect bounds() const;

	// content in these bounds isn't blocked by system overlays and receives pointer input
	IG::WindowRect contentBounds() const;

	Orientation softOrientation() const;
	Orientation validSoftOrientations() const;
	bool requestOrientationChange(Orientation o);
	bool setValidOrientations(Orientation oMask);
	static bool systemAnimatesRotation();

	bool updateSize(IG::Point2D<int> surfaceSize);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM);
	bool updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM);
	bool updatePhysicalSizeWithCurrentSize();
	bool hasSurface() const;
	bool dispatchInputEvent(Input::Event event);
	void dispatchFocusChange(bool in);
	void dispatchDragDrop(const char *filename);
	void dispatchDismissRequest();
	void deinit();

private:
	IG::Point2D<float> pixelSizeAsMM(IG::Point2D<int> size);
	IG::Point2D<float> pixelSizeAsSMM(IG::Point2D<int> size);
	void dispatchSurfaceChange();
	void draw(bool needsSync = false);
};

Window &mainWindow();
Screen &mainScreen();

}
