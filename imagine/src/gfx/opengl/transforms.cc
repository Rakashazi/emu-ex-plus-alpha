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

#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include "utils.h"
#include "private.hh"

namespace Gfx
{

void GLRenderer::setGLProjectionMatrix(RendererCommands &cmds, const Mat4 &mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		cmds.glcMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
		cmds.glcMatrixMode(GL_MODELVIEW);
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		cmds.projectionMat = mat;
	}
	#endif
}

void Renderer::setProjectionMatrixRotation(Angle angle)
{
	projectionMatRot = angle;
}

void RendererCommands::setProjectionMatrix(const Mat4 &mat)
{
	if(renderer().projectionMatRot)
	{
		logMsg("rotated projection matrix by %f degrees", (double)IG::degrees(renderer().projectionMatRot));
		auto rotatedMat = mat.rollRotate(renderer().projectionMatRot);
		renderer().setGLProjectionMatrix(*this, rotatedMat);
	}
	else
	{
		//logMsg("no rotation");
		renderer().setGLProjectionMatrix(*this, mat);
	}
}

void Renderer::animateProjectionMatrixRotation(Base::Window &win, Angle srcAngle, Angle destAngle)
{
	projAngleM.set(srcAngle, destAngle, INTERPOLATOR_TYPE_EASEOUTQUAD, 10);
	win.screen()->addOnFrame(
		[this, &win](IG::FrameParams params)
		{
			using namespace Base;
			//logMsg("animating rotation");
			projAngleM.update(1);
			setProjectionMatrixRotation(projAngleM.now());
			win.postDraw();
			return !projAngleM.isComplete();
		});
}

void RendererCommands::setTransformTarget(TransformTargetEnum target)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
		glcMatrixMode(target == TARGET_TEXTURE ? GL_TEXTURE : GL_MODELVIEW);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!renderer().support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
	#endif
}

void RendererCommands::loadTransform(Mat4 mat)
{
	rTask->verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
	{
		glLoadMatrixf(&mat[0][0]);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	modelMat = mat;
	if(likely(currProgram))
	{
		auto mvpMat = projectionMat.mult(mat);
		glUniformMatrix4fv(currProgram->modelViewProjectionUniform, 1, GL_FALSE, &mvpMat[0][0]);
	}
	#endif
}

void RendererCommands::loadTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
{
	loadTransform(Mat4::makeTranslate({x, y, z}));
}

void RendererCommands::loadIdentTransform()
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(renderer().support.useFixedFunctionPipeline)
		glLoadIdentity();
	#endif
	if(!renderer().support.useFixedFunctionPipeline)
		loadTransform({});
}

}
