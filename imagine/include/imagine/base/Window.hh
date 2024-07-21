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
#include <span>
#include <memory>

namespace IG
{

class PixelFormat;
enum class FrameTimeSource : uint8_t;

class Window : public WindowImpl
{
public:
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
	void setFrameEventsOnThisThread();
	void removeFrameEvents();
	static constexpr int8_t drawEventPriorityLocked = 127; // max value passed to setDrawEventPriority() also blocks implicit drawing
	int8_t setDrawEventPriority(int8_t = 0);
	int8_t drawEventPriority() const;
	bool isReady() const { return drawPhase == DrawPhase::READY; }
	DrawPhase activeDrawPhase() const { return drawPhase; }
	void drawNow(bool needsSync = false);
	Screen *screen() const;
	NativeWindow nativeObject() const;
	void setIntendedFrameRate(FrameRate rate);
	void setFormat(NativeWindowFormat);
	void setFormat(PixelFormat);
	PixelFormat pixelFormat() const;
	bool operator ==(Window const &rhs) const;
	bool addOnFrame(OnFrameDelegate, FrameTimeSource src = {}, int priority = 0);
	bool removeOnFrame(OnFrameDelegate, FrameTimeSource src = {});
	bool moveOnFrame(Window &srcWin, OnFrameDelegate, FrameTimeSource src = {});
	FrameTimeSource defaultFrameTimeSource() const;
	FrameTimeSource evalFrameTimeSource(FrameTimeSource) const;
	void configureFrameTimeSource(FrameTimeSource);
	void resetAppData();
	void resetRendererData();
	bool isMainWindow() const;
	ApplicationContext appContext() const;
	Application &application() const;
	void setCursorVisible(bool);
	void setSystemGestureExclusionRects(std::span<const WRect>);
	void setDecorations(bool);
	void setPosition(WPt pos);
	void setSize(WSize size);
	void toggleFullScreen();

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

	int realWidth() const;
	int realHeight() const;
	int width() const;
	int height() const;
	WSize realSize() const;
	WSize size() const;
	bool isPortrait() const;
	bool isLandscape() const;
	F2Size sizeMM() const;
	F2Size sizeScaledMM() const;
	int widthMMInPixels(float mm) const;
	int heightMMInPixels(float mm) const;
	int widthScaledMMInPixels(float mm) const;
	int heightScaledMMInPixels(float mm) const;
	WRect bounds() const;
	F2Pt transformInputPos(F2Pt srcPos) const;
	Viewport viewport(WindowRect rect) const;
	Viewport viewport() const;

	// content in these bounds isn't blocked by system overlays and receives pointer input
	WRect contentBounds() const;

	Rotation softOrientation() const;
	bool requestOrientationChange(Rotation o);
	bool setValidOrientations(Orientations);

	bool updateSize(WSize surfaceSize);
	bool updatePhysicalSize(F2Size surfaceSizeMM);
	bool updatePhysicalSize(F2Size surfaceSizeMM, F2Size surfaceSizeSMM);
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
	void signalSurfaceChanged(WindowSurfaceChangeFlags);

private:
	F2Size pixelSizeAsMM(WSize size);
	F2Size pixelSizeAsScaledMM(WSize size);
	void draw(bool needsSync = false);
};

}
