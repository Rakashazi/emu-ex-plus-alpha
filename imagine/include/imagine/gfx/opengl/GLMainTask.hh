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
#include <imagine/util/FunctionTraits.hh>
#include <thread>

namespace IG
{
class Semaphore;
}

namespace Gfx
{

class Renderer;
class GLRendererTask;
class DrawableHolder;

// Wraps an OpenGL context in a thread + message port
class GLMainTask
{
public:
	class TaskContext
	{
	public:
		constexpr TaskContext(Base::GLDisplay glDpy, IG::Semaphore *semPtr, bool *semaphoreWasNotifiedPtr):
			glDpy{glDpy}, semPtr{semPtr}, semaphoreWasNotifiedPtr{semaphoreWasNotifiedPtr}
		{}
		void notifySemaphore();
		void markSemaphoreNotified();
		constexpr Base::GLDisplay glDisplay() const { return glDpy; }
		constexpr IG::Semaphore *semaphorePtr() const { return semPtr; }

	protected:
		Base::GLDisplay glDpy{};
		IG::Semaphore *semPtr{};
		bool *semaphoreWasNotifiedPtr{};
	};

	using FuncDelegate = DelegateFunc2<sizeof(uintptr_t)*4 + sizeof(int)*10, void(TaskContext)>;

	enum class Command: uint8_t
	{
		UNSET,
		RUN_FUNC,						// delegate manages the semaphore
		RUN_FUNC_SEMAPHORE, // delegate can signal semaphore, or it's automatically signaled
		EXIT
	};

	struct CommandMessage
	{
		IG::Semaphore *semPtr{};
		union Args
		{
			struct RunArgs
			{
				FuncDelegate func;
			} run;
		} args{};
		Command command{Command::UNSET};

		constexpr CommandMessage() {}
		constexpr CommandMessage(Command command, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, command{command} {}
		constexpr CommandMessage(Command command, FuncDelegate funcDel, IG::Semaphore *semPtr = nullptr):
			semPtr{semPtr}, args{funcDel}, command{command} {}
		explicit operator bool() const { return command != Command::UNSET; }
		void setReplySemaphore(IG::Semaphore *semPtr_) { assert(!semPtr); semPtr = semPtr_; };
	};

	GLMainTask();
	GLMainTask(const char *debugLabel, Base::GLContext context, int threadPriority = 0);
	GLMainTask(GLMainTask &&o) = default;
	GLMainTask &operator=(GLMainTask &&o) = default;
	~GLMainTask();
	void runFunc(FuncDelegate del, bool awaitReply, bool manageSemaphore);
	Base::GLContext glContext() const;
	explicit operator bool() const;

	template<class Func>
	void run(Func &&del, bool awaitReply = false) { runFunc(wrapFuncDelegate(del), awaitReply, true); }
	template<class Func>
	void runSync(Func &&del) { runFunc(wrapFuncDelegate(del), true, true); }
	template<class Func>
	void runUnmangedSem(Func &&del, bool awaitReply = false) { runFunc(wrapFuncDelegate(del), awaitReply, false); }

	template<class Func = FuncDelegate>
	static FuncDelegate wrapFuncDelegate(Func del)
	{
		constexpr auto args = IG::functionTraitsArity<Func>;
		if constexpr(args == 0)
		{
			// for void ()
			return
				[=](TaskContext)
				{
					del();
				};
		}
		else
		{
			// for void (GLMainTask::TaskContext)
			return del;
		}
	}

protected:
	std::thread thread{};
	Base::GLContext context{};
	Base::ExitDelegate onExit{};
	Base::MessagePort<CommandMessage> commandPort{Base::MessagePort<CommandMessage>::NullInit{}};

	void deinit();
};

using RendererTaskContext = GLMainTask::TaskContext;

class GLRendererTaskDrawContext
{
public:
	GLRendererTaskDrawContext(GLRendererTask &task, GLMainTask::TaskContext taskCtx, bool notifySemaphoreAfterPresent);

	RendererTask *task;
	IG::Semaphore *drawCompleteSemPtr{};
	Base::GLDisplay glDpy{};
	bool notifySemaphoreAfterPresent{};
};

using RendererTaskDrawContextImpl = GLRendererTaskDrawContext;

}
