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

#define LOGTAG "Window"
#include <imagine/base/Base.hh>
#include <imagine/base/Screen.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>
#include "windowPrivate.hh"
#include <imagine/input/Input.hh>

namespace Base
{

#ifdef CONFIG_BASE_MULTI_WINDOW
std::vector<Window*> window_;
#else
Window *mainWin = nullptr;
#endif

void BaseWindow::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	onSurfaceChange = del ? del : [](Window &, SurfaceChange){};
}

void BaseWindow::setOnDraw(DrawDelegate del)
{
	onDraw = del ? del : [](Window &, DrawParams){ return true; };
}

void BaseWindow::setOnFocusChange(FocusChangeDelegate del)
{
	onFocusChange = del ? del : [](Window &, bool){};
}

void BaseWindow::setOnDragDrop(DragDropDelegate del)
{
	onDragDrop = del ? del : [](Window &, const char *){};
}

void BaseWindow::setOnInputEvent(InputEventDelegate del)
{
	onInputEvent = del ? del : [](Window &, Input::Event ){ return false; };
}

void BaseWindow::setOnDismissRequest(DismissRequestDelegate del)
{
	onDismissRequest = del ? del : [](Window &win){ Base::exit(); };
}

void BaseWindow::setOnDismiss(DismissDelegate del)
{
	onDismiss = del ? del : [](Window &win){};
}

void BaseWindow::setOnFree(FreeDelegate del)
{
	onFree = del ? del : [](){};
}

void BaseWindow::initDelegates(const WindowConfig &config)
{
	setOnSurfaceChange(config.onSurfaceChange());
	setOnDraw(config.onDraw());
	setOnFocusChange(config.onFocusChange());
	setOnDragDrop(config.onDragDrop());
	setOnInputEvent(config.onInputEvent());
	setOnDismissRequest(config.onDismissRequest());
	setOnDismiss(config.onDismiss());
	setOnFree(config.onFree());
	onExit =
		[this](bool backgrounded)
		{
			notifyDrawAllowed = false;
			drawEvent.cancel();
			return true;
		};
	Base::addOnExit(onExit, WINDOW_ON_EXIT_PRIORITY);
	onResume =
		[this](bool)
		{
			// allow drawing and trigger the draw event if this window was posted since app was suspended
			static_cast<Window*>(this)->deferredDrawComplete();
			return true;
		};
	Base::addOnResume(onResume, WINDOW_ON_RESUME_PRIORITY);
	drawEvent.attach(
		[this]()
		{
			//logDMsg("running window draw event");
			static_cast<Window*>(this)->dispatchOnDraw();
		});
}

void BaseWindow::initDefaultValidSoftOrientations()
{
	#ifdef CONFIG_GFX_SOFT_ORIENTATION
	validSoftOrientations_ = defaultSystemOrientations();
	#endif
}

void BaseWindow::init(const WindowConfig &config)
{
	initDelegates(config);
	initDefaultValidSoftOrientations();
	customDataPtr = config.customData();
}

void Window::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	BaseWindow::setOnSurfaceChange(del);
}

void Window::setOnDraw(DrawDelegate del)
{
	BaseWindow::setOnDraw(del);
}

void Window::setOnFocusChange(FocusChangeDelegate del)
{
	BaseWindow::setOnFocusChange(del);
}

void Window::setOnDragDrop(DragDropDelegate del)
{
	BaseWindow::setOnDragDrop(del);
}

void Window::setOnInputEvent(InputEventDelegate del)
{
	BaseWindow::setOnInputEvent(del);
}

void Window::setOnDismissRequest(DismissRequestDelegate del)
{
	BaseWindow::setOnDismissRequest(del);
}

void Window::setOnDismiss(DismissDelegate del)
{
	BaseWindow::setOnDismiss(del);
}

Window &mainWindow()
{
	assert(Window::windows());
	return *Window::window(0);
}

Screen *Window::screen() const
{
	#ifdef CONFIG_BASE_MULTI_SCREEN
	return screen_;
	#else
	return &mainScreen();
	#endif
}

bool Window::setNeedsDraw(bool needsDraw)
{
	if(needsDraw)
	{
		if(unlikely(!hasSurface()))
		{
			drawNeeded = false;
			return false;
		}
		drawNeeded = true;
		return true;
	}
	else
	{
		drawNeeded = false;
		return false;
	}
}

bool Window::needsDraw() const
{
	return drawNeeded;
}

void Window::postDraw()
{
	if(!setNeedsDraw(true))
		return;
	if(!notifyDrawAllowed)
	{
		//logDMsg("posted draw but event notification disabled");
		return;
	}
	drawEvent.notify();
	//logDMsg("window:%p needs draw", this);
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
	drawEvent.cancel();
	//logDMsg("window:%p cancelled draw", this);
}

void Window::deferredDrawComplete()
{
	notifyDrawAllowed = true;
	if(drawNeeded)
	{
		//logDMsg("draw event after deferred draw complete");
		drawEvent.notify();
	}
}

void Window::drawNow(bool needsSync)
{
	draw(needsSync);
}

void Window::setNeedsCustomViewportResize(bool needsResize)
{
	if(needsResize)
	{
		surfaceChange.addCustomViewportResized();
	}
	else
	{
		surfaceChange.removeCustomViewportResized();
	}
}

bool Window::dispatchInputEvent(Input::Event event)
{
	return onInputEvent.callCopy(*this, event);
}

void Window::dispatchFocusChange(bool in)
{
	onFocusChange.callCopy(*this, in);
}

void Window::dispatchDragDrop(const char *filename)
{
	onDragDrop.callCopy(*this, filename);
}

void Window::dispatchDismissRequest()
{
	onDismissRequest.callCopy(*this);
}

void Window::dispatchSurfaceChange()
{
	onSurfaceChange.callCopy(*this, std::exchange(surfaceChange, {}));
}

void Window::dispatchOnDraw(bool needsSync)
{
	if(!needsDraw())
		return;
	drawNeeded = false;
	draw(needsSync);
}

void Window::draw(bool needsSync)
{
	DrawParams params;
	params.needsSync_ = needsSync;
	if(unlikely(surfaceChange.flags))
	{
		dispatchSurfaceChange();
		params.wasResized_ = true;
	}
	notifyDrawAllowed = false;
	if(onDraw.callCopy(*this, params))
	{
		deferredDrawComplete();
	}
}

bool Window::updateSize(IG::Point2D<int> surfaceSize)
{
	auto oldW = w, oldH = h;
	w = surfaceSize.x;
	h = surfaceSize.y;
	if(orientationIsSideways(softOrientation_))
		std::swap(w, h);
	if(oldW == w && oldH == h) // is the new size the same as the old?
	{
		logMsg("same window size %d,%d", realWidth(), realHeight());
		return false;
	}
	#ifdef __ANDROID__
	updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}), pixelSizeAsSMM({realWidth(), realHeight()}));
	#else
	updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}));
	#endif
	surfaceChange.addSurfaceResized();
	return true;
}

bool Window::updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM)
{
	bool changed = false;
	auto oldW = wMM, oldH = hMM;
	wMM = surfaceSizeMM.x;
	hMM = surfaceSizeMM.y;
	if(orientationIsSideways(softOrientation_))
		std::swap(wMM, hMM);
	mmToPixelXScaler = w / wMM;
	mmToPixelYScaler = h / hMM;
	if(oldW != wMM || oldH != hMM)
		changed = true;
	#ifdef __ANDROID__
	assert(surfaceSizeSMM.x && surfaceSizeSMM.y);
	auto oldSW = wSMM, oldSH = hSMM;
	wSMM = surfaceSizeSMM.x;
	hSMM = surfaceSizeSMM.y;
	if(orientationIsSideways(softOrientation_))
		std::swap(wSMM, hSMM);
	smmToPixelXScaler = w / wSMM;
	smmToPixelYScaler = h / hSMM;
	if(oldSW != wSMM || oldSH != hSMM)
		changed = true;
	#endif
	if(softOrientation_ == VIEW_ROTATE_0)
	{
		logMsg("updated window size:%dx%d (%.2fx%.2fmm, scaled %.2fx%.2fmm)",
			width(), height(), widthMM(), heightMM(), widthSMM(), heightSMM());
	}
	else
	{
		logMsg("updated window size:%dx%d (%.2fx%.2fmm, scaled %.2fx%.2fmm) with rotation, real size:%dx%d",
			width(), height(), widthMM(), heightMM(), widthSMM(), heightSMM(), realWidth(), realHeight());
	}
	return changed;
}

bool Window::updatePhysicalSize(IG::Point2D<float> surfaceSizeMM)
{
	return updatePhysicalSize(surfaceSizeMM, {0., 0.});
}

bool Window::updatePhysicalSizeWithCurrentSize()
{
	#ifdef __ANDROID__
	return updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}), pixelSizeAsSMM({realWidth(), realHeight()}));
	#else
	return updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}));
	#endif
}

#ifdef CONFIG_GFX_SOFT_ORIENTATION
bool Window::setValidOrientations(Orientation oMask)
{
	oMask = validateOrientationMask(oMask);
	validSoftOrientations_ = oMask;
	if(validSoftOrientations_ & setSoftOrientation)
		return requestOrientationChange(setSoftOrientation);
	if(!(validSoftOrientations_ & softOrientation_))
	{
		if(validSoftOrientations_ & VIEW_ROTATE_0)
			return requestOrientationChange(VIEW_ROTATE_0);
		else if(validSoftOrientations_ & VIEW_ROTATE_90)
			return requestOrientationChange(VIEW_ROTATE_90);
		else if(validSoftOrientations_ & VIEW_ROTATE_180)
			return requestOrientationChange(VIEW_ROTATE_180);
		else if(validSoftOrientations_ & VIEW_ROTATE_270)
			return requestOrientationChange(VIEW_ROTATE_270);
		else
		{
			bug_unreachable("bad orientation mask: 0x%X", oMask);
		}
	}
	return false;
}

bool Window::requestOrientationChange(Orientation o)
{
	assert(o == VIEW_ROTATE_0 || o == VIEW_ROTATE_90 || o == VIEW_ROTATE_180 || o == VIEW_ROTATE_270);
	setSoftOrientation = o;
	if((validSoftOrientations_ & o) && softOrientation_ != o)
	{
		logMsg("setting orientation %s", orientationToStr(o));
		int savedRealWidth = realWidth();
		int savedRealHeight = realHeight();
		softOrientation_ = o;
		updateSize({savedRealWidth, savedRealHeight});
		postDraw();
		if(*this == mainWindow())
			setSystemOrientation(o);
		Input::configureInputForOrientation(*this);
		return true;
	}
	return false;
}
#endif

Orientation Window::softOrientation() const
{
	return softOrientation_;
}

Orientation Window::validSoftOrientations() const
{
	return validSoftOrientations_;
}

uint32_t Window::windows()
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	return window_.size();
	#else
	return mainWin ? 1 : 0;
	#endif
}

Window *Window::window(uint32_t idx)
{
	#ifdef CONFIG_BASE_MULTI_WINDOW
	if(idx >= window_.size())
		return nullptr;
	return window_[idx];
	#else
	return mainWin;
	#endif
}

void Window::dismiss()
{
	onDismiss(*this);
	Base::removeOnExit(onExit);
	Base::removeOnResume(onResume);
	drawEvent.detach();
	auto onFree = this->onFree;
	deinit();
	#ifdef CONFIG_BASE_MULTI_WINDOW
	IG::eraseFirst(window_, this);
	#else
	mainWin = nullptr;
	#endif
	onFree();
}

void Window::setCustomData(void *data)
{
	customDataPtr = data;
}

int Window::realWidth() const { return orientationIsSideways(softOrientation()) ? h : w; }

int Window::realHeight() const { return orientationIsSideways(softOrientation()) ? w : h; }

int Window::width() const { return w; }

int Window::height() const { return h; }

IG::Point2D<int> Window::realSize() const { return {realWidth(), realHeight()}; }

IG::Point2D<int> Window::size() const { return {width(), height()}; }

bool Window::isPortrait() const
{
	return width() < height();
}

bool Window::isLandscape() const
{
	return !isPortrait();
}

float Window::widthMM() const
{
	assert(wMM);
	return wMM;
}

float Window::heightMM() const
{
	assert(hMM);
	return hMM;
}

#ifdef __ANDROID__
float Window::widthSMM() const
{
	assert(wSMM);
	return wSMM;
}

float Window::heightSMM() const
{
	assert(hSMM);
	return hSMM;
}
#else
float Window::widthSMM() const { return widthMM(); }
float Window::heightSMM() const { return heightMM(); }
#endif

int Window::widthMMInPixels(float mm) const
{
	return std::round(mm * (mmToPixelXScaler));
}

int Window::heightMMInPixels(float mm) const
{
	return std::round(mm * (mmToPixelYScaler));
}

#ifdef __ANDROID__
int Window::widthSMMInPixels(float mm) const
{
	return std::round(mm * (smmToPixelXScaler));
}
int Window::heightSMMInPixels(float mm) const
{
	return std::round(mm * (smmToPixelYScaler));
}
#else
int Window::widthSMMInPixels(float mm) const { return widthMMInPixels(mm); }
int Window::heightSMMInPixels(float mm) const { return heightMMInPixels(mm); }
#endif

IG::WindowRect Window::bounds() const
{
	return {0, 0, width(), height()};
}

Screen &WindowConfig::screen() const
{
	return screen_ ? *screen_ : *Screen::screen(0);
}

}
