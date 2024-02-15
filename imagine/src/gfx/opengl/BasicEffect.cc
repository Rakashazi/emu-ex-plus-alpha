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
#include <imagine/gfx/Program.hh>
#include <imagine/gfx/Texture.hh>
#include <imagine/gfx/Mat4.hh>

namespace IG::Gfx
{

bool GLBasicEffect::setShaders(RendererTask &task, std::span<std::string_view> vertSrcs, std::span<std::string_view> fragSrcs)
{
	UniformLocationDesc uniformDescs[]
	{
		{"modelView", &modelViewUniform},
		{"proj", &projUniform},
		{"textureMode", &textureModeUniform},
	};
	Program newProg{task,
		Shader{task, vertSrcs, ShaderType::VERTEX, Shader::CompileMode::COMPAT},
		Shader{task, fragSrcs, ShaderType::FRAGMENT, Shader::CompileMode::COMPAT},
		{.hasColor = true, .hasTexture = true}, uniformDescs};
	if(!newProg) [[unlikely]]
		return false;
	program = newProg.release();
	return true;
}

void GLBasicEffect::updateTextureType(RendererCommands &cmds, TextureType newTexType)
{
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

void GLBasicEffect::setModelView(RendererCommands &cmds, Mat4 mat)
{
	cmds.uniform(modelViewUniform, mat);
}

void GLBasicEffect::setProjection(RendererCommands &cmds, Mat4 mat)
{
	cmds.uniform(projUniform, mat);
}

void BasicEffect::setModelView(RendererCommands &cmds, Mat4 mat)
{
	prepareDraw(cmds);
	GLBasicEffect::setModelView(cmds, mat);
}

void BasicEffect::setProjection(RendererCommands &cmds, Mat4 mat)
{
	prepareDraw(cmds);
	GLBasicEffect::setProjection(cmds, mat);
}

void BasicEffect::setModelViewProjection(RendererCommands &cmds, Mat4 modelView, Mat4 proj)
{
	prepareDraw(cmds);
	GLBasicEffect::setProjection(cmds, proj);
	GLBasicEffect::setModelView(cmds, modelView);
}

void BasicEffect::prepareDraw(RendererCommands &cmds)
{
	cmds.setProgram(program);
}

}
