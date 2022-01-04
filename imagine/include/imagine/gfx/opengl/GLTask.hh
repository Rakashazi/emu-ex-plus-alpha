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
#include <imagine/util/concepts.hh>
#include <imagine/util/utility.h>
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
	int threadPriority{};
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
	using FuncDelegate = DelegateFuncA<sizeof(uintptr_t)*4 + sizeof(int)*10, 16, void(GLDisplay glDpy, std::binary_semaphore *semPtr)>;

	enum class Command: uint8_t
	{
		UNSET,
		RUN_FUNC,
		EXIT
	};

	struct CommandMessage
	{
		std::binary_semaphore *semPtr{};
		union Args
		{
			struct RunArgs
			{
				FuncDelegate func;
			} run;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() = default;
		constexpr CommandMessage(Command command, std::binary_semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, FuncDelegate funcDel, std::binary_semaphore *semPtr = nullptr):
			semPtr{semPtr}, args{funcDel}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
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

	void run(IG::invocable auto &&f, bool awaitReply = false)
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

	void run(IG::invocable<TaskContext> auto &&f, bool awaitReply = false)
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

	GLContext makeGLContext(GLManager &, GLBufferConfig bufferConf);
	void deinit();
};

}
