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
#include <imagine/gfx/SyncFence.hh>
#include <imagine/base/baseDefs.hh>
#include <utility>

namespace IG
{
class Semaphore;
}

namespace Gfx
{

class RendererTask : public RendererTaskImpl
{
public:
	using RendererTaskImpl::RendererTaskImpl;
	void updateDrawableForSurfaceChange(Base::Window &, Base::WindowSurfaceChange);
	void releaseShaderCompiler();
	void flush();
	void setDebugOutput(bool on);
	Renderer &renderer() const;
	explicit operator bool() const;

	// Run a delegate on the renderer thread with signature:
	// void()
	template<class Func>
	void run(Func &&del, bool awaitReply = false)
	{
		RendererTaskImpl::run(std::forward<Func>(del), awaitReply);
	}

	// Run a delegate for drawing on the renderer thread with signature:
	// void(Base::Window &win, RendererCommands &cmds)
	// Returns true if the window's contents were presented synchronously
	template<class Func>
	bool draw(Base::Window &win, Base::WindowDrawParams winParams, DrawParams params,
		const Viewport &viewport, const Mat4 &projMat, Func &&del)
	{
		return RendererTaskImpl::draw(win, winParams, params, viewport, projMat, std::forward<Func>(del));
	}

	// synchronization
	SyncFence addSyncFence();
	void deleteSyncFence(SyncFence);
	void clientWaitSync(SyncFence fence, int flags = 0, std::chrono::nanoseconds timeout = SyncFence::IGNORE_TIMEOUT);
	SyncFence clientWaitSyncReset(SyncFence fence, int flags = 0, std::chrono::nanoseconds timeout = SyncFence::IGNORE_TIMEOUT);
	void waitSync(SyncFence fence);
	void awaitPending();
};
}
