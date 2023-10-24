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
	class TaskContext
	{
	public:
		constexpr TaskContext(GLDisplay glDpy, std::binary_semaphore *semPtr, bool *semaphoreNeedsNotifyPtr):
			glDpy{glDpy}, semPtr{semPtr}, semaphoreNeedsNotifyPtr{semaphoreNeedsNotifyPtr}
		{}
		void notifySemaphore();
		void markSemaphoreNotified();
		constexpr GLDisplay glDisplay() const { return glDpy; }
		constexpr std::binary_semaphore *semaphorePtr() const { return semPtr; }

	protected:
		[[no_unique_address]] GLDisplay glDpy{};
		std::binary_semaphore *semPtr{};
		bool *semaphoreNeedsNotifyPtr{};
	};

	// Align delegate data to 16 bytes in case we store SIMD types
	static constexpr size_t FuncDelegateStorageSize = sizeof(uintptr_t)*2 + sizeof(int)*16;
	using FuncDelegate = DelegateFuncA<FuncDelegateStorageSize, 16, void(GLDisplay glDpy, std::binary_semaphore *semPtr)>;

	struct CommandMessage
	{
		std::binary_semaphore *semPtr{};
		FuncDelegate func{};

		void setReplySemaphore(std::binary_semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	GLTask(ApplicationContext);
	GLTask(ApplicationContext, const char *debugLabel);
	~GLTask();
	GLTask &operator=(GLTask &&) = delete;
	bool makeGLContext(GLTaskConfig);
	void runFunc(FuncDelegate del, bool awaitReply);
	GLBufferConfig glBufferConfig() const;
	const GLContext &glContext() const;
	ApplicationContext appContext() const;
	explicit operator bool() const;

	void run(std::invocable auto &&f, bool awaitReply = false)
	{
		runFunc(
			[=](GLDisplay, std::binary_semaphore *semPtr)
			{
				f();
				if(semPtr)
				{
					semPtr->release();
				}
			}, awaitReply);
	}

	void run(std::invocable<TaskContext> auto &&f, bool awaitReply = false)
	{
		runFunc(
			[=](GLDisplay glDpy, std::binary_semaphore *semPtr)
			{
				bool semaphoreNeedsNotify = semPtr;
				TaskContext ctx{glDpy, semPtr, &semaphoreNeedsNotify};
				f(ctx);
				if(semaphoreNeedsNotify) // semaphore wasn't already notified in the delegate
				{
					semPtr->release();
				}
			}, awaitReply);
	}

	void runSync(auto &&f) { run(IG_forward(f), true); }

protected:
	std::thread thread{};
	GLContext context{};
	GLBufferConfig bufferConfig{};
	OnExit onExit;
	MessagePort<CommandMessage> commandPort{MessagePort<CommandMessage>::NullInit{}};
	ThreadId threadId_{};

	GLContext makeGLContext(GLManager &, GLBufferConfig bufferConf);
	void deinit();
};

}
