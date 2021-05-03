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

bool Program::init(RendererTask &rTask, Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	if(program_)
		deinit(rTask);
	rTask.runSync(
		[this, vShader, fShader, hasColor, hasTex]()
		{
			program_ = makeGLProgram(vShader, fShader);
			runGLChecked(
				[&]()
				{
					glBindAttribLocation(program_, VATTR_POS, "pos");
				}, "glBindAttribLocation(..., pos)");
			if(hasColor)
			{
				runGLChecked(
					[&]()
					{
						glBindAttribLocation(program_, VATTR_COLOR, "color");
					}, "glBindAttribLocation(..., color)");
			}
			if(hasTex)
			{
				runGLChecked(
					[&]()
					{
						glBindAttribLocation(program_, VATTR_TEX_UV, "texUV");
					}, "glBindAttribLocation(..., texUV)");
			}
		});
	return program_;
}

void Program::deinit(RendererTask &rTask)
{
	if(!program_)
		return;
	logMsg("deleting program:%d", (int)program_);
	rTask.run(
		[program = program_]()
		{
			runGLChecked(
				[&]()
				{
					glDeleteProgram(program);
				}, "glDeleteProgram()");
		});
	program_ = 0;
}

bool Program::link(RendererTask &rTask)
{
	bool success;
	rTask.runSync(
		[this, &success]()
		{
			success = linkGLProgram(program_);
		});
	if(!success)
	{
		deinit(rTask);
		return false;
	}
	initUniforms(rTask);
	return true;
}

GLint GLSLProgram::modelViewProjectionUniform() const
{
	return mvpUniform;
}

GLuint GLSLProgram::glProgram() const { return program_; }

bool GLSLProgram::operator ==(GLSLProgram const &rhs) const
{
	return program_ == rhs.program_;
}

Program::operator bool() const
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	return program_;
	#else
	return true;
	#endif
}

int Program::uniformLocation(RendererTask &rTask, const char *uniformName)
{
	GLint loc;
	rTask.runSync(
		[this, &loc, uniformName]()
		{
			runGLChecked([&]()
			{
				loc = glGetUniformLocation(program_, uniformName);
			}, "glGetUniformLocation()");
		});
	return loc;
}

void GLSLProgram::initUniforms(RendererTask &rTask)
{
	assert(program_);
	rTask.runSync(
		[this]()
		{
			runGLChecked([&]()
			{
				mvpUniform = glGetUniformLocation(program_, "modelviewproj");
			}, "glGetUniformLocation(modelviewproj)");
		});
}

static void setGLProgram(GLuint program)
{
	runGLCheckedVerbose([&]()
	{
		glUseProgram(program);
	}, "glUseProgram()");
}

Shader Renderer::makeShader(const char **src, uint32_t srcCount, ShaderType type)
{
	GLuint shader;
	task().runSync(
		[this, &shader, src, srcCount, type]()
		{
			shader = glCreateShader((GLenum)type);
			glShaderSource(shader, srcCount, src, nullptr);
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
					iterateTimes(srcCount, i)
					{
						logger_printfn(LOG_E, "[part %u]", i);
						logger_printfn(LOG_E, "%s", src[i]);
					}
				}
				shader = 0;
			}
		});
	return shader;
}

Shader Renderer::makeShader(const char *src, ShaderType type)
{
	const char *singleSrc[]{src};
	return makeShader(singleSrc, 1, type);
}

Shader Renderer::makeCompatShader(const char **mainSrc, uint32_t mainSrcCount, ShaderType type)
{
	const uint32_t srcCount = mainSrcCount + 2;
	const char *src[srcCount];
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
	bool legacyGLSL = support.useLegacyGLSL;
	src[0] = legacyGLSL ? "" : version;
	if(type == ShaderType::VERTEX)
		src[1] = legacyGLSL ? legacyVertDefs : "";
	else
		src[1] = legacyGLSL ? legacyFragDefs : fragDefs;
	memcpy(&src[2], &mainSrc[0], sizeof(const char *) * mainSrcCount);
	return makeShader(src, srcCount, type);
}

Shader Renderer::makeCompatShader(const char *src, ShaderType type)
{
	const char *singleSrc[]{src};
	return makeCompatShader(singleSrc, 1, type);
}

Shader Renderer::makeDefaultVShader()
{
	if(!defaultVShader)
		defaultVShader = makeCompatShader(vShaderSrc, ShaderType::VERTEX);
	return defaultVShader;
}

static bool linkCommonProgram(RendererTask &rTask, Program &prog, const char **fragSrc, uint32_t fragSrcCount, bool hasTex)
{
	assert(fragSrc);
	auto vShader = rTask.renderer().makeDefaultVShader();
	assert(vShader);
	auto fShader = rTask.renderer().makeCompatShader(fragSrc, fragSrcCount, ShaderType::FRAGMENT);
	if(!fShader)
	{
		return false;
	}
	prog.init(rTask, vShader, fShader, true, hasTex);
	prog.link(rTask);
	assert(prog.glProgram());
	return true;
}

static bool linkCommonProgram(RendererTask &rTask, Program &prog, const char *fragSrc, bool hasTex, const char *progName)
{
	if(prog.glProgram())
		return false;
	logMsg("making %s program", progName);
	const char *singleSrc[]{fragSrc};
	return linkCommonProgram(rTask, prog, singleSrc, 1, hasTex);
}

#ifdef CONFIG_GFX_OPENGL_TEXTURE_TARGET_EXTERNAL
static bool linkCommonExternalTextureProgram(RendererTask &rTask, Program &prog, const char *fragSrc, const char *progName)
{
	assert(rTask.appContext().androidSDK() >= 14);
	bool compiled = linkCommonProgram(rTask, prog, fragSrc, true, progName);
	if(!prog.program())
	{
		// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
		logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
		const char *workaroundFragSrc[]{"#define texture2D texture\n", fragSrc};
		return linkCommonProgram(rTask, prog, workaroundFragSrc, std::size(workaroundFragSrc), true);
	}
	return compiled;
}
#endif

const Program &GLRenderer::commonProgramRef(CommonProgram program) const
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
	return (bool)commonProgramRef(program);
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
	cmds.setProgram(commonProgramRef(program), modelMat);
	#endif
}

void Renderer::deleteShader(Shader shader)
{
	logMsg("deleting shader:%u", (uint32_t)shader);
	assert(shader != defaultVShader);
	task().run(
		[shader]()
		{
			glDeleteShader(shader);
		});
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
