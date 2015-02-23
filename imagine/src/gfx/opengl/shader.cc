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
#include "private.hh"

namespace Gfx
{

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE

GLSLProgram *currProgram{};
static GLuint defaultVShader = 0;

static const char *vShaderSrc =
"in vec4 pos; "
"in vec4 color; "
"in vec2 texUV; "
"out vec4 colorOut; "
"out vec2 texUVOut; "
"uniform mat4 modelview; "
"uniform mat4 proj; "
"void main() { "
	"colorOut = color; "
	"texUVOut = texUV; "
	"gl_Position = (proj * modelview) * pos; "
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
"#extension GL_OES_EGL_image_external : require\n"
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"in lowp vec2 texUVOut; "
"uniform samplerExternalOES tex; "
"void main() { "
	"FRAGCOLOR = colorOut * texture(tex, texUVOut); "
"}"
;

static const char *texExternalReplaceFragShaderSrc =
"#extension GL_OES_EGL_image_external : require\n"
"FRAGCOLOR_DEF "
"in lowp vec2 texUVOut; "
"uniform samplerExternalOES tex; "
"void main() { "
	"FRAGCOLOR = texture(tex, texUVOut); "
"}"
;
#endif

static const char *noTexFragShaderSrc =
"FRAGCOLOR_DEF "
"in lowp vec4 colorOut; "
"void main() { "
	"FRAGCOLOR = colorOut; "
"}"
;

GLuint makeProgram(GLuint vShader, GLuint fShader)
{
	auto program = glCreateProgram();
	glAttachShader(program, vShader);
	glAttachShader(program, fShader);
	return program;
}

bool linkProgram(GLuint program)
{
	glLinkProgram(program);
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

bool GLSLProgram::init(Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	if(program_)
		deinit();
	program_ = makeProgram(vShader, fShader);
	glBindAttribLocation(program_, VATTR_POS, "pos");
	handleGLErrors([](GLenum, const char *err) { logErr("%s in glBindAttribLocation pos", err); });
	if(hasColor)
	{
		glBindAttribLocation(program_, VATTR_COLOR, "color");
		handleGLErrors([](GLenum, const char *err) { logErr("%s in glBindAttribLocation color", err); });
	}
	if(hasTex)
	{
		glBindAttribLocation(program_, VATTR_TEX_UV, "texUV");
		handleGLErrors([](GLenum, const char *err) { logErr("%s in glBindAttribLocation texUV", err); });
	}
	return program_;
}

void GLSLProgram::deinit()
{
	if(program_)
	{
		logMsg("deleting program %d", (int)program_);
		glDeleteProgram(program_);
		projectionUniformAge = -1;
		modelViewUniformAge = -1;
		program_ = 0;
	}
}

bool GLSLProgram::link()
{
	if(!linkProgram(program_))
		return false;
	initUniforms();
	return true;
}

bool Program::init(Shader vShader, Shader fShader, bool hasColor, bool hasTex)
{
	return GLSLProgram::init(vShader, fShader, hasColor, hasTex);
}

void Program::deinit()
{
	GLSLProgram::deinit();
}

bool Program::link()
{
	return GLSLProgram::link();
}

int Program::uniformLocation(const char *uniformName)
{
	auto loc = glGetUniformLocation(program_, uniformName);
	handleGLErrors([](GLenum, const char *err) { logErr("%s in glGetUniformLocation proj", err); });
	return loc;
}

void GLSLProgram::initUniforms()
{
	assert(program_);
	projectionUniform = glGetUniformLocation(program_, "proj");
	handleGLErrors([](GLenum, const char *err) { logErr("%s in glGetUniformLocation proj", err); });
	modelViewUniform = glGetUniformLocation(program_, "modelview");
	handleGLErrors([](GLenum, const char *err) { logErr("%s in glGetUniformLocation modelview", err); });
}

void TexProgram::init(GLuint vShader, GLuint fShader)
{
	GLSLProgram::init(vShader, fShader, true, true);
	link();
}

void ColorProgram::init(GLuint vShader, GLuint fShader)
{
	GLSLProgram::init(vShader, fShader, true, false);
	link();
}

static void setProgram(GLSLProgram &program, const Mat4 *modelMat)
{
	if(currProgram != &program)
	{
		//logMsg("setting program: %d", program.program());
		assert(program.program());
		glUseProgram(program.program());
		currProgram = &program;
		updateProgramProjectionTransform(program);
		if(!modelMat)
			updateProgramModelViewTransform(program);
		else
		{
			//logMsg("skipping model/view matrix sync");
		}
	}
	if(modelMat)
	{
		loadTransform(*modelMat);
	}
}

void setProgram(Program &program)
{
	setProgram((GLSLProgram&)program, nullptr);
}

void setProgram(Program &program, Mat4 modelMat)
{
	setProgram((GLSLProgram&)program, &modelMat);
}

Shader makeShader(const char **src, uint srcCount, uint type)
{
	auto shader = glCreateShader(type);
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
		logErr("failed shader source:");
		iterateTimes(srcCount, i)
		{
			logErr("part %u:", i);
			logErr("%s", src[i]);
		}
		return 0;
	}
	return shader;
}

Shader makeShader(const char *src, uint type)
{
	const char *singleSrc[]{src};
	return makeShader(singleSrc, 1, type);
}

Shader makeCompatShader(const char **mainSrc, uint mainSrcCount, uint type)
{
	const uint srcCount = mainSrcCount + 2;
	const char *src[srcCount];
	const char *version = Config::Gfx::OPENGL_ES ? "#version 300 es\n" : "#version 150\n";
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
		"#define FRAGCOLOR_DEF out vec4 fragColor;\n"
		"#define FRAGCOLOR fragColor\n"
	};
	src[0] = useLegacyGLSL ? "" : version;
	if(type == GL_VERTEX_SHADER)
		src[1] = useLegacyGLSL ? legacyVertDefs : "";
	else
		src[1] = useLegacyGLSL ? legacyFragDefs : fragDefs;
	memcpy(&src[2], &mainSrc[0], sizeof(const char *) * mainSrcCount);
	return makeShader(src, srcCount, type);
}

Shader makeCompatShader(const char *src, uint type)
{
	const char *singleSrc[]{src};
	return makeCompatShader(singleSrc, 1, type);
}

Shader makeDefaultVShader()
{
	if(!defaultVShader)
		defaultVShader = makeCompatShader(vShaderSrc, GL_VERTEX_SHADER);
	return defaultVShader;
}

void deleteShader(Shader shader)
{
	logMsg("deleting shader:%u", (uint)shader);
	assert(shader != defaultVShader);
	glDeleteShader(shader);
}

void uniformF(int uniformLocation, float v1, float v2)
{
	glUniform2f(uniformLocation, v1, v2);
}

void initShaders()
{
	setColor(COLOR_WHITE);
	glEnableVertexAttribArray(VATTR_POS);
	handleGLErrors([](GLenum, const char *err) { logErr("%s in glEnableVertexAttribArray VATTR_POS", err); });
}

template <class T>
static void compileDefaultProgram(T &prog, const char *fragSrc)
{
	assert(fragSrc);
	auto vShader = makeDefaultVShader();
	assert(vShader);
	auto fShader = makeCompatShader(fragSrc, GL_FRAGMENT_SHADER);
	assert(fShader);
	prog.init(vShader, fShader);
	assert(prog.program());
	// TODO: we should be able to delete unused shaders after they're linked,
	// but when testing on a Droid running Android 2.3 (CM7), it can cause
	// malfunctions like the wrong color used for a fragment, probably a driver issue
	//glDetachShader(prog.program(), fShader);
	//glDeleteShader(fShader);
}

#else

void uniformF(int uniformLocation, float v1, float v2)
{
	bug_exit("called uniformF() without shader support");
}

void setProgram(Program &program)
{
	bug_exit("called setProgram() without shader support");
}

void setProgram(Program &program, Mat4 modelMat)
{
	bug_exit("called setProgram() without shader support");
}

void Program::deinit() {}

void deleteShader(Shader shader) {}

#endif

DefaultTexReplaceProgram texReplaceProgram;
DefaultTexAlphaReplaceProgram texAlphaReplaceProgram;
DefaultTexReplaceProgram &texIntensityAlphaReplaceProgram = texReplaceProgram;
DefaultTexExternalReplaceProgram texExternalReplaceProgram;

DefaultTexProgram texProgram;
DefaultTexAlphaProgram texAlphaProgram;
DefaultTexProgram &texIntensityAlphaProgram = texProgram;
DefaultTexExternalProgram texExternalProgram;
DefaultColorProgram noTexProgram;

bool DefaultTexProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	logMsg("making texture program");
	compileDefaultProgram(*this, texFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	setProgram(*this, modelMat);
	#endif
}

bool DefaultTexReplaceProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	logMsg("making texture program (replace mode)");
	compileDefaultProgram(*this, texReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexReplaceProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_REPLACE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	setProgram(*this, modelMat);
	#endif
}

bool DefaultTexAlphaReplaceProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	if(useTextureSwizzle)
	{
		auto compiled = texProgram.compile();
		impl = &texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program (replace mode)");
	compileDefaultProgram(*this, texAlphaReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaReplaceProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_REPLACE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(impl)
		impl->use(modelMat);
	else
		setProgram(*this, modelMat);
	#endif
}

bool DefaultTexAlphaProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	if(useTextureSwizzle)
	{
		auto compiled = texProgram.compile();
		impl = &texProgram;
		return compiled;
	}
	logMsg("making alpha-only texture program");
	compileDefaultProgram(*this, texAlphaFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexAlphaProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(impl)
		impl->use(modelMat);
	else
		setProgram(*this, modelMat);
	#endif
}

bool DefaultTexExternalReplaceProgram::compile()
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || useFixedFunctionPipeline)
		return false;
	assert(Base::androidSDK() >= 14);
	logMsg("making external texture program (replace mode)");
	compileDefaultProgram(*this, texExternalReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexExternalReplaceProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_REPLACE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	setProgram(*this, modelMat);
	#endif
}

bool DefaultTexExternalProgram::compile()
{
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_MULTIPLE_TEXTURE_TARGETS
	if(program() || useFixedFunctionPipeline)
		return false;
	assert(Base::androidSDK() >= 14);
	logMsg("making external texture program");
	compileDefaultProgram(*this, texExternalFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexExternalProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	setProgram(*this, modelMat);
	#endif
}

bool DefaultColorProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	logMsg("making color shaded program");
	compileDefaultProgram(*this, noTexFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultColorProgram::use(const Mat4 *modelMat)
{
	#ifdef CONFIG_GFX_OPENGL_FIXED_FUNCTION_PIPELINE
	if(useFixedFunctionPipeline)
	{
		setActiveTexture(0, GL_TEXTURE_2D);
		setImgMode(IMG_MODE_MODULATE);
		if(modelMat)
			loadTransform(*modelMat);
		return;
	}
	#endif
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	setProgram(*this, modelMat);
	#endif
}

}
