#include <gfx/Gfx.hh>
#include <gfx/GfxBufferImage.hh>
#include "private.hh"

namespace Gfx
{

#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE

GLSLProgram *currProgram = nullptr;
static GLuint defaultVShader = 0;

	#ifdef CONFIG_GFX_OPENGL_ES
	#define GLSL_VERSION_DIRECTIVE
	#define GLSL_VIN "attribute"
	#define GLSL_VOUT "varying"
	#define GLSL_FIN "varying"
	#define GLSL_TEXTURE "texture2D"
	#define GLSL_FRAGCOLOR "gl_FragColor"
	#define GLSL_FRAGCOLOR_DEF
	#else
	#define GLSL_VIN "in"
	#define GLSL_VOUT "out"
	#define GLSL_FIN "in"
	#define GLSL_TEXTURE "texture"
	#define GLSL_FRAGCOLOR "fragColor"
	#define GLSL_FRAGCOLOR_DEF "out vec4 fragColor;\n"
		#ifdef __APPLE__
		#define GLSL_VERSION_DIRECTIVE "#version 150\n"
		#else
		#define GLSL_VERSION_DIRECTIVE "#version 130\n"
		#endif
	#endif

static const char *vShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_VIN " vec4 pos; "
GLSL_VIN " vec4 color; "
GLSL_VIN " vec2 texUV; "
GLSL_VOUT " vec4 colorOut; "
GLSL_VOUT " vec2 texUVOut; "
"uniform mat4 modelview; "
"uniform mat4 proj; "
"void main() { "
	"colorOut = color; "
	"texUVOut = texUV; "
	"gl_Position = (proj * modelview) * pos; "
"}"
;

static const char *texFragShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_FRAGCOLOR_DEF
GLSL_FIN " lowp vec4 colorOut; "
GLSL_FIN " lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	GLSL_FRAGCOLOR " = colorOut * " GLSL_TEXTURE "(tex, texUVOut); "
"}"
;

static const char *texReplaceFragShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_FRAGCOLOR_DEF
GLSL_FIN " lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	GLSL_FRAGCOLOR " = " GLSL_TEXTURE "(tex, texUVOut); "
"}"
;

static const char *texAlphaFragShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_FRAGCOLOR_DEF
GLSL_FIN " lowp vec4 colorOut; "
GLSL_FIN " lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	// adapted from: gl_FragColor = colorOut * vec4(1., 1., 1., texture2D(tex, texUVOut).[alpha]);
	"lowp vec4 tmp; "
	"tmp.rgb = colorOut.rgb; "
	#ifdef CONFIG_GFX_OPENGL_ES
	"tmp.a = colorOut.a * " GLSL_TEXTURE "(tex, texUVOut).a; "
	#else
	"tmp.a = colorOut.a * " GLSL_TEXTURE "(tex, texUVOut).r; "
	#endif
	GLSL_FRAGCOLOR " = tmp;"
"}"
;

static const char *texAlphaReplaceFragShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_FRAGCOLOR_DEF
GLSL_FIN " lowp vec4 colorOut; "
GLSL_FIN " lowp vec2 texUVOut; "
"uniform sampler2D tex; "
"void main() { "
	"lowp vec4 tmp; "
	"tmp.rgb = colorOut.rgb; "
	#ifdef CONFIG_GFX_OPENGL_ES
	"tmp.a = " GLSL_TEXTURE "(tex, texUVOut).a; "
	#else
	"tmp.a = " GLSL_TEXTURE "(tex, texUVOut).r; "
	#endif
	GLSL_FRAGCOLOR " = tmp;"
"}"
;

	#ifndef CONFIG_GFX_OPENGL_ES
	static const char *texIntensityAlphaFragShaderSrc =
	GLSL_VERSION_DIRECTIVE
	GLSL_FRAGCOLOR_DEF
	GLSL_FIN " lowp vec4 colorOut; "
	GLSL_FIN " lowp vec2 texUVOut; "
	"uniform sampler2D tex; "
	"void main() { "
		"lowp float i = " GLSL_TEXTURE "(tex, texUVOut).r; "
		GLSL_FRAGCOLOR " = colorOut * vec4(i, i, i, " GLSL_TEXTURE "(tex, texUVOut).g); "
	"}"
	;

	static const char *texIntensityAlphaReplaceFragShaderSrc =
	GLSL_VERSION_DIRECTIVE
	GLSL_FRAGCOLOR_DEF
	GLSL_FIN " lowp vec2 texUVOut; "
	"uniform sampler2D tex; "
	"void main() { "
		"lowp float i = " GLSL_TEXTURE "(tex, texUVOut).r; "
		GLSL_FRAGCOLOR " = vec4(i, i, i, " GLSL_TEXTURE "(tex, texUVOut).g); "
	"}"
	;
	#else
	static const char *texIntensityAlphaFragShaderSrc = nullptr;
	static const char *texIntensityAlphaReplaceFragShaderSrc = nullptr;
	#endif

	#if defined CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
	static const char *texExternalFragShaderSrc =
	GLSL_VERSION_DIRECTIVE
	GLSL_FRAGCOLOR_DEF
	"#extension GL_OES_EGL_image_external:enable\n"
	GLSL_FIN " lowp vec4 colorOut; "
	GLSL_FIN " lowp vec2 texUVOut; "
	"uniform samplerExternalOES tex; "
	"void main() { "
		GLSL_FRAGCOLOR " = colorOut * " GLSL_TEXTURE "(tex, texUVOut); "
	"}"
	;

	static const char *texExternalReplaceFragShaderSrc =
	GLSL_VERSION_DIRECTIVE
	GLSL_FRAGCOLOR_DEF
	"#extension GL_OES_EGL_image_external:enable\n"
	GLSL_FIN " lowp vec2 texUVOut; "
	"uniform samplerExternalOES tex; "
	"void main() { "
		GLSL_FRAGCOLOR " = " GLSL_TEXTURE "(tex, texUVOut); "
	"}"
	;
	#endif

static const char *noTexFragShaderSrc =
GLSL_VERSION_DIRECTIVE
GLSL_FRAGCOLOR_DEF
GLSL_FIN " lowp vec4 colorOut; "
"void main() { "
	GLSL_FRAGCOLOR " = colorOut; "
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
	#ifndef NDEBUG
	GLchar messages[4096];
	glGetProgramInfoLog(program, sizeof(messages), nullptr, messages);
	if(strlen(messages))
		logDMsg("linker info log: %s", messages);
	#endif
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
		projectionUniformAge = 0;
		modelViewUniformAge = 0;
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
	assert(loc != -1);
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
	//textureUniform = glGetUniformLocation(program_, "tex");
	//handleGLErrorsVerbose([](GLenum, const char *err) { logErr("%s in glGetUniformLocation tex", err); });
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

Shader makeShader(const char *src, uint type)
{
	auto shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

	#ifndef NDEBUG
	GLchar messages[4096];
	glGetShaderInfoLog(shader, sizeof(messages), nullptr, messages);
	if(strlen(messages))
		logDMsg("shader info log: %s", messages);
	#endif
	if(success == GL_FALSE)
		return 0;
	return shader;
}

Shader makePluginVertexShader(const char *src, uint imgMode)
{
	const char *modulateDefs =
	"#define OUT_POSITION colorOut = color; gl_Position = (proj * modelview) * pos\n"
	GLSL_VIN " vec4 pos; "
	"uniform mat4 modelview; "
	"uniform mat4 proj; "
	GLSL_VIN " vec4 color; "
	GLSL_VOUT " vec4 colorOut; ";
	const char *replaceDefs =
	"#define OUT_POSITION gl_Position = (proj * modelview) * pos\n"
	GLSL_VIN " vec4 pos; "
	"uniform mat4 modelview; "
	"uniform mat4 proj; ";
	char shaderStr[8192] {0};
	if(!string_printf(shaderStr,
		GLSL_VERSION_DIRECTIVE
		"%s%s",
		imgMode == IMG_MODE_MODULATE ? modulateDefs : replaceDefs,
		src))
	{
		logMsg("shader text too large");
		return 0;
	}
	//logMsg("making vertex shader with source:\n%s", shaderStr);
	return makeShader(shaderStr, GL_VERTEX_SHADER);
}

Shader makePluginFragmentShader(const char *src, uint imgMode, const BufferImage &img)
{
	const char *modulateDefs =
	"#define OUT_FRAGCOLOR(c) " GLSL_FRAGCOLOR " = colorOut * c\n"
	GLSL_FIN " lowp vec4 colorOut;\n";
	const char *replaceDefs =
	"#define OUT_FRAGCOLOR(c) " GLSL_FRAGCOLOR " = c\n";
	const char *externalTexDefs =
	"#extension GL_OES_EGL_image_external:enable\n"
	"#define sampler2D samplerExternalOES\n";
	char shaderStr[8192] {0};
	if(!string_printf(shaderStr,
		GLSL_VERSION_DIRECTIVE
		GLSL_FRAGCOLOR_DEF
		"%s%s%s",
		(Config::envIsAndroid && img.type() == TEX_2D_EXTERNAL) ? externalTexDefs : "",
		imgMode == IMG_MODE_MODULATE ? modulateDefs : replaceDefs,
		src))
	{
		logMsg("shader text too large");
		return 0;
	}
	//logMsg("making fragment shader with source:\n%s", shaderStr);
	return makeShader(shaderStr, GL_FRAGMENT_SHADER);
}

Shader makeDefaultVShader()
{
	if(!defaultVShader)
		defaultVShader = makeShader(vShaderSrc, GL_VERTEX_SHADER);
	return defaultVShader;
}

void deleteShader(Shader shader)
{
	logMsg("deleting shader %d", (int)shader);
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
	auto fShader = makeShader(fragSrc, GL_FRAGMENT_SHADER);
	prog.init(vShader, fShader);
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
DefaultTexIntensityAlphaReplaceProgram texIntensityAlphaReplaceProgram;
DefaultTexExternalReplaceProgram texExternalReplaceProgram;

DefaultTexProgram texProgram;
DefaultTexAlphaProgram texAlphaProgram;
DefaultTexIntensityAlphaProgram texIntensityAlphaProgram;
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

bool DefaultTexIntensityAlphaReplaceProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	#ifndef CONFIG_GFX_OPENGL_ES
	if(useTextureSwizzle)
	#endif
	{
		auto compiled = texReplaceProgram.compile();
		impl = &texReplaceProgram;
		return compiled;
	}
	logMsg("making intensity+alpha texture program (replace mode)");
	compileDefaultProgram(*this, texIntensityAlphaReplaceFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexIntensityAlphaReplaceProgram::use(const Mat4 *modelMat)
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

bool DefaultTexIntensityAlphaProgram::compile()
{
	#ifdef CONFIG_GFX_OPENGL_SHADER_PIPELINE
	if(program() || useFixedFunctionPipeline)
		return false;
	#ifndef CONFIG_GFX_OPENGL_ES
	if(useTextureSwizzle)
	#endif
	{
		auto compiled = texProgram.compile();
		impl = &texProgram;
		return compiled;
	}
	logMsg("making intensity+alpha texture program");
	compileDefaultProgram(*this, texIntensityAlphaFragShaderSrc);
	return true;
	#else
	return false;
	#endif
};

void DefaultTexIntensityAlphaProgram::use(const Mat4 *modelMat)
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
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
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
	#if defined CONFIG_GFX_OPENGL_SHADER_PIPELINE && defined CONFIG_GFX_OPENGL_TEXTURE_EXTERNAL_OES
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
		setActiveTexture(0);
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
