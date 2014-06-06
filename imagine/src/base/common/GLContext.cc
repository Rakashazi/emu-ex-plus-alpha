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

#include <imagine/base/GLContext.hh>
#include <imagine/logger/logger.h>

namespace Base
{

static GLContext *currentContext = nullptr;
static Window *currentDrawable_ = nullptr;

bool GLContext::setCurrent(GLContext *context, Window *win)
{
	if(currentContext != context)
	{
		logMsg("setting drawable %p and context %p", win, context);
		currentContext = context;
		if(!context)
			win = nullptr; // always unset the drawable if unbinding context
		currentDrawable_ = win;
		setCurrentContext(context, win);
		return true;
	}
	else
		return setDrawable(win);
}

bool GLContext::setDrawable(Window *win)
{
	if(currentContext && currentDrawable_ != win)
	{
		logMsg("setting drawable %p for current context %p", win, currentContext);
		currentDrawable_ = win;
		currentContext->setCurrentDrawable(win);
		return true;
	}
	else if(!currentContext && win)
	{
		bug_exit("cannot set drawable without current context");
	}
	return false;
}

GLContext *GLContext::current()
{
	return currentContext;
}

Window *GLContext::drawable()
{
	return currentDrawable_;
}

bool GLContext::validateCurrent()
{
	if(currentContext && !currentContext->isRealCurrentContext())
	{
		setCurrentContext(currentContext, currentDrawable_);
		return true;
	}
	return false;
}

}
