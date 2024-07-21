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
#include <imagine/base/GLContext.hh>
#include <imagine/base/MessagePort.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/thread/Semaphore.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/util/utility.h>
#include <concepts>
#include <thread>

namespace IG::Gfx
{

class Renderer;
class GLRendererTask;

struct GLTaskConfig
{
	GLManager *glManagerPtr{};
	GLBufferConfig bufferConfig{};
	Drawable initialDrawable{};
};

// Wraps an OpenGL context in a thread + message port
class GLTask
{
public:
	struct CommandMessage;
	using CommandMessages = Messages<CommandMessage>;

	// Align delegate data to 16 bytes in case we store SIMD types
	static constexpr size_t FuncDelegateStorageSize = sizeof(uintptr_t)*2 + sizeof(int)*16;
	using FuncDelegate = DelegateFuncA<FuncDelegateStorageSize, 16, void(GLDisplay, std::binary_semaphore *, CommandMessages &)>;

	struct CommandMessage
	{
		std::binary_semaphore *semPtr{};
		FuncDelegate func{};

		void setReplySemaphore(std::binary_semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	using CommandMessagePort = MessagePort<CommandMessage>;

	struct TaskContext
	{
		[[no_unique_address]] GLDisplay glDisplay{};
		std::binary_semaphore *semaphorePtr{};
		bool *semaphoreNeedsNotifyPtr{};
		CommandMessages *msgsPtr;

		void notifySemaphore();
		void markSemaphoreNotified();
	};

	GLTask(ApplicationContext);
	GLTask(ApplicationContext, const char *debugLabel);
	~GLTask();
	GLTask &operator=(GLTask &&) = delete;
	bool makeGLContext(GLTaskConfig);
	void runFunc(FuncDelegate del, std::span<const uint8_t> extBuff, MessageReplyMode);
	GLBufferConfig glBufferConfig() const;
	const GLContext &glContext() const;
	ApplicationContext appContext() const;
	explicit operator bool() const;

	void run(std::invocable auto &&f, MessageReplyMode mode = MessageReplyMode::none)
	{
		runFunc(
			[=](GLDisplay, std::binary_semaphore *semPtr, CommandMessages &)
			{
				f();
				if(semPtr)
				{
					semPtr->release();
				}
			}, {}, mode);
	}

	template<class ExtraData>
	void run(std::invocable<TaskContext> auto &&f, ExtraData &&extData, MessageReplyMode mode = MessageReplyMode::none)
	{
		std::span<const uint8_t> extBuff;
		if constexpr(!std::is_null_pointer_v<ExtraData>)
		{
			extBuff = {reinterpret_cast<const uint8_t*>(&extData), sizeof(extData)};
		}
		runFunc(
			[=](GLDisplay glDpy, std::binary_semaphore *semPtr, CommandMessages &msgs)
			{
				bool semaphoreNeedsNotify = semPtr;
				TaskContext ctx{glDpy, semPtr, &semaphoreNeedsNotify, &msgs};
				f(ctx);
				if(semaphoreNeedsNotify) // semaphore wasn't already notified in the delegate
				{
					semPtr->release();
				}
			}, extBuff, mode);
	}

	void run(std::invocable<TaskContext> auto &&f, MessageReplyMode mode = MessageReplyMode::none)
	{
		run(IG_forward(f), nullptr, mode);
	}

	void runSync(auto &&f) { run(IG_forward(f), MessageReplyMode::wait); }

protected:
	std::thread thread{};
	GLContext context{};
	GLBufferConfig bufferConfig{};
	OnExit onExit;
	CommandMessagePort commandPort;
	ThreadId threadId_{};

	GLContext makeGLContext(GLManager &, GLBufferConfig bufferConf);
	void deinit();
};

}
