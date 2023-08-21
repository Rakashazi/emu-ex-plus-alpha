#pragma once

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

#include <imagine/gfx/defs.hh>
#include <span>
#include <string_view>

#ifdef CONFIG_GFX_OPENGL
#include <imagine/gfx/opengl/GLSLProgram.hh>
#endif

namespace IG::Gfx
{

class RendererTask;
class Mat4;

class Shader : public ShaderImpl
{
public:
	enum class CompileMode
	{
		NORMAL, COMPAT
	};

	using ShaderImpl::ShaderImpl;
	Shader(RendererTask &, std::span<std::string_view> srcs, ShaderType type, CompileMode mode = CompileMode::NORMAL);
	Shader(RendererTask &, std::string_view src, ShaderType type, CompileMode mode = CompileMode::NORMAL);
	explicit operator bool() const;
};

struct UniformLocationDesc
{
	const char *name;
	int *locationPtr;
};

struct ProgramFlags
{
	uint8_t
	hasColor:1{},
	hasTexture:1{};
};

class Program : public ProgramImpl
{
public:
	using ProgramImpl::ProgramImpl;
	Program(RendererTask &, NativeShader vShader, NativeShader fShader,
		ProgramFlags, std::span<UniformLocationDesc>);
	int uniformLocation(const char *name);
	void uniform(int location, float v1);
	void uniform(int location, float v1, float v2);
	void uniform(int location, float v1, float v2, float v3);
	void uniform(int location, float v1, float v2, float v3, float v4);
	void uniform(int location, int v1);
	void uniform(int location, int v1, int v2);
	void uniform(int location, int v1, int v2, int v3);
	void uniform(int location, int v1, int v2, int v3, int v4);
	void uniform(int location, Mat4);
	explicit operator bool() const;
};

}
