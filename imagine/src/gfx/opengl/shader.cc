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
#include <imagine/gfx/Gfx.hh>
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

bool GLSLProgram::init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	if(program_)
		deinit(r);
	r.runGLTaskSync(
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

void GLSLProgram::deinit(Renderer &r)
{
	if(program_)
	{
		r.runGLTaskSync(
			[this]()
			{
				logMsg("deleting program %d", (int)program_);
				runGLChecked(
					[&]()
					{
						glDeleteProgram(program_);
					}, "glDeleteProgram()");
				program_ = 0;
			});
	}
}

bool GLSLProgram::link(Renderer &r)
{
	bool success;
	r.runGLTaskSync(
		[this, &success]()
		{
			success = linkGLProgram(program_);
		});
	if(!success)
	{
		deinit(r);
		return false;
	}
	initUniforms(r);
	return true;
}

bool Program::init(Renderer &r, Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	return GLSLProgram::init(r, vShader, fShader, hasColor, hasTex);
}

void Program::deinit(Renderer &r)
{
	GLSLProgram::deinit(r);
}

bool Program::link(Renderer &r)
{
	return GLSLProgram::link(r);
}

int Program::uniformLocation(Renderer &r, const char *uniformName)
{
	GLint loc;
	r.runGLTaskSync(
		[this, &loc, uniformName]()
		{
			runGLChecked([&]()
			{
				loc = glGetUniformLocation(program_, uniformName);
			}, "glGetUniformLocation()");
		});
	return loc;
}

void GLSLProgram::initUniforms(Renderer &r)
{
	assert(program_);
	r.runGLTaskSync(
		[this]()
		{
			runGLChecked([&]()
			{
				modelViewProjectionUniform = glGetUniformLocation(program_, "modelviewproj");
			}, "glGetUniformLocation(modelviewproj)");
		});
}

void TexProgram::init(Renderer &r, GLuint vShader, GLuint fShader)
{
	GLSLProgram::init(r, vShader, fShader, true, true);
	link(r);
}

void ColorProgram::init(Renderer &r, GLuint vShader, GLuint fShader)
{
	GLSLProgram::init(r, vShader, fShader, true, false);
	link(r);
}

static void setGLProgram(GLuint program)
{
	runGLCheckedVerbose([&]()
	{
		glUseProgram(program);
	}, "glUseProgram()");
}

void GLRenderer::setProgram(GLSLProgram &program)
{
	//logMsg("setting program: %d", program.program());
	verifyCurrentResourceContext();
	GLuint p = program.program();
	assert(p);
	setGLProgram(p);
}

Shader Renderer::makeShader(const char **src, uint32_t srcCount, uint32_t type)
{
	GLuint shader;
	runGLTaskSync(
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
			else
			{
				resourceUpdate = true;
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

void Renderer::deleteShader(Shader shader)
{
	logMsg("deleting shader:%u", (uint32_t)shader);
	assert(shader != defaultVShader);
	runGLTask(
		[shader]()
		{
			glDeleteShader(shader);
		});
}

void Renderer::uniformF(Program &program, int uniformLocation, float v1, float v2)
{
	auto p = program.program();
	runGLTask(
		[p, uniformLocation, v1, v2]()
		{
			setGLProgram(p);
			runGLCheckedVerbose([&]()
			{
				glUniform2f(uniformLocation, v1, v2);
			}, "glUniform2f()");
		});
	resourceUpdate = true;
}

template <class T>
static bool compileDefaultProgram(Renderer &r, T &prog, const char **fragSrc, uint32_t fragSrcCount)
{
	assert(fragSrc);
	auto vShader = r.makeDefaultVShader();
	assert(vShader);
	auto fShader = r.makeCompatShader(fragSrc, fragSrcCount, GL_FRAGMENT_SHADER);
	if(!fShader)
	{
		return false;
	}
	prog.init(r, vShader, fShader);
	assert(prog.program());
	return true;
}

template <class T>
static bool compileDefaultProgram(Renderer &r, T &prog, const char *fragSrc)
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

bool DefaultTexProgram::compile(Renderer &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.support.useFixedFunctionPipeline)
		return false;
	logMsg("making texture program");
	compileDefaultProgram(r, *this, texFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultTexReplaceProgram::compile(Renderer &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.support.useFixedFunctionPipeline)
		return false;
	logMsg("making texture program (replace mode)");
	compileDefaultProgram(r, *this, texReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultTexAlphaReplaceProgram::compile(Renderer &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.support.useFixedFunctionPipeline)
		return false;
	if(r.support.hasTextureSwizzle)
	{
		auto compiled = r.texProgram.compile(r);
		impl = &r.texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program (replace mode)");
	compileDefaultProgram(r, *this, texAlphaReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultTexAlphaProgram::compile(Renderer &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.support.useFixedFunctionPipeline)
		return false;
	if(r.support.hasTextureSwizzle)
	{
		auto compiled = r.texProgram.compile(r);
		impl = &r.texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program");
	compileDefaultProgram(r, *this, texAlphaFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultTexExternalReplaceProgram::compile(Renderer &r)
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || r.support.useFixedFunctionPipeline)
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

void DefaultTexExternalReplaceProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultTexExternalProgram::compile(Renderer &r)
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || r.support.useFixedFunctionPipeline)
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

void DefaultTexExternalProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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

bool DefaultColorProgram::compile(Renderer &r)
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || r.support.useFixedFunctionPipeline)
		return false;
	logMsg("making color shaded program");
	compileDefaultProgram(r, *this, noTexFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultColorProgram::use(RendererCommands &cmds, const Mat4 *modelMat)
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
