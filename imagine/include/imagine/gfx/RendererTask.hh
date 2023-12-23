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

#include <imagine/gfx/defs.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <chrono>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLRendererTask.hh>
#endif

namespace IG
{
struct WindowDrawParams;
class Window;
class Viewport;
}

namespace IG::Gfx
{

WISE_ENUM_CLASS((PresentMode, uint8_t),
	Auto, Immediate, FIFO
);

class RendererTask : public RendererTaskImpl
{
public:
	using RendererTaskImpl::RendererTaskImpl;
	void updateDrawableForSurfaceChange(Window &, WindowSurfaceChange);
	void setPresentMode(Window &, PresentMode);
	void setDefaultViewport(Window &, Viewport);
	void releaseShaderCompiler();
	void flush();
	void setDebugOutput(bool on);
	Renderer &renderer() const;
	explicit operator bool() const;

	// Run a delegate on the renderer thread
	void run(std::invocable auto &&f, MessageReplyMode mode = MessageReplyMode::none)
	{
		RendererTaskImpl::run(IG_forward(f), mode);
	}

	// Run a delegate for drawing on the renderer thread
	// Returns true if the window's contents were presented asynchronously
	bool draw(Window &win, WindowDrawParams winParams, DrawParams params,
		std::invocable<Window &, RendererCommands &> auto &&f)
	{
		return RendererTaskImpl::draw(win, winParams, params, IG_forward(f));
	}

	// synchronization
	SyncFence addSyncFence();
	void deleteSyncFence(SyncFence);
	void clientWaitSync(SyncFence fence, int flags = 0, std::chrono::nanoseconds timeout = SyncFence::IGNORE_TIMEOUT);
	SyncFence clientWaitSyncReset(SyncFence fence, int flags = 0, std::chrono::nanoseconds timeout = SyncFence::IGNORE_TIMEOUT);
	void waitSync(SyncFence fence);
	void awaitPending();
	ThreadId threadId() const;

	// buffers
	void write(auto &buff, auto &&data, ssize_t offset)
	{
		if constexpr(sizeof(&buff) + sizeof(offset) + sizeof(data) <= FuncDelegateStorageSize)
		{
			RendererTaskImpl::run([&buff, offset, data]()
			{
				buff.writeSubData(offset * buff.elemSize, sizeof(data), &data);
			});
		}
		else
		{
			RendererTaskImpl::run([&buff, offset](TaskContext ctx)
			{
				std::remove_cvref_t<decltype(data)> data;
				ctx.msgsPtr->readExtraData(std::span{&data, 1});
				buff.writeSubData(offset * buff.elemSize, sizeof(data), &data);
			}, IG_forward(data));
		}
	}
};

}
