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

#include <imagine/gfx/Renderer.hh>
#include <imagine/logger/logger.h>
#include "../utils.hh"

namespace IG::Gfx
{

static constexpr bool USE_DEPTH_BUFFER = false;

IG::Point2D<int> GLRendererTask::makeIOSDrawableRenderbuffer(void *layer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer)
{
	GLint backingWidth, backingHeight;
	runSync(
		[layer, &colorRenderbuffer, &depthRenderbuffer, &backingWidth, &backingHeight]()
		{
			// make the color renderbuffer
			if(!colorRenderbuffer)
			{
				glGenRenderbuffers(1, &colorRenderbuffer);
			}
			runGLChecked([&]()
			{
				glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
			}, "glBindRenderbuffer()");
			if(![[EAGLContext currentContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:(__bridge CAEAGLLayer*)layer])
			{
				logErr("error setting renderbuffer storage");
			}
			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
			if(USE_DEPTH_BUFFER)
			{
				if(!depthRenderbuffer)
				{
					glGenRenderbuffers(1, &depthRenderbuffer);
				}
				glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
			}
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
		});
	logMsg("made color renderbuffer:%d with size %dx%d", colorRenderbuffer, backingWidth, backingHeight);
	return {backingWidth, backingHeight};
}

void GLRendererTask::deleteIOSDrawableRenderbuffer(GLuint colorRenderbuffer, GLuint depthRenderbuffer)
{
	if(!colorRenderbuffer)
		return;
	runSync(
		[colorRenderbuffer, depthRenderbuffer]()
		{
			glBindRenderbuffer(GL_RENDERBUFFER, colorRenderbuffer);
			[[EAGLContext currentContext] renderbufferStorage:GL_RENDERBUFFER fromDrawable:nil];
			glDeleteRenderbuffers(1, &colorRenderbuffer);
			if(depthRenderbuffer)
			{
				glDeleteRenderbuffers(1, &depthRenderbuffer);
			}
		});
}

void GLRendererTask::setIOSDrawableDelegates()
{
	makeRenderbuffer =
		[this](void *layer, GLuint &colorRenderbuffer, GLuint &depthRenderbuffer)
		{
			return makeIOSDrawableRenderbuffer(layer, colorRenderbuffer, depthRenderbuffer);
		};
	deleteRenderbuffer =
		[this](GLuint colorRenderbuffer, GLuint depthRenderbuffer)
		{
			deleteIOSDrawableRenderbuffer(colorRenderbuffer, depthRenderbuffer);
		};
}

}
