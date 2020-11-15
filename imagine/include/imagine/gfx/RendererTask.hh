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

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRendererTask.hh>
#endif

#include <imagine/gfx/defs.hh>
#include <imagine/gfx/Viewport.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/base/baseDefs.hh>

namespace IG
{
class Semaphore;
}

namespace Gfx
{

class DrawableHolder;
class SyncFence;

class RendererTask : public RendererTaskImpl
{
public:
	enum class AsyncMode
	{
		NONE, PRESENT, FULL
	};

	enum class FenceMode
	{
		NONE, RESOURCE
	};

	class DrawParams
	{
	public:
		constexpr DrawParams() {}

		void setAsyncMode(AsyncMode mode)
		{
			asyncMode_ = mode;
		}

		AsyncMode asyncMode() const { return asyncMode_; }

		void setFenceMode(FenceMode mode)
		{
			fenceMode_ = mode;
		}

		FenceMode fenceMode() const { return fenceMode_; }

	private:
		AsyncMode asyncMode_ = AsyncMode::PRESENT;
		FenceMode fenceMode_ = FenceMode::RESOURCE;
	};

	RendererTask(Renderer &r);
	void start();
	void stop();
	void draw(DrawableHolder &drawable, Base::Window &win, Base::WindowDrawParams winParams, DrawParams params, DrawDelegate del);
	#ifdef CONFIG_GFX_RENDERER_TASK_DRAW_LOCK
	void lockDraw();
	void unlockDraw();
	#endif
	void waitForDrawFinished();
	void run(RenderTaskFuncDelegate func, bool awaitReply = false);
	void runSync(RenderTaskFuncDelegate func);
	void acquireFenceAndWait(Gfx::SyncFence &fenceVar);
	void updateDrawableForSurfaceChange(DrawableHolder &drawable, Base::WindowSurfaceChange change);
	void destroyDrawable(DrawableHolder &drawable);
	constexpr Renderer &renderer() const { return r; }

private:
	Renderer &r;
};

class RendererDrawTask : public RendererDrawTaskImpl
{
public:
	using RendererDrawTaskImpl::RendererDrawTaskImpl;

	RendererCommands makeRendererCommands(Drawable drawable, Viewport viewport, Mat4 projMat);
	void verifyCurrentContext() const;
	void notifyCommandsFinished();
	Renderer &renderer() const;
};

}
