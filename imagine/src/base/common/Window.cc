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
#include <imagine/base/ApplicationContext.hh>
#include <imagine/base/Application.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include <imagine/input/Input.hh>
#include <imagine/util/algorithm.h>
#include <imagine/logger/logger.h>

namespace Base
{

static constexpr uint8_t MAX_DRAW_EVENT_PRIORITY = 0xFF;

static auto defaultOnSurfaceChange = [](Window &, Window::SurfaceChange){};
static auto defaultOnDraw = [](Window &, Window::DrawParams){ return true; };
static auto defaultOnFocusChange = [](Window &, bool){};
static auto defaultOnDragDrop = [](Window &, const char *){};
static auto defaultOnInputEvent = [](Window &, Input::Event ){ return false; };
static auto defaultOnDismissRequest = [](Window &win){ win.appContext().exit(); };
static auto defaultOnDismiss = [](Window &){};

BaseWindow::BaseWindow(ApplicationContext ctx, WindowConfig config):
	onExit
	{
		[this](ApplicationContext ctx, bool backgrounded)
		{
			auto &win = *static_cast<Window*>(this);
			auto savedDrawEventPriority = win.setDrawEventPriority(MAX_DRAW_EVENT_PRIORITY);
			drawEvent.cancel();
			drawEvent.detach();
			if(backgrounded)
			{
				ctx.addOnResume(
					[this, savedDrawEventPriority](ApplicationContext, bool)
					{
						auto &win = *static_cast<Window*>(this);
						win.setDrawEventPriority(savedDrawEventPriority);
						attachDrawEvent();
						return false;
					}, WINDOW_ON_RESUME_PRIORITY
				);
			}
			return true;
		}, ctx, WINDOW_ON_EXIT_PRIORITY},
	onSurfaceChange{config.onSurfaceChange() ? config.onSurfaceChange() : defaultOnSurfaceChange},
	onDraw{config.onDraw() ? config.onDraw() : defaultOnDraw},
	onInputEvent{config.onInputEvent() ? config.onInputEvent() : defaultOnInputEvent},
	onFocusChange{config.onFocusChange() ? config.onFocusChange() : defaultOnFocusChange},
	onDragDrop{config.onDragDrop() ? config.onDragDrop() : defaultOnDragDrop},
	onDismissRequest{config.onDismissRequest() ? config.onDismissRequest() : defaultOnDismissRequest},
	onDismiss{config.onDismiss() ? config.onDismiss() : defaultOnDismiss},
	validSoftOrientations_{ctx.defaultSystemOrientations()}
{
	attachDrawEvent();
}

void BaseWindow::setOnSurfaceChange(SurfaceChangeDelegate del)
{
	onSurfaceChange = del ? del : defaultOnSurfaceChange;
}

void BaseWindow::setOnDraw(DrawDelegate del)
{
	onDraw = del ? del : defaultOnDraw;
}

void BaseWindow::setOnFocusChange(FocusChangeDelegate del)
{
	onFocusChange = del ? del : defaultOnFocusChange;
}

void BaseWindow::setOnDragDrop(DragDropDelegate del)
{
	onDragDrop = del ? del : defaultOnDragDrop;
}

void BaseWindow::setOnInputEvent(InputEventDelegate del)
{
	onInputEvent = del ? del : defaultOnInputEvent;
}

void BaseWindow::setOnDismissRequest(DismissRequestDelegate del)
{
	onDismissRequest = del ? del : defaultOnDismissRequest;
}

void BaseWindow::setOnDismiss(DismissDelegate del)
{
	onDismiss = del ? del : defaultOnDismiss;
}

void BaseWindow::attachDrawEvent()
{
	drawEvent.attach(
		[&win = *static_cast<Window*>(this)]()
		{
			//logDMsg("running window events");
			win.dispatchOnFrame();
			win.dispatchOnDraw();
		});
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

static Window::FrameTimeSource frameClock(const Screen &screen, Window::FrameTimeSource clock)
{
	if(clock == Window::FrameTimeSource::AUTO)
		return screen.supportsTimestamps() ? Window::FrameTimeSource::SCREEN : Window::FrameTimeSource::RENDERER;
	return clock;
}

bool Window::addOnFrame(Base::OnFrameDelegate del, FrameTimeSource clock, int priority)
{
	clock = frameClock(*screen(), clock);
	if(clock == FrameTimeSource::SCREEN)
	{
		return screen()->addOnFrame(del);
	}
	else
	{
		bool added = onFrame.add(del, priority);
		drawEvent.notify();
		return added;
	}
}

bool Window::removeOnFrame(Base::OnFrameDelegate del, FrameTimeSource clock)
{
	clock = frameClock(*screen(), clock);
	if(clock == FrameTimeSource::SCREEN)
	{
		return screen()->removeOnFrame(del);
	}
	else
	{
		return onFrame.remove(del);
	}
}

void Window::resetAppData()
{
	appDataPtr.reset();
}

void Window::resetRendererData()
{
	rendererDataPtr.reset();
}

bool Window::isMainWindow() const
{
	return *this == appContext().mainWindow();
}

Screen *Window::screen() const
{
	if constexpr(Config::BASE_MULTI_SCREEN)
		return screen_;
	else
		return &appContext().mainScreen();
}

bool Window::setNeedsDraw(bool needsDraw)
{
	if(needsDraw)
	{
		if(!hasSurface()) [[unlikely]]
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

void Window::postDraw(uint8_t priority)
{
	if(priority < drawEventPriority())
	{
		logDMsg("skipped posting draw with priority:%u < %u", priority, drawEventPriority());
		return;
	}
	if(!setNeedsDraw(true))
		return;
	if(drawPhase != DrawPhase::DRAW)
		drawEvent.notify();
	//logDMsg("window:%p needs draw", this);
}

void Window::unpostDraw()
{
	setNeedsDraw(false);
	drawEvent.cancel();
	//logDMsg("window:%p cancelled draw", this);
}

void Window::postFrameReady()
{
	drawPhase = DrawPhase::READY;
	if(onFrame.size() || needsDraw())
		drawEvent.notify();
}

void Window::postDrawToMainThread(uint8_t priority)
{
	appContext().runOnMainThread(
		[this, priority](ApplicationContext)
		{
			postDraw(priority);
		});
}

void Window::postFrameReadyToMainThread()
{
	postFrameReady();
}

uint8_t Window::setDrawEventPriority(uint8_t priority)
{
	return std::exchange(drawEventPriority_, priority);
}

uint8_t Window::drawEventPriority() const
{
	return drawEventPriority_;
}

void Window::drawNow(bool needsSync)
{
	draw(needsSync);
}

void Window::setNeedsCustomViewportResize(bool needsResize)
{
	surfaceChangeFlags = setOrClearBits(surfaceChangeFlags, SurfaceChange::CUSTOM_VIEWPORT_RESIZED, needsResize);
}

bool Window::dispatchInputEvent(Input::Event event)
{
	return onInputEvent.callCopy(*this, event);
}

bool Window::dispatchRepeatableKeyInputEvent(Input::Event event)
{
	assert(event.isKey());
	application().startKeyRepeatTimer(event);
	return dispatchInputEvent(event);
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

void Window::dispatchSurfaceCreated()
{
	onSurfaceChange.callCopy(*this, {SurfaceChange::Action::CREATED});
	surfaceChangeFlags |= SurfaceChange::SURFACE_RESIZED;
}

void Window::dispatchSurfaceChanged()
{
	onSurfaceChange.callCopy(*this, {SurfaceChange::Action::CHANGED, std::exchange(surfaceChangeFlags, {})});
}

void Window::dispatchSurfaceDestroyed()
{
	surfaceChangeFlags = 0;
	unpostDraw();
	onSurfaceChange.callCopy(*this, {SurfaceChange::Action::DESTROYED});
}

void Window::dispatchOnDraw(bool needsSync)
{
	if(!needsDraw())
		return;
	draw(needsSync);
}

void Window::dispatchOnFrame()
{
	if(drawPhase != DrawPhase::READY)
	{
		return;
	}
	drawPhase = DrawPhase::UPDATE;
	if(!onFrame.size())
	{
		return;
	}
	//logDMsg("running %u onFrame delegates", onFrame.size());
	auto now = IG::steadyClockTimestamp();
	FrameParams frameParams{now, screen()->frameTime()};
	onFrame.runAll([&](Base::OnFrameDelegate del){ return del(frameParams); });
	if(onFrame.size())
	{
		setNeedsDraw(true);
	}
}

void Window::draw(bool needsSync)
{
	DrawParams params;
	params.needsSync_ = needsSync;
	if(surfaceChangeFlags) [[unlikely]]
	{
		dispatchSurfaceChanged();
		params.wasResized_ = true;
	}
	drawNeeded = false;
	drawPhase = DrawPhase::DRAW;
	if(onDraw.callCopy(*this, params))
	{
		postFrameReady();
	}
}

bool Window::updateSize(IG::Point2D<int> surfaceSize)
{
	if(orientationIsSideways(softOrientation_))
		std::swap(surfaceSize.x, surfaceSize.y);
	auto oldSize = std::exchange(winSizePixels, surfaceSize);
	if(oldSize == winSizePixels)
	{
		logMsg("same window size %d,%d", realWidth(), realHeight());
		return false;
	}
	if constexpr(Config::envIsAndroid)
	{
		updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}), pixelSizeAsSMM({realWidth(), realHeight()}));
	}
	else
	{
		updatePhysicalSize(pixelSizeAsMM({realWidth(), realHeight()}));
	}
	surfaceChangeFlags |= SurfaceChange::SURFACE_RESIZED;
	return true;
}

bool Window::updatePhysicalSize(IG::Point2D<float> surfaceSizeMM, IG::Point2D<float> surfaceSizeSMM)
{
	bool changed = false;
	if(orientationIsSideways(softOrientation_))
		std::swap(surfaceSizeMM.x, surfaceSizeMM.y);
	auto oldSizeMM = std::exchange(winSizeMM, surfaceSizeMM);
	if(oldSizeMM != winSizeMM)
		changed = true;
	auto pixelSizeFloat = IG::Point2D<float>{(float)width(), (float)height()};
	mmToPixelScaler = pixelSizeFloat / winSizeMM;
	if constexpr(Config::envIsAndroid)
	{
		assert(surfaceSizeSMM.x && surfaceSizeSMM.y);
		if(orientationIsSideways(softOrientation_))
			std::swap(surfaceSizeSMM.x, surfaceSizeSMM.y);
		auto oldSizeSMM = std::exchange(winSizeSMM, surfaceSizeSMM);
		if(oldSizeSMM != sizeSMM())
			changed = true;
		smmToPixelScaler = pixelSizeFloat / sizeSMM();
	}
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
	oMask = appContext().validateOrientationMask(oMask);
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
		if(isMainWindow())
			appContext().setSystemOrientation(o);
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

void Window::dismiss()
{
	onDismiss(*this);
	drawEvent.detach();
	application().moveOutWindow(*this);
}

int Window::realWidth() const { return orientationIsSideways(softOrientation()) ? height() : width(); }

int Window::realHeight() const { return orientationIsSideways(softOrientation()) ? width() : height(); }

int Window::width() const { return winSizePixels.x; }

int Window::height() const { return winSizePixels.y; }

IG::Point2D<int> Window::realSize() const { return {realWidth(), realHeight()}; }

IG::Point2D<int> Window::size() const { return winSizePixels; }

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
	assert(sizeMM().x);
	return sizeMM().x;
}

float Window::heightMM() const
{
	assert(sizeMM().y);
	return sizeMM().y;
}

IG::Point2D<float> Window::sizeMM() const
{
	return winSizeMM;
}

float Window::widthSMM() const
{
	if constexpr(Config::envIsAndroid)
	{
		assert(sizeSMM().x);
		return sizeSMM().x;
	}
	return widthMM();
}

float Window::heightSMM() const
{
	if constexpr(Config::envIsAndroid)
	{
		assert(sizeSMM().y);
		return sizeSMM().y;
	}
	return heightMM();
}

IG::Point2D<float> Window::sizeSMM() const
{
	if constexpr(Config::envIsAndroid)
	{
		return winSizeSMM;
	}
	return sizeMM();
}

int Window::widthMMInPixels(float mm) const
{
	return std::round(mm * (mmToPixelScaler.x));
}

int Window::heightMMInPixels(float mm) const
{
	return std::round(mm * (mmToPixelScaler.y));
}

int Window::widthSMMInPixels(float mm) const
{
	if constexpr(Config::envIsAndroid)
	{
		return std::round(mm * (smmPixelScaler().x));
	}
	return widthMMInPixels(mm);
}

int Window::heightSMMInPixels(float mm) const
{
	if constexpr(Config::envIsAndroid)
	{
		return std::round(mm * (smmPixelScaler().y));
	}
	return heightMMInPixels(mm);
}

IG::Point2D<float> BaseWindow::smmPixelScaler() const
{
	return smmToPixelScaler;
}

IG::WindowRect Window::bounds() const
{
	return {{}, size()};
}

IG::Point2D<int> Window::transformInputPos(IG::Point2D<int> srcPos) const
{
	enum class PointerMode {NORMAL, INVERT};
	const auto xPointerTransform = softOrientation() == VIEW_ROTATE_0 || softOrientation() == VIEW_ROTATE_90 ? PointerMode::NORMAL : PointerMode::INVERT;
	const auto yPointerTransform = softOrientation() == VIEW_ROTATE_0 || softOrientation() == VIEW_ROTATE_270 ? PointerMode::NORMAL : PointerMode::INVERT;
	const auto pointerAxis = softOrientation() == VIEW_ROTATE_0 || softOrientation() == VIEW_ROTATE_180 ? PointerMode::NORMAL : PointerMode::INVERT;

	IG::Point2D<int> pos;
	// x,y axis is swapped first
	pos.x = pointerAxis == PointerMode::INVERT ? srcPos.y : srcPos.x;
	pos.y = pointerAxis == PointerMode::INVERT ? srcPos.x : srcPos.y;

	// then coordinates are inverted
	if(xPointerTransform == PointerMode::INVERT)
		pos.x = width() - pos.x;
	if(yPointerTransform == PointerMode::INVERT)
		pos.y = height() - pos.y;
	return pos;
}

Screen &WindowConfig::screen(ApplicationContext ctx) const
{
	return screen_ ? *screen_ : ctx.mainScreen();
}

ApplicationContext Window::appContext() const
{
	return onExit.appContext();
}

Application &Window::application() const
{
	return appContext().application();
}

}
