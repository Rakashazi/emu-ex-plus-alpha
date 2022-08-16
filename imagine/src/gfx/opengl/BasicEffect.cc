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

#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/Projection.hh>

namespace IG::Gfx
{

void GLBasicEffect::updateTextureType(RendererCommands &cmds, TextureType newTexType)
{
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		if(newTexType == TextureType::UNSET)
		{
			cmds.glcDisable(GL_TEXTURE_2D);
		}
		else
		{
			cmds.glcEnable(GL_TEXTURE_2D);
		}
		return;
	}
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	cmds.setProgram(program);
	if(texType != newTexType)
	{
		if(newTexType == TextureType::UNSET)
		{
			glUniform1i(textureModeUniform, 0); // texture off
		}
		else if(newTexType == TextureType::T2D_1 && !cmds.renderer().support.hasTextureSwizzle)
		{
			glUniform1i(textureModeUniform, 2); // alpha texture with swizzle
		}
		else
		{
			glUniform1i(textureModeUniform, 1); // normal texture
		}
		texType = newTexType;
	}
	#endif
}

void BasicEffect::enableTexture(RendererCommands &cmds, const Texture &tex)
{
	updateTextureType(cmds, tex.type());
	cmds.set(tex.binding());
}

void BasicEffect::enableTexture(RendererCommands &cmds, TextureBinding texBinding)
{
	enableTexture(cmds);
	cmds.set(texBinding);
}

void BasicEffect::enableTexture(RendererCommands &cmds)
{
	updateTextureType(cmds, TextureType::T2D_4);
}

void BasicEffect::enableAlphaTexture(RendererCommands &cmds)
{
	updateTextureType(cmds, TextureType::T2D_1);
}

void BasicEffect::disableTexture(RendererCommands &cmds)
{
	updateTextureType(cmds, TextureType::UNSET);
}

void BasicEffect::setModelView(RendererCommands &cmds, Mat4 mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		glLoadMatrixf(&mat[0][0]);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	cmds.setProgram(program);
	cmds.uniform(modelViewUniform, mat);
	#endif
}

void BasicEffect::setProjection(RendererCommands &cmds, Mat4 mat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(&mat[0][0]);
		glMatrixMode(GL_MODELVIEW);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	cmds.setProgram(program);
	cmds.uniform(projUniform, mat);
	#endif
}

void BasicEffect::setModelViewProjection(RendererCommands &cmds, Mat4 modelView, Mat4 proj)
{
	setProjection(cmds, proj);
	setModelView(cmds, modelView);
}

void BasicEffect::setModelViewProjection(RendererCommands &cmds, Projection proj)
{
	setProjection(cmds, proj.matrix());
	setModelView(cmds, proj.plane().makeTranslate());
}

void BasicEffect::prepareDraw(RendererCommands &cmds)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
		return;
	#endif
	cmds.setProgram(program);
}

}
