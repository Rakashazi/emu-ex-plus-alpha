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
#include <imagine/gfx/Renderer.hh>
#include <imagine/base/Window.hh>
#include <imagine/base/Screen.hh>
#include "utils.hh"
#include "internalDefs.hh"

namespace IG::Gfx
{

void GLRenderer::setGLProjectionMatrix(RendererCommands &cmds, Mat4 mat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
		glMatrixMode(GL_MODELVIEW);
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		cmds.setCachedProjectionMatrix(mat);
	}
	#endif
}

void RendererCommands::setProjectionMatrix(Mat4 mat)
{
	auto projectionMatRot = winPtr ? (float)winData(*winPtr).projAngleM : 0.f;
	if(projectionMatRot)
	{
		logMsg("rotated projection matrix by %f degrees", (double)IG::degrees(projectionMatRot));
		auto rotatedMat = mat.rollRotate(projectionMatRot);
		renderer().setGLProjectionMatrix(*this, rotatedMat);
	}
	else
	{
		//logMsg("no rotation");
		renderer().setGLProjectionMatrix(*this, mat);
	}
}

void Renderer::animateProjectionMatrixRotation(Window &win, float srcAngle, float destAngle)
{
	winData(win).projAngleM = {srcAngle, destAngle, {}, IG::steadyClockTimestamp(), IG::Milliseconds{165}};
	win.addOnFrame(
		[this, &win](IG::FrameParams params)
		{
			bool didUpdate = winData(win).projAngleM.update(params.timestamp());
			win.postDraw();
			return didUpdate;
		});
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
	if(currProgram.program) [[likely]]
	{
		auto mvpMat = projectionMat.mult(mat);
		glUniformMatrix4fv(currProgram.mvpUniform, 1, GL_FALSE, &mvpMat[0][0]);
	}
	#endif
}

void RendererCommands::loadTranslate(float x, float y, float z)
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
