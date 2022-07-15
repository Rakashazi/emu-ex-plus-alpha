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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/util/container/ArrayList.hh>
#include <imagine/util/format.hh>
#include "internalDefs.hh"
#include "utils.hh"
#include <cstring>

namespace IG::Gfx
{

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE

static constexpr size_t maxSourceStrings = 16;

using namespace std::literals;

static std::string_view vShaderSrc =
R"(in vec4 pos;
in vec4 color;
in vec2 texUV;
out vec4 colorOut;
out vec2 texUVOut;
uniform mat4 modelviewproj;
void main() {
	colorOut = color;
	texUVOut = texUV;
	gl_Position = modelviewproj * pos;
})";

static std::string_view texFragShaderSrc =
R"(FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform sampler2D tex;
void main() {
	FRAGCOLOR = colorOut * texture(tex, texUVOut);
})";

static std::string_view texReplaceFragShaderSrc =
R"(FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform sampler2D tex;
void main() {
	FRAGCOLOR = texture(tex, texUVOut);
})";

static std::string_view texAlphaFragShaderSrc =
R"(FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform sampler2D tex;
void main() {
	lowp vec4 tmp;
	tmp.rgb = colorOut.rgb;
	tmp.a = colorOut.a * texture(tex, texUVOut).a;
	FRAGCOLOR = tmp;
})"; // adapted from: gl_FragColor = colorOut * vec4(1., 1., 1., texture2D(tex, texUVOut).[alpha]);

static std::string_view texAlphaReplaceFragShaderSrc =
R"(FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform sampler2D tex;
void main() {
	lowp vec4 tmp;
	tmp.rgb = colorOut.rgb;
	tmp.a = texture(tex, texUVOut).a;
	FRAGCOLOR = tmp;
})";

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
static std::string_view texExternalFragShaderSrc =
R"(#extension GL_OES_EGL_image_external : enable
#extension GL_OES_EGL_image_external_essl3 : enable
FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform lowp samplerExternalOES tex;
void main() {
	FRAGCOLOR = colorOut * texture2D(tex, texUVOut);
})";

static std::string_view texExternalReplaceFragShaderSrc =
R"(#extension GL_OES_EGL_image_external : enable
#extension GL_OES_EGL_image_external_essl3 : enable
FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
uniform lowp samplerExternalOES tex;
void main() {
	FRAGCOLOR = texture2D(tex, texUVOut);
})";
#endif

static std::string_view noTexFragShaderSrc =
R"(FRAGCOLOR_DEF
in lowp vec4 colorOut;
in lowp vec2 texUVOut;
void main() {
	FRAGCOLOR = colorOut;
})";

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

Program::Program(RendererTask &rTask, NativeShader vShader, NativeShader fShader, bool hasColor, bool hasTex)
{
	GLuint programOut{};
	rTask.runSync(
		[&programOut, &mvpUniform = mvpUniform, vShader, fShader, hasColor, hasTex]()
		{
			auto program = makeGLProgram(vShader, fShader);
			if(!program) [[unlikely]]
				return;
			runGLChecked(
				[&]()
				{
					glBindAttribLocation(program, VATTR_POS, "pos");
				}, "glBindAttribLocation(..., pos)");
			if(hasColor)
			{
				runGLChecked(
					[&]()
					{
						glBindAttribLocation(program, VATTR_COLOR, "color");
					}, "glBindAttribLocation(..., color)");
			}
			if(hasTex)
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
			glDetachShader(program, vShader);
			glDetachShader(program, fShader);
			runGLChecked([&]()
			{
				mvpUniform = glGetUniformLocation(program, "modelviewproj");
			}, "glGetUniformLocation(modelviewproj)");
			programOut = program;
		});
	program_ = {programOut, {&rTask}};
	if(programOut)
		logMsg("made program:%d", programOut);
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
	runGLCheckedVerbose([&]()
	{
		glUseProgram(program);
	}, "glUseProgram()");
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
			StaticArrayList<const GLchar*, maxSourceStrings> srcStrings{};
			StaticArrayList<GLint, maxSourceStrings> srcSizes{};
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
						logger_printfn(LOG_E, "%s", fmt::format("{}", s).c_str());
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
	StaticArrayList<std::string_view, maxSourceStrings> compatSrcs{};
	std::string_view version = Config::Gfx::OPENGL_ES ? "#version 300 es\n"sv : "#version 330\n"sv;
	std::string_view legacyVertDefs // for GL ES 2.0
	{
		"#define in attribute\n"
		"#define out varying\n"
	};
	std::string_view legacyFragDefs // for GL ES 2.0
	{
		"#define in varying\n"
		"#define texture texture2D\n"
		"#define FRAGCOLOR_DEF\n"
		"#define FRAGCOLOR gl_FragColor\n"
	};
	std::string_view fragDefs
	{
		"#define FRAGCOLOR_DEF out mediump vec4 FRAGCOLOR;\n"
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

static GLuint makeGLShader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type, bool compatMode)
{
	if(compatMode)
		return makeCompatGLShader(rTask, srcs, type);
	else
		return makeGLShader(rTask, srcs, type);
}

Shader::Shader(RendererTask &rTask, std::span<std::string_view> srcs, ShaderType type, bool compatMode):
	UniqueGLShader{makeGLShader(rTask, srcs, type, compatMode), {&rTask}}
{}

Shader::Shader(RendererTask &rTask, std::string_view src, ShaderType type, bool compatMode):
	Shader{rTask, {&src, 1}, type, compatMode} {}

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
	return {task(), srcs, type, true};
}

Shader Renderer::makeCompatShader(std::string_view src, ShaderType type)
{
	return {task(), {&src, 1}, type, true};
}

NativeShader Renderer::defaultVShader()
{
	if(!defaultVShader_)
		defaultVShader_ = makeCompatGLShader(task(), {&vShaderSrc, 1}, ShaderType::VERTEX);
	return defaultVShader_;
}

static bool linkCommonProgram(RendererTask &rTask, NativeProgramBundle &bundle, std::span<std::string_view> fragSrcs, bool hasTex)
{
	assert(fragSrcs.size());
	auto vShader = rTask.renderer().defaultVShader();
	assert(vShader);
	Shader fShader{rTask, fragSrcs, ShaderType::FRAGMENT, true};
	if(!fShader)
	{
		return false;
	}
	Program newProg{rTask, vShader, fShader, true, hasTex};
	assert(newProg.glProgram());
	bundle = newProg.releaseProgramBundle();
	return true;
}

static bool linkCommonProgram(RendererTask &rTask, NativeProgramBundle &bundle, std::string_view fragSrc, bool hasTex, const char *progName)
{
	if(bundle.program)
		return false;
	logMsg("making %s program", progName);
	return linkCommonProgram(rTask, bundle, {&fragSrc, 1}, hasTex);
}

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
static bool linkCommonExternalTextureProgram(RendererTask &rTask, NativeProgramBundle &bundle, std::string_view fragSrc, const char *progName)
{
	assert(rTask.appContext().androidSDK() >= 14);
	bool compiled = linkCommonProgram(rTask, bundle, fragSrc, true, progName);
	if(!bundle.program())
	{
		// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
		logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
		std::string_view workaroundFragSrc[]{"#define texture2D texture\n"sv, fragSrc};
		return linkCommonProgram(rTask, bundle, workaroundFragSrc, std::size(workaroundFragSrc), true);
	}
	return compiled;
}
#endif

NativeProgramBundle GLRenderer::commonProgramBundle(CommonProgram program) const
{
	switch(program)
	{
		case CommonProgram::TEX_REPLACE: return commonProgram.texReplace;
		case CommonProgram::TEX_ALPHA_REPLACE: return commonProgram.texAlphaReplace;
		#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
		case CommonProgram::TEX_EXTERNAL_REPLACE: return commonProgram.texExternalReplace;
		#endif
		case CommonProgram::TEX: return commonProgram.tex;
		case CommonProgram::TEX_ALPHA: return commonProgram.texAlpha;
		#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
		case CommonProgram::TEX_EXTERNAL: return commonProgram.texExternal;
		#endif
		case CommonProgram::NO_TEX: return commonProgram.noTex;
		default: bug_unreachable("program:%d", (int)program);
	}
}

bool Renderer::makeCommonProgram(CommonProgram program)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(support.useFixedFunctionPipeline)
		return false;
	auto &rTask = task();
	switch(program)
	{
		case CommonProgram::TEX_REPLACE :
			return linkCommonProgram(rTask, commonProgram.texReplace, texReplaceFragShaderSrc, true, "texture (replace mode)");
		case CommonProgram::TEX_ALPHA_REPLACE :
			if(support.hasTextureSwizzle)
			{
				bool compiled = makeCommonProgram(CommonProgram::TEX_REPLACE);
				commonProgram.texAlphaReplace = commonProgram.texReplace;
				return compiled;
			}
			return linkCommonProgram(rTask, commonProgram.texAlphaReplace, texAlphaReplaceFragShaderSrc, true, "alpha-only texture (replace mode)");
		case CommonProgram::TEX :
			return linkCommonProgram(rTask, commonProgram.tex, texFragShaderSrc, true, "texture");
		case CommonProgram::TEX_ALPHA :
			if(support.hasTextureSwizzle)
			{
				bool compiled = makeCommonProgram(CommonProgram::TEX);
				commonProgram.texAlpha = commonProgram.tex;
				return compiled;
			}
			return linkCommonProgram(rTask, commonProgram.texAlpha, texAlphaFragShaderSrc, true, "alpha-only texture");
		#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
		case CommonProgram::TEX_EXTERNAL_REPLACE :
			return linkCommonExternalTextureProgram(rTask, commonProgram.texExternalReplace, texExternalReplaceFragShaderSrc, "external texture (replace mode)");
		case CommonProgram::TEX_EXTERNAL :
			return linkCommonExternalTextureProgram(rTask, commonProgram.texExternal, texExternalFragShaderSrc, "external texture");
		#endif
		case CommonProgram::NO_TEX :
			return linkCommonProgram(rTask, commonProgram.noTex, noTexFragShaderSrc, false, "color shaded");
		default:
			bug_unreachable("program:%d", (int)program);
	}
	#else
	return false;
	#endif
}

bool Renderer::commonProgramIsCompiled(CommonProgram program) const
{
	return commonProgramBundle(program).program;
}

void GLRenderer::useCommonProgram(RendererCommands &cmds, CommonProgram program, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		switch(program)
		{
			bcase CommonProgram::TEX_REPLACE:
			bcase CommonProgram::TEX_ALPHA_REPLACE:
				cmds.glcEnable(GL_TEXTURE_2D);
				cmds.setImgMode(EnvMode::REPLACE);
			bcase CommonProgram::TEX:
			bcase CommonProgram::TEX_ALPHA:
				cmds.glcEnable(GL_TEXTURE_2D);
				cmds.setImgMode(EnvMode::MODULATE);
			bcase CommonProgram::NO_TEX:
				cmds.glcDisable(GL_TEXTURE_2D);
			bdefault: bug_unreachable("program:%d", (int)program);
		}
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	cmds.GLRendererCommands::setProgram(commonProgramBundle(program), modelMat);
	#endif
}

void Renderer::uniformF(Program &program, int uniformLocation, float v1, float v2)
{
	task().run(
		[p = program.glProgram(), uniformLocation, v1, v2]()
		{
			setGLProgram(p);
			runGLCheckedVerbose([&]()
			{
				glUniform2f(uniformLocation, v1, v2);
			}, "glUniform2f()");
		});
}

#else

void uniformF(int uniformLocation, float v1, float v2)
{
	bug_unreachable("called uniformF() without shader support");
}

void setProgram(Program &program)
{
	bug_unreachable("called setProgram() without shader support");
}

void setProgram(Program &program, Mat4 modelMat)
{
	bug_unreachable("called setProgram() without shader support");
}

void Program::deinit() {}

void deleteShader(Shader shader) {}

#endif

}
