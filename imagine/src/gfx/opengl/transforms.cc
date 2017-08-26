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

#include <imagine/gfx/Gfx.hh>
#include <imagine/base/Base.hh>
#include "utils.h"
#include "private.hh"

namespace Gfx
{

void GLRenderer::setGLProjectionMatrix(const Mat4 &mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		glcMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
		glcMatrixMode(GL_MODELVIEW);
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		projectionMat = mat;
		projectionMatAge++;
		if(likely(currProgram))
		{
			glUniformMatrix4fv(currProgram->projectionUniform, 1, GL_FALSE, &mat[0][0]);
			currProgram->projectionUniformAge = projectionMatAge;
		}
	}
	#endif
}

void Renderer::setProjectionMatrixRotation(Angle angle)
{
	projectionMatRot = angle;
}

const Mat4 &Renderer::projectionMatrix()
{
	return projectionMatPreTransformed;
}

void Renderer::setProjectionMatrix(const Mat4 &mat)
{
	verifyCurrentContext();
	projectionMatPreTransformed = mat;
	if(projectionMatRot)
	{
		logMsg("rotated projection matrix by %f degrees", (double)IG::degrees(projectionMatRot));
		auto rotatedMat = mat.rollRotate(projectionMatRot);
		setGLProjectionMatrix(rotatedMat);
	}
	else
	{
		//logMsg("no rotation");
		setGLProjectionMatrix(mat);
	}
}

void Renderer::animateProjectionMatrixRotation(Angle srcAngle, Angle destAngle)
{
	projAngleM.set(srcAngle, destAngle, INTERPOLATOR_TYPE_EASEOUTQUAD, 10);
	Base::mainScreen().addOnFrameOnce(
		[this](Base::Screen::FrameParams params)
		{
			using namespace Base;
			//logMsg("animating rotation");
			bind();
			projAngleM.update(1);
			setProjectionMatrixRotation(projAngleM.now());
			setProjectionMatrix(projectionMatrix());
			mainWindow().setNeedsDraw(true);
			if(!projAngleM.isComplete())
			{
				params.readdOnFrame();
			}
		});
}

void Renderer::setTransformTarget(TransformTargetEnum target)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
		glcMatrixMode(target == TARGET_TEXTURE ? GL_TEXTURE : GL_MODELVIEW);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!support.useFixedFunctionPipeline)
	{
		bug_unreachable("TODO");
	}
	#endif
}

void Renderer::loadTransform(Mat4 mat)
{
	verifyCurrentContext();
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
	{
		glLoadMatrixf(&mat[0][0]);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat != mat)
	{
		modelMat = mat;
		modelMatAge++;
		//logMsg("transform matrix updated, age %d", modelMatAge);
	}
	else
	{
		//logMsg("transform matrix matches, skipped age update");
	}
	if(likely(currProgram) && currProgram->modelViewUniformAge != modelMatAge)
	{
		glUniformMatrix4fv(currProgram->modelViewUniform, 1, GL_FALSE, &mat[0][0]);
		currProgram->modelViewUniformAge = modelMatAge;
	}
	#endif
}

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
void GLRenderer::updateProgramProjectionTransform(GLSLProgram &program)
{
	verifyCurrentContext();
	if(program.projectionUniformAge != projectionMatAge)
	{
		//logMsg("updating projection matrix for program %d (age was %d, now %d)", program.program(), program.projectionUniformAge, projectionMatAge);
		if(likely(program.projectionUniform != -1))
			glUniformMatrix4fv(program.projectionUniform, 1, GL_FALSE, &projectionMat[0][0]);
		program.projectionUniformAge = projectionMatAge;
	}
}

void GLRenderer::updateProgramModelViewTransform(GLSLProgram &program)
{
	verifyCurrentContext();
	if(program.modelViewUniformAge != modelMatAge)
	{
		//logMsg("updating model/view matrix for program %d (age was %d, now %d)", program.program(), program.modelViewUniformAge, modelMatAge);
		if(likely(program.modelViewUniform != -1))
			glUniformMatrix4fv(program.modelViewUniform, 1, GL_FALSE, &modelMat[0][0]);
		program.modelViewUniformAge = modelMatAge;
	}
}
#endif

void Renderer::loadTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
{
	loadTransform(Mat4::makeTranslate({x, y, z}));
}

void Renderer::loadIdentTransform()
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(support.useFixedFunctionPipeline)
		glLoadIdentity();
	#endif
	if(!support.useFixedFunctionPipeline)
		loadTransform({});
}

}
