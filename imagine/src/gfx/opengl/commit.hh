#pragma once

#include <base/private.hh>

namespace Gfx
{

static uint clearColorBufferBit = 0, clearZBufferBit = GL_DEPTH_BUFFER_BIT;

void waitVideoSync()
{
	logDMsg("called wait video sync");
	/*#ifdef CONFIG_BASE_X11
	if(useSGIVidSync)
	{
		uint oldFrameTime = gfx_frameTime;
		//glXGetVideoSyncSGI(&gfx_frameTime);
		glXWaitVideoSyncSGI(2, (gfx_frameTime+1)%2, &gfx_frameTime);
		//if(gfx_frameTime - oldFrameTime > 1)
		//	logDMsg("%d since last sync, after wait", gfx_frameTime - oldFrameTime);
	}
	#endif*/
}

void setVideoInterval(uint interval)
{
	Base::setVideoInterval(interval);
}

/*void gfx_initFrameClockTime()
{
	/*#ifdef CONFIG_BASE_X11
	if(useSGIVidSync)
	{
		glXGetVideoSyncSGI(&gfx_frameTime);
		gfx_frameTimeRel = 1;
	}
	else
	#endif*/
	/*{
		gfx_frameClockTime.setTimeNow();
		//gfx_frameTimeRel = 1;
	}
}*/

void updateFrameTime()
{
	#ifdef CONFIG_GFX_OPENGL_GLX
	if(useSGIVidSync)
	{
		uint prevFrameTime = gfx_frameTime;
		glXGetVideoSyncSGI(&gfx_frameTime);
		gfx_frameTimeRel = gfx_frameTime - prevFrameTime;
	}
	else
	#endif
	{
		//static TimeSys prevTime;
		TimeSys now;
		now.setTimeNow();
		TimeSys currFrameTime = now - startFrameTime;
		#ifdef CONFIG_ENV_WEBOS
		//uint currFrame = currFrameTime.divByUSecs(15936); // 62~63 fps
		uint currFrame = currFrameTime.divByUSecs(19000);
		#else
		uint currFrame = currFrameTime.divByUSecs(16666);
		#endif
		//logMsg("now %f start %f, curr %f %d", (double)now, (double)startFrameTime, (double)currFrameTime, currFrame);
		gfx_frameTimeRel = currFrame - gfx_frameTime;
		gfx_frameTime = currFrame;
	}
	//logMsg("current frame %d, diff %d", gfx_frameTime, gfx_frameTimeRel);
}

void clear()
{
	#if defined(CONFIG_GFX_OPENGL_ES)
		// always clear screen to trigger a discarded buffer optimization
		// TODO: test other systems to determine the what's best
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);
	#else
		if(clearColorBufferBit || clearZBufferBit)
		{
			//logMsg("clear color: %d, clear z: %d", clearColorBufferBit, clearZBufferBit);
			glClear(clearColorBufferBit | clearZBufferBit);
		}
	#endif
}

void renderFrame()
{
	if(unlikely(animateOrientationChange && !projAngleM.isComplete()))
	{
		//logMsg("animating rotation");
		projAngleM.update();
		resizeGLScene(Base::window());
		Base::displayNeedsUpdate();
	}

	Gfx::onDraw();

	//glFlush();
	//glFinish();
	#ifdef CONFIG_BASE_ANDROID
		if(unlikely(glSyncHackEnabled)) glFinish();
	#endif

	Base::openGLUpdateScreen();

	/*#if defined(CONFIG_GFX_OPENGL_ES) && defined(CONFIG_BASE_IOS)
	if(useDiscardFramebufferEXT)
	{
		static const GLenum discardAttachments[] = { GL_COLOR_EXT, GL_DEPTH_EXT, GL_STENCIL_EXT };
		glDiscardFramebufferEXT(GL_FRAMEBUFFER_OES, 3, discardAttachments);
	}
	#endif*/

	clear();

	/*#ifdef CONFIG_BASE_X11
	if(useSGIVidSync)
	{
		uint oldFrameTime = gfx_frameTime;
		glXGetVideoSyncSGI(&gfx_frameTime);
		logDMsg("gfx update, old %d now %d", );
		if(gfx_frameTime - oldFrameTime > 1)
			logDMsg("%d since last sync, after render", gfx_frameTime - oldFrameTime);
	}
	#endif*/
}

}
