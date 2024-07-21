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

#define LOGTAG "GLShader"
#include <imagine/gfx/Program.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/Mat4.hh>
#include <imagine/util/container/ArrayList.hh>
#include "internalDefs.hh"
#include "utils.hh"
#include <cstring>
#include <format>

namespace IG::Gfx
{

static constexpr size_t maxSourceStrings = 16;

using namespace std::literals;

static GLuint makeGLProgram(GLuint vShader, GLuint fShader)
{
	auto program = glCreateProgram();
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	return program;
}

static bool linkGLProgram(GLuint program)
{
	runGLChecked(
		[&]()
		{
			glLinkProgram(program);
		}, "glLinkProgram()");
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(Config::DEBUG_BUILD)
	{
		GLchar messages[4096];
		glGetProgramInfoLog(program, sizeof(messages), nullptr, messages);
		if(strlen(messages))
			logDMsg("linker info log: %s", messages);
	}
	if(success == GL_FALSE)
		return false;
	return true;
}

void destroyGLShader(RendererTask &rTask, NativeShader s)
{
	if(!s)
		return;
	logMsg("deleting shader:%d", (int)s);
	rTask.run(
		[shader = s]()
		{
			runGLChecked(
				[&]()
				{
					glDeleteShader(shader);
				}, "glDeleteShader()");
		});
}

Shader::operator bool() const
{
	return get();
}

void destroyGLProgram(RendererTask &rTask, NativeProgram p)
{
	if(!p)
		return;
	logMsg("deleting program:%d", (int)p);
	rTask.run(
		[program = p]()
		{
			runGLChecked(
				[&]()
				{
					glDeleteProgram(program);
				}, "glDeleteProgram()");
		});
}

Program::Program(RendererTask &rTask, NativeShader vShader, NativeShader fShader,
	ProgramFlags flags, std::span<UniformLocationDesc> uniformDescs)
{
	GLuint programOut{};
	rTask.runSync(
		[=, &programOut]()
		{
			auto program = makeGLProgram(vShader, fShader);
			if(!program) [[unlikely]]
				return;
			runGLChecked(
				[&]()
				{
					glBindAttribLocation(program, VATTR_POS, "pos");
				}, "glBindAttribLocation(..., pos)");
			if(flags.hasColor)
			{
				runGLChecked(
					[&]()
					{
						glBindAttribLocation(program, VATTR_COLOR, "color");
					}, "glBindAttribLocation(..., color)");
			}
			if(flags.hasTexture)
			{
				runGLChecked(
					[&]()
					{
						glBindAttribLocation(program, VATTR_TEX_UV, "texUV");
					}, "glBindAttribLocation(..., texUV)");
			}
			if(!linkGLProgram(program))
			{
				glDeleteProgram(program);
				return;
			}
			logMsg("made program:%d", program);
			glDetachShader(program, vShader);
			glDetachShader(program, fShader);
			for(auto desc : uniformDescs)
			{
				runGLChecked([&]()
				{
					*desc.locationPtr = glGetUniformLocation(program, desc.name);
				}, "glGetUniformLocation()");
				logMsg("uniform:%s location:%d", desc.name, *desc.locationPtr);
			}
			programOut = program;
		});
	program_ = {programOut, {&rTask}};
}

Program::operator bool() const
{
	return program_.get();
}

int Program::uniformLocation(const char *uniformName)
{
	GLint loc{};
	task().runSync(
		[program = (GLuint)program_, &loc, uniformName]()
		{
			runGLChecked([&]()
			{
				loc = glGetUniformLocation(program, uniformName);
			}, "glGetUniformLocation()");
		});
	return loc;
}

static void setGLProgram(GLuint program)
{
	runGLCheckedVerbose([&]() { glUseProgram(program); }, "glUseProgram()");
}

void Program::uniform(int loc, float v1)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform1f(loc, v1); }, "glUniform1f()");
	});
}

void Program::uniform(int loc, float v1, float v2)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform2f(loc, v1, v2); }, "glUniform2f()");
	});
}

void Program::uniform(int loc, float v1, float v2, float v3)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform3f(loc, v1, v2, v3); }, "glUniform3f()");
	});
}

void Program::uniform(int loc, float v1, float v2, float v3, float v4)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform4f(loc, v1, v2, v3, v4); }, "glUniform4f()");
	});
}

void Program::uniform(int loc, int v1)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform1i(loc, v1); }, "glUniform1i()");
	});
}

void Program::uniform(int loc, int v1, int v2)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform2i(loc, v1, v2); }, "glUniform2i()");
	});
}

void Program::uniform(int loc, int v1, int v2, int v3)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform3i(loc, v1, v2, v3); }, "glUniform3i()");
	});
}

void Program::uniform(int loc, int v1, int v2, int v3, int v4)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniform4i(loc, v1, v2, v3, v4); }, "glUniform4i()");
	});
}

void Program::uniform(int loc, Mat4 mat)
{
	task().run([=, p = glProgram()]()
	{
		setGLProgram(p);
		runGLCheckedVerbose([&]() { glUniformMatrix4fv(loc, 1, GL_FALSE, &mat[0][0]); }, "glUniformMatrix4fv()");
	});
}

static GLuint makeGLShader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type)
{
	if(srcs.size() > maxSourceStrings) [[unlikely]]
	{
		logErr("%zu source strings is over %zu limit", srcs.size(), maxSourceStrings);
		return 0;
	}
	GLuint shaderOut{};
	rTask.runSync(
		[&shaderOut, srcs, type]()
		{
			auto shader = glCreateShader((GLenum)type);
			StaticArrayList<const GLchar*, maxSourceStrings> srcStrings;
			StaticArrayList<GLint, maxSourceStrings> srcSizes;
			for(auto s : srcs)
			{
				srcStrings.emplace_back(s.data());
				srcSizes.emplace_back(s.size());
			}
			glShaderSource(shader, srcs.size(), srcStrings.data(), srcSizes.data());
			glCompileShader(shader);
			GLint success;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if(Config::DEBUG_BUILD)
			{
				GLchar messages[4096];
				glGetShaderInfoLog(shader, sizeof(messages), nullptr, messages);
				if(strlen(messages))
					logDMsg("shader info log: %s", messages);
			}
			if(success == GL_FALSE)
			{
				if constexpr(Config::DEBUG_BUILD)
				{
					logErr("failed shader source:");
					for(auto &s : srcs)
					{
						logger_printfn(LOG_E, "[part %zu]", (size_t)std::distance(srcs.data(), &s));
						logger_printfn(LOG_E, "%s", std::format("{}", s).c_str());
					}
				}
			}
			else
			{
				shaderOut = shader;
			}
		});
	if(shaderOut)
		logMsg("made %s shader:%d", type == ShaderType::FRAGMENT ? "fragment" : "vertex", shaderOut);
	return shaderOut;
}

static GLuint makeCompatGLShader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type)
{
	if(auto srcCount = srcs.size() + 2;
		srcCount > maxSourceStrings) [[unlikely]]
	{
		logErr("%zu source strings is over %zu limit", srcCount, maxSourceStrings);
		return 0;
	}
	StaticArrayList<std::string_view, maxSourceStrings> compatSrcs;
	std::string_view version = Config::Gfx::OPENGL_ES ? "#version 300 es\n"sv : "#version 330\n"sv;
	constexpr std::string_view legacyVertDefs // for GL ES 2.0
	{
		"#define in attribute\n"
		"#define out varying\n"
	};
	constexpr std::string_view legacyFragDefs // for GL ES 2.0
	{
		"#define in varying\n"
		"#define texture texture2D\n"
		"#define FRAGCOLOR gl_FragColor\n"
	};
	constexpr std::string_view fragDefs
	{
		"out mediump vec4 FRAGCOLOR;\n"
	};
	bool legacyGLSL = rTask.renderer().support.useLegacyGLSL;
	compatSrcs.emplace_back(legacyGLSL ? ""sv : version);
	if(type == ShaderType::VERTEX)
		compatSrcs.emplace_back(legacyGLSL ? legacyVertDefs : ""sv);
	else
		compatSrcs.emplace_back(legacyGLSL ? legacyFragDefs : fragDefs);
	for(auto s : srcs)
	{
		compatSrcs.emplace_back(s);
	}
	return makeGLShader(rTask, compatSrcs, type);
}

static GLuint makeGLShader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type, Shader::CompileMode mode)
{
	if(mode == Shader::CompileMode::COMPAT)
		return makeCompatGLShader(rTask, srcs, type);
	else
		return makeGLShader(rTask, srcs, type);
}

Shader::Shader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type, CompileMode mode):
	UniqueGLShader{makeGLShader(rTask, srcs, type, mode), {&rTask}}
{}

Shader::Shader(RendererTask &rTask, std::string_view src, ShaderType type, CompileMode mode):
	Shader{rTask, {&src, 1}, type, mode} {}

Shader Renderer::makeShader(std::span<std::string_view> srcs, ShaderType type)
{
	return {task(), srcs, type};
}

Shader Renderer::makeShader(std::string_view src, ShaderType type)
{
	return {task(), {&src, 1}, type};
}

Shader Renderer::makeCompatShader(std::span<std::string_view> srcs, ShaderType type)
{
	return {task(), srcs, type, Shader::CompileMode::COMPAT};
}

Shader Renderer::makeCompatShader(std::string_view src, ShaderType type)
{
	return {task(), {&src, 1}, type, Shader::CompileMode::COMPAT};
}

}
