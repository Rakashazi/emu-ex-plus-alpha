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
#include "GLStateCache.hh"
#include "utils.h"
#include "private.hh"

namespace Gfx
{

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
Mat4 modelMat, projectionMat;
extern GLint projectionUniform, modelViewUniform;
static uint modelMatAge = 0, projectionMatAge = 0;
#endif
Angle projectionMatRot = 0;
Mat4 projectionMatPreTransformed;

static void setGLProjectionMatrix(const Mat4 &mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		glcMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
		glcMatrixMode(GL_MODELVIEW);
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
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

void setProjectionMatrixRotation(Angle angle)
{
	projectionMatRot = angle;
}

const Mat4 &projectionMatrix()
{
	return projectionMatPreTransformed;
}

void setProjectionMatrix(const Mat4 &mat)
{
	projectionMatPreTransformed = mat;
	if(projectionMatRot)
	{
		logMsg("rotated projection matrix by %f degrees", (double)IG::toDegrees(projectionMatRot));
		auto rotatedMat = mat.rollRotate(projectionMatRot);
		setGLProjectionMatrix(rotatedMat);
	}
	else
	{
		//logMsg("no rotation");
		setGLProjectionMatrix(mat);
	}
}

void animateProjectionMatrixRotation(Angle srcAngle, Angle destAngle)
{
	Gfx::projAngleM.set(srcAngle, destAngle, INTERPOLATOR_TYPE_EASEOUTQUAD, 10);
	Base::mainScreen().addOnFrameOnce(
		[](Base::Screen &screen, Base::Screen::FrameParams params)
		{
			using namespace Base;
			setCurrentWindow(&mainWindow());
			//logMsg("animating rotation");
			projAngleM.update(1);
			setProjectionMatrixRotation(projAngleM.now());
			setProjectionMatrix(projectionMatrix());
			mainWindow().setNeedsDraw(true);
			if(!projAngleM.isComplete())
			{
				screen.postOnFrame(params.thisOnFrame());
			}
		});
}

void setTransformTarget(TransformTargetEnum target)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		glcMatrixMode(target == TARGET_TEXTURE ? GL_TEXTURE : GL_MODELVIEW);
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(!useFixedFunctionPipeline)
	{
		bug_exit("TODO");
	}
	#endif
}

void loadTransform(Mat4 mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
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
void updateProgramProjectionTransform(GLSLProgram &program)
{
	if(program.projectionUniformAge != projectionMatAge)
	{
		//logMsg("updating projection matrix for program %d (age was %d, now %d)", program.program(), program.projectionUniformAge, projectionMatAge);
		if(likely(program.projectionUniform != -1))
			glUniformMatrix4fv(program.projectionUniform, 1, GL_FALSE, &projectionMat[0][0]);
		program.projectionUniformAge = projectionMatAge;
	}
}

void updateProgramModelViewTransform(GLSLProgram &program)
{
	if(program.modelViewUniformAge != modelMatAge)
	{
		//logMsg("updating model/view matrix for program %d (age was %d, now %d)", program.program(), program.modelViewUniformAge, modelMatAge);
		if(likely(program.modelViewUniform != -1))
			glUniformMatrix4fv(program.modelViewUniform, 1, GL_FALSE, &modelMat[0][0]);
		program.modelViewUniformAge = modelMatAge;
	}
}
#endif

void loadTranslate(TransformCoordinate x, TransformCoordinate y, TransformCoordinate z)
{
	loadTransform(Mat4::makeTranslate({x, y, z}));
}

void loadIdentTransform()
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
		glLoadIdentity();
	#endif
	if(!useFixedFunctionPipeline)
		loadTransform({});
}

}
