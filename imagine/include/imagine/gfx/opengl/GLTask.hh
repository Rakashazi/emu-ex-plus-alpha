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
#include <imagine/util/FunctionTraits.hh>
#include <imagine/util/NonCopyable.hh>
#include <thread>

namespace IG
{
class Semaphore;
}

namespace Gfx
{

class Renderer;
class GLRendererTask;

struct GLTaskConfig
{
	Base::GLManager *glManagerPtr{};
	Base::GLBufferConfig bufferConfig{};
	Drawable initialDrawable{};
	int threadPriority{};
};

// Wraps an OpenGL context in a thread + message port
class GLTask : private NonCopyable
{
public:
	class TaskContext
	{
	public:
		constexpr TaskContext(Base::GLDisplay glDpy, IG::Semaphore *semPtr, bool *semaphoreNeedsNotifyPtr):
			glDpy{glDpy}, semPtr{semPtr}, semaphoreNeedsNotifyPtr{semaphoreNeedsNotifyPtr}
		{}
		void notifySemaphore();
		void markSemaphoreNotified();
		constexpr Base::GLDisplay glDisplay() const { return glDpy; }
		constexpr IG::Semaphore *semaphorePtr() const { return semPtr; }

	protected:
		[[no_unique_address]] Base::GLDisplay glDpy{};
		IG::Semaphore *semPtr{};
		bool *semaphoreNeedsNotifyPtr{};
	};

	using FuncDelegate = DelegateFunc2<sizeof(uintptr_t)*4 + sizeof(int)*10, void(Base::GLDisplay glDpy, IG::Semaphore *semPtr)>;

	enum class Command: uint8_t
	{
		UNSET,
		RUN_FUNC,
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

	GLTask(Base::ApplicationContext);
	GLTask(Base::ApplicationContext, const char *debugLabel);
	~GLTask();
	Error makeGLContext(GLTaskConfig);
	void runFunc(FuncDelegate del, bool awaitReply);
	Base::GLBufferConfig glBufferConfig() const;
	const Base::GLContext &glContext() const;
	Base::ApplicationContext appContext() const;
	explicit operator bool() const;

	template<class Func>
	void run(Func &&del, bool awaitReply = false) { runFunc(wrapFuncDelegate(std::forward<Func>(del)), awaitReply); }

	template<class Func>
	void runSync(Func &&del) { run(std::forward<Func>(del), true); }

	template<class Func>
	static constexpr FuncDelegate wrapFuncDelegate(Func &&del)
	{
		return
			[=](Base::GLDisplay glDpy, IG::Semaphore *semPtr)
			{
				constexpr auto args = IG::functionTraitsArity<Func>;
				if constexpr(args == 0)
				{
					del();
					if(semPtr)
					{
						semPtr->notify();
					}
				}
				else
				{
					bool semaphoreNeedsNotify = semPtr;
					TaskContext ctx{glDpy, semPtr, &semaphoreNeedsNotify};
					del(ctx);
					if(semaphoreNeedsNotify) // semaphore wasn't already notified in the delegate
					{
						semPtr->notify();
					}
				}
			};
	}

protected:
	std::thread thread{};
	Base::GLContext context{};
	Base::GLBufferConfig bufferConfig{};
	Base::OnExit onExit;
	Base::MessagePort<CommandMessage> commandPort{Base::MessagePort<CommandMessage>::NullInit{}};

	Base::GLContext makeGLContext(Base::GLManager &, Base::GLBufferConfig bufferConf);
	void deinit();
};

}
