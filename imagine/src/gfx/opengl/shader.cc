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
#include <imagine/gfx/RendererTask.hh>
#if __ANDROID__
#include <imagine/base/android/android.hh>
#endif
#include "private.hh"

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

#ifdef CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
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

bool GLSLProgram::init(RendererTask &rTask, Shader vShader, Shader fShader, bool hasColor, bool hasTex)
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

void GLSLProgram::deinit(RendererTask &rTask)
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

bool GLSLProgram::link(RendererTask &rTask)
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

GLSLProgram::operator bool() const
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	return program_;
	#else
	return true;
	#endif
}

bool Program::init(RendererTask &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	return GLSLProgram::init(r, vShader, fShader, hasColor, hasTex);
}

void Program::deinit(RendererTask &r)
{
	GLSLProgram::deinit(r);
}

bool Program::link(RendererTask &r)
{
	return GLSLProgram::link(r);
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

Shader Renderer::makeShader(const char **src, uint32_t srcCount, uint32_t type)
{
	GLuint shader;
	task().runSync(
		[this, &shader, src, srcCount, type]()
		{
			shader = glCreateShader(type);
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

Shader Renderer::makeShader(const char *src, uint32_t type)
{
	const char *singleSrc[]{src};
	return makeShader(singleSrc, 1, type);
}

Shader Renderer::makeCompatShader(const char **mainSrc, uint32_t mainSrcCount, uint32_t type)
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
		"#define FRAGCOLOR_DEF out lowp vec4 FRAGCOLOR;\n"
	};
	bool legacyGLSL = support.useLegacyGLSL;
	src[0] = legacyGLSL ? "" : version;
	if(type == GL_VERTEX_SHADER)
		src[1] = legacyGLSL ? legacyVertDefs : "";
	else
		src[1] = legacyGLSL ? legacyFragDefs : fragDefs;
	memcpy(&src[2], &mainSrc[0], sizeof(const char *) * mainSrcCount);
	return makeShader(src, srcCount, type);
}

Shader Renderer::makeCompatShader(const char *src, uint32_t type)
{
	const char *singleSrc[]{src};
	return makeCompatShader(singleSrc, 1, type);
}

Shader Renderer::makeDefaultVShader()
{
	if(!defaultVShader)
		defaultVShader = makeCompatShader(vShaderSrc, GL_VERTEX_SHADER);
	return defaultVShader;
}

bool Renderer::makeCommonProgram(CommonProgram program)
{
	auto &rTask = task();
	switch(program)
	{
		case CommonProgram::TEX_REPLACE : return texReplaceProgram.compile(rTask);
		case CommonProgram::TEX_ALPHA_REPLACE : return texAlphaReplaceProgram.compile(rTask);
		case CommonProgram::TEX : return texProgram.compile(rTask);
		case CommonProgram::TEX_ALPHA : return texAlphaProgram.compile(rTask);
		#ifdef __ANDROID__
		case CommonProgram::TEX_EXTERNAL_REPLACE : return texExternalReplaceProgram.compile(rTask);
		case CommonProgram::TEX_EXTERNAL : return texExternalProgram.compile(rTask);
		#endif
		case CommonProgram::NO_TEX : return noTexProgram.compile(rTask);
		default:
			bug_unreachable("program:%d", (int)program);
			return false;
	}
}

void GLRenderer::useCommonProgram(RendererCommands &cmds, CommonProgram program, const Mat4 *modelMat)
{
	switch(program)
	{
		bcase CommonProgram::TEX_REPLACE: texReplaceProgram.use(cmds, modelMat);
		bcase CommonProgram::TEX_ALPHA_REPLACE: texAlphaReplaceProgram.use(cmds, modelMat);
		#ifdef __ANDROID__
		bcase CommonProgram::TEX_EXTERNAL_REPLACE: texExternalProgram.use(cmds, modelMat);
		#endif
		bcase CommonProgram::TEX: texProgram.use(cmds, modelMat);
		bcase CommonProgram::TEX_ALPHA: texAlphaProgram.use(cmds, modelMat);
		#ifdef __ANDROID__
		bcase CommonProgram::TEX_EXTERNAL: texExternalProgram.use(cmds, modelMat);
		#endif
		bcase CommonProgram::NO_TEX: noTexProgram.use(cmds, modelMat);
		bdefault: bug_unreachable("program:%d", (int)program);
	}
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
		[p = program.program(), uniformLocation, v1, v2]()
		{
			setGLProgram(p);
			runGLCheckedVerbose([&]()
			{
				glUniform2f(uniformLocation, v1, v2);
			}, "glUniform2f()");
		});
}

template <class T>
static bool compileDefaultProgram(RendererTask &r, T &prog, const char **fragSrc, uint32_t fragSrcCount)
{
	assert(fragSrc);
	auto vShader = r.renderer().makeDefaultVShader();
	assert(vShader);
	auto fShader = r.renderer().makeCompatShader(fragSrc, fragSrcCount, GL_FRAGMENT_SHADER);
	if(!fShader)
	{
		return false;
	}
	prog.init(r, vShader, fShader);
	assert(prog.program());
	return true;
}

template <class T>
static bool compileDefaultProgram(RendererTask &r, T &prog, const char *fragSrc)
{
	const char *singleSrc[]{fragSrc};
	return compileDefaultProgram(r, prog, singleSrc, 1);
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

bool DefaultTexProgram::compile(RendererTask &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	logMsg("making texture program");
	compileDefaultProgram(r, *this, texFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		cmds.glcEnable(GL_TEXTURE_2D);
		cmds.setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat)
		cmds.setProgram(*(Program*)this, *modelMat);
	else
		cmds.setProgram(*(Program*)this);
	#endif
}

bool DefaultTexReplaceProgram::compile(RendererTask &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	logMsg("making texture program (replace mode)");
	compileDefaultProgram(r, *this, texReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		cmds.glcEnable(GL_TEXTURE_2D);
		cmds.setImgMode(IMG_MODE_REPLACE);
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat)
		cmds.setProgram(*(Program*)this, *modelMat);
	else
		cmds.setProgram(*(Program*)this);
	#endif
}

bool DefaultTexAlphaReplaceProgram::compile(RendererTask &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	if(r.renderer().support.hasTextureSwizzle)
	{
		auto compiled = r.renderer().texProgram.compile(r);
		impl = &r.renderer().texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program (replace mode)");
	compileDefaultProgram(r, *this, texAlphaReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		cmds.glcEnable(GL_TEXTURE_2D);
		cmds.setImgMode(IMG_MODE_REPLACE);
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(impl)
		impl->use(cmds, modelMat);
	else
	{
		if(modelMat)
			cmds.setProgram(*(Program*)this, *modelMat);
		else
			cmds.setProgram(*(Program*)this);
	}
	#endif
}

bool DefaultTexAlphaProgram::compile(RendererTask &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	if(r.renderer().support.hasTextureSwizzle)
	{
		auto compiled = r.renderer().texProgram.compile(r);
		impl = &r.renderer().texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program");
	compileDefaultProgram(r, *this, texAlphaFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		cmds.glcEnable(GL_TEXTURE_2D);
		cmds.setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(impl)
		impl->use(cmds, modelMat);
	else
	{
		if(modelMat)
			cmds.setProgram(*(Program*)this, *modelMat);
		else
			cmds.setProgram(*(Program*)this);
	}
	#endif
}

bool DefaultTexExternalReplaceProgram::compile(RendererTask &r)
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	assert(Base::androidSDK() >= 14);
	logMsg("making external texture program (replace mode)");
	compileDefaultProgram(r, *this, texExternalReplaceFragShaderSrc);
	if(!program())
	{
		// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
		logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
		const char *fragSrc[]{"#define texture2D texture\n", texExternalReplaceFragShaderSrc};
		compileDefaultProgram(r, *this, fragSrc, std::size(fragSrc));
	}
	return true;
	#else
	return false;
	#endif
};

void DefaultTexExternalReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		bug_unreachable("external texture program not supported");
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat)
		cmds.setProgram(*(Program*)this, *modelMat);
	else
		cmds.setProgram(*(Program*)this);
	#endif
}

bool DefaultTexExternalProgram::compile(RendererTask &r)
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	assert(Base::androidSDK() >= 14);
	logMsg("making external texture program");
	compileDefaultProgram(r, *this, texExternalFragShaderSrc);
	if(!program())
	{
		// Adreno 320 compiler missing texture2D for external textures with GLSL 3.0 ES
		logWarn("retrying compile with Adreno GLSL 3.0 ES work-around");
		const char *fragSrc[]{"#define texture2D texture\n", texExternalFragShaderSrc};
		compileDefaultProgram(r, *this, fragSrc, std::size(fragSrc));
	}
	return true;
	#else
	return false;
	#endif
};

void DefaultTexExternalProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		bug_unreachable("external texture program not supported");
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat)
		cmds.setProgram(*(Program*)this, *modelMat);
	else
		cmds.setProgram(*(Program*)this);
	#endif
}

bool DefaultColorProgram::compile(RendererTask &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.renderer().support.useFixedFunctionPipeline)
		return false;
	logMsg("making color shaded program");
	compileDefaultProgram(r, *this, noTexFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultColorProgram::use(RendererCommands &cmds, const Mat4 *modelMat) const
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(cmds.renderer().support.useFixedFunctionPipeline)
	{
		cmds.glcDisable(GL_TEXTURE_2D);
		cmds.setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			cmds.loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(modelMat)
		cmds.setProgram(*(Program*)this, *modelMat);
	else
		cmds.setProgram(*(Program*)this);
	#endif
}

}
