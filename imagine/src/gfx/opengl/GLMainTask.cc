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

#define LOGTAG "GLRenderer"
#include <assert.h>
#include <imagine/gfx/Gfx.hh>
#include <imagine/thread/Thread.hh>
#include <imagine/logger/logger.h>

namespace Gfx
{

GLMainTask::~GLMainTask()
{
	stop();
}

void GLMainTask::start(Base::GLContext context)
{
	if(started)
		return;
	IG::makeDetachedThread(
		[this, context]()
		{
			auto glDpy = Base::GLDisplay::getDefault();
			#ifdef CONFIG_GFX_OPENGL_ES
			if(!Base::GLContext::bindAPI(Base::GLContext::OPENGL_ES_API))
			{
				bug_unreachable("unable to bind GLES API");
			}
			#endif
			context.setCurrent(glDpy, context, {});
			auto eventLoop = Base::EventLoop::makeForThread();
			commandPipe.addToEventLoop(eventLoop,
				[this, glDpy](Base::Pipe &pipe)
				{
					auto msg = pipe.readNoErr<CommandMessage>();
					switch(msg.command)
					{
						bcase Command::RUN_FUNC:
						{
							TaskContext ctx{};
							msg.args.run.func(ctx);
							if(msg.writeReply)
							{
								assert(!msg.args.run.semAddr);
								//logDMsg("sending command complete reply");
								replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
							}
							else if(msg.args.run.semAddr)
							{
								//logDMsg("notifying semaphore:%p", msg.args.run.semAddr);
								msg.args.run.semAddr->notify();
							}
						}
						bcase Command::EXIT:
						{
							Base::GLContext::setCurrent(glDpy, {}, {});
							Base::EventLoop::forThread().stop();
							return false;
						}
						bdefault:
						{
							logWarn("unknown ThreadCommandMessage value:%d", (int)msg.command);
						}
					}
					return true;
				});
			{
				replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
			}
			logMsg("starting main GL thread event loop");
			eventLoop.run();
			logMsg("main GL thread exit");
			commandPipe.removeFromEventLoop();
			{
				replyPipe.write(ReplyMessage{Reply::COMMAND_FINISHED});
			}
		});
	started = true;
	waitForCommandFinished();
}

void GLMainTask::runFunc(FuncDelegate del, bool writeReply, IG::Semaphore *semAddr)
{
	assert(started);
	commandPipe.write(CommandMessage{Command::RUN_FUNC, del, writeReply, semAddr});
}

void GLMainTask::runFuncSync(FuncDelegate del, bool writeReply, IG::Semaphore *semAddr)
{
	runFunc(del, writeReply, semAddr);
	if(semAddr)
	{
		//logDMsg("waiting for semaphore:%p", semAddr);
		assert(!writeReply);
		semAddr->wait();
	}
	else
	{
		waitForCommandFinished();
	}
}

void GLMainTask::stop()
{
	if(!started)
		return;
	commandPipe.write(CommandMessage{Command::EXIT});
	waitForCommandFinished();
	started = false;
}

void GLMainTask::waitForCommandFinished()
{
	assert(started);
	//logDMsg("waiting for command complete reply");
	ReplyMessage msg{};
	while(replyPipe.read(&msg, sizeof(msg)))
	{
		if(msg.reply == Reply::COMMAND_FINISHED)
		{
			//logDMsg("got reply");
			return;
		}
	}
}

}
