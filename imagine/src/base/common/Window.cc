#define thisModuleName "base:window"
#include <base/Base.hh>
#include <base/common/windowPrivate.hh>
#ifdef CONFIG_GFX
#include <gfx/Gfx.hh>
#endif

namespace Base
{

#ifdef CONFIG_BASE_MULTI_WINDOW

StaticArrayList<Window*, 4> window;

Window &mainWindow()
{
	assert(!window.empty());
	return *window[0];
}

bool drawWindows(Gfx::FrameTimeBase frameTime)
{
	bool didDraw = false;
	if(unlikely(appState != APP_RUNNING))
	{
		// unpost all windows if inactive
		for(auto w : window)
		{
			w->drawPosted = false;
		}
		return false;
	}
	for(auto w : window)
	{
		if(w->drawPosted)
		{
			w->setAsDrawTarget();
			Gfx::clear();
			w->draw(frameTime);
			w->needsSwap = true;
			didDraw = true;
		}
	}
	for(auto w : window)
	{
		if(w->needsSwap)
		{
			w->swapBuffers();
		}
	}
	return didDraw;
}

bool windowsArePosted()
{
	for(auto w : window)
	{
		if(w->drawPosted)
		{
			return true;
		}
	}
	return false;
}

#else

Window *mainWin = nullptr;

Window &mainWindow()
{
	assert(mainWin);
	return *mainWin;
}

bool drawWindows(Gfx::FrameTimeBase frameTime)
{
	if(unlikely(appState != APP_RUNNING))
	{
		mainWin->drawPosted = false;
		return false;
	}
	if(mainWin->drawPosted)
	{
		mainWin->draw(frameTime);
		mainWin->swapBuffers();
		Gfx::clear();
		return true;
	}
	else
		return false;
}

bool windowsArePosted()
{
	return mainWin->drawPosted;
}

#endif

void Window::adjustViewport(IG::Rect2<int> rect)
{
	viewRect = rect;
	viewPixelWidth_ = viewRect.xSize();
	viewPixelHeight_ = viewRect.ySize();
	#ifdef CONFIG_GFX
	Gfx::setViewport(*this);
	Gfx::setProjector(*this);
	#endif
}

void Window::postResize(bool redraw)
{
	logMsg("setting window bounds to %d:%d:%d:%d", viewRect.x, viewRect.y, viewRect.x2, viewRect.y2);
	#ifdef CONFIG_GFX
	Gfx::setViewport(*this);
	#endif
	resizePosted = true;
	if(redraw)
		displayNeedsUpdate();
}

void Window::draw(Gfx::FrameTimeBase frameTime)
{
	drawPosted = false;
	if(unlikely(resizePosted))
	{
		resizePosted = false;
		#ifdef CONFIG_GFX
		Gfx::setProjector(*this);
		Gfx::onViewChange(*this, nullptr);
		#endif
	}
	#ifdef CONFIG_GFX
	Gfx::renderFrame(*this, frameTime);
	#endif
}

}
