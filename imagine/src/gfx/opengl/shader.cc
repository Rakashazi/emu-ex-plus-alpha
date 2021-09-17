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
#include "internalDefs.hh"
#include "utils.hh"
#include <cstring>

namespace Gfx
{

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE

static const char *vShaderSrc =
"in vec4 pos; "
"in vec4 color; "
"in vec2 texUV; "
"out vec4 colorOut; "
"out vec2 texUVOut; "
"uniform mat4 modelviewproj; "
"void main() { "
	"colorOut = color; "
	"texUVOut = texUV; "
	"gl_Position = modelviewproj * pos; "
"}"
;

static const char *texFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	"FRAGCOLOR = colorOut * texture(tex, texUVOut); "
"}"
;

static const char *texReplaceFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	"FRAGCOLOR = texture(tex, texUVOut); "
"}"
;

static const char *texAlphaFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	// adapted from: gl_FragColor = colorOut * vec4(1., 1., 1., texture2D(tex, texUVOut).[alpha]);
	"lowp vec4 tmp; "
	"tmp.rgb = colorOut.rgb; "
	"tmp.a = colorOut.a * texture(tex, texUVOut).a; "
	"FRAGCOLOR = tmp;"
"}"
;

static const char *texAlphaReplaceFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	"lowp vec4 tmp; "
	"tmp.rgb = colorOut.rgb; "
	"tmp.a = texture(tex, texUVOut).a; "
	"FRAGCOLOR = tmp;"
"}"
;

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
static const char *texExternalFragShaderSrc =
"#extension GL_OES_EGL_image_external : enable\n"
"#extension GL_OES_EGL_image_external_essl3 : enable\n"
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform lowp samplerExternalOES tex; "
"void main() { "
	"FRAGCOLOR = colorOut * texture2D(tex, texUVOut); "
"}"
;

static const char *texExternalReplaceFragShaderSrc =
"#extension GL_OES_EGL_image_external : enable\n"
"#extension GL_OES_EGL_image_external_essl3 : enable\n"
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform lowp samplerExternalOES tex; "
"void main() { "
	"FRAGCOLOR = texture2D(tex, texUVOut); "
"}"
;
#endif

static const char *noTexFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"void main() { "
	"FRAGCOLOR = colorOut; "
"}"
;

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
	GLint loc;
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

static GLuint makeGLShader(RendererTask &rTask, std::span<const char *> srcs, ShaderType type)
{
	GLuint shaderOut{};
	rTask.runSync(
		[&shaderOut, srcs, type]()
		{
			auto shader = glCreateShader((GLenum)type);
			glShaderSource(shader, srcs.size(), srcs.data(), nullptr);
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
				if(Config::DEBUG_BUILD)
				{
					logErr("failed shader source:");
					iterateTimes(srcs.size(), i)
					{
						logger_printfn(LOG_E, "[part %u]", i);
						logger_printfn(LOG_E, "%s", srcs[i]);
					}
				}
			}
			else
			{
				shaderOut = shader;
			}
		});
	if(shaderOut)
		logMsg("made shader:%d", shaderOut);
	return shaderOut;
}

static GLuint makeCompatGLShader(RendererTask &rTask, std::span<const char *> srcs, ShaderType type)
{
	const uint32_t srcCount = srcs.size() + 2;
	const char *compatSrcs[srcCount];
	const char *version = Config::Gfx::OPENGL_ES ? "#version 300 es\n" : "#version 330\n";
	const char legacyVertDefs[] // for GL ES 2.0
	{
		"#define in attribute\n"
		"#define out varying\n"
	};
	const char legacyFragDefs[] // for GL ES 2.0
	{
		"#define in varying\n"
		"#define texture texture2D\n"
		"#define FRAGCOLOR_DEF\n"
		"#define FRAGCOLOR gl_FragColor\n"
	};
	const char fragDefs[]
	{
		"#define FRAGCOLOR_DEF out mediump vec4 FRAGCOLOR;\n"
	};
	bool legacyGLSL = rTask.renderer().support.useLegacyGLSL;
	compatSrcs[0] = legacyGLSL ? "" : version;
	if(type == ShaderType::VERTEX)
		compatSrcs[1] = legacyGLSL ? legacyVertDefs : "";
	else
		compatSrcs[1] = legacyGLSL ? legacyFragDefs : fragDefs;
	IG::copy_n_r(srcs.data(), srcs.size(), &compatSrcs[2]);
	return makeGLShader(rTask, {compatSrcs, srcCount}, type);
}

static GLuint makeGLShader(RendererTask &rTask, std::span<const char *> srcs, ShaderType type, bool compatMode)
{
	if(compatMode)
		return makeCompatGLShader(rTask, srcs, type);
	else
		return makeGLShader(rTask, srcs, type);
}

Shader::Shader(RendererTask &rTask, std::span<const char *> srcs, ShaderType type, bool compatMode):
	UniqueGLShader{makeGLShader(rTask, srcs, type, compatMode), {&rTask}}
{}

Shader::Shader(RendererTask &rTask, const char *src, ShaderType type, bool compatMode):
	Shader{rTask, {&src, 1}, type, compatMode} {}

Shader Renderer::makeShader(std::span<const char *> srcs, ShaderType type)
{
	return {task(), srcs, type};
}

Shader Renderer::makeShader(const char *src, ShaderType type)
{
	return {task(), {&src, 1}, type};
}

Shader Renderer::makeCompatShader(std::span<const char *> srcs, ShaderType type)
{
	return {task(), srcs, type, true};
}

Shader Renderer::makeCompatShader(const char *src, ShaderType type)
{
	return {task(), {&src, 1}, type, true};
}

NativeShader Renderer::defaultVShader()
{
	if(!defaultVShader_)
		defaultVShader_ = makeCompatGLShader(task(), {&vShaderSrc, 1}, ShaderType::VERTEX);
	return defaultVShader_;
}

static bool linkCommonProgram(RendererTask &rTask, NativeProgramBundle &bundle, const char **fragSrc, uint32_t fragSrcCount, bool hasTex)
{
	assert(fragSrc);
	auto vShader = rTask.renderer().defaultVShader();
	assert(vShader);
	Shader fShader{rTask, {fragSrc, fragSrcCount}, ShaderType::FRAGMENT, true};
	if(!fShader)
	{
		return false;
	}
	Program newProg{rTask, vShader, fShader, true, hasTex};
	assert(newProg.glProgram());
	bundle = newProg.releaseProgramBundle();
	return true;
}

static bool linkCommonProgram(RendererTask &rTask, NativeProgramBundle &bundle, const char *fragSrc, bool hasTex, const char *progName)
{
	if(bundle.program)
		return false;
	logMsg("making %s program", progName);
	const char *singleSrc[]{fragSrc};
	return linkCommonProgram(rTask, bundle, singleSrc, 1, hasTex);
}

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
static bool linkCommonExternalTextureProgram(RendererTask &rTask, NativeProgramBundle &bundle, const char *fragSrc, const char *progName)
{
	assert(rTask.appContext().androidSDK() >= 14);
	bool compiled = linkCommonProgram(rTask, bundle, fragSrc, true, progName);
	if(!bundle.program())
	{
		// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
		logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
		const char *workaroundFragSrc[]{"#define texture2D texture\n", fragSrc};
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
			return commonProgram.noTex;
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
			return false;
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
				cmds.setImgMode(IMG_MODE_REPLACE);
			bcase CommonProgram::TEX:
			bcase CommonProgram::TEX_ALPHA:
				cmds.glcEnable(GL_TEXTURE_2D);
				cmds.setImgMode(IMG_MODE_MODULATE);
			bcase CommonProgram::NO_TEX:
				cmds.glcDisable(GL_TEXTURE_2D);
				cmds.setImgMode(IMG_MODE_MODULATE);
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
