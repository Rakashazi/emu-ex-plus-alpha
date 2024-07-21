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

#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererTask.hh>
#include <imagine/gfx/TextureSampler.hh>
#include <imagine/util/variant.hh>
#include <imagine/util/format.hh>
#include "utils.hh"
#include <limits>

namespace IG::Gfx
{

constexpr SystemLogger log{"GLTextureSampler"};

static void setSamplerParameteri(const Renderer &r, GLuint sampler, GLenum pname, GLint param)
{
	runGLCheckedVerbose(
		[&]()
		{
			r.support.glSamplerParameteri(sampler, pname, param);
		}, "glSamplerParameteri()");
}

// make sure sampler-related enums can fit into a 16-bit int
static_assert(GL_LINEAR < std::numeric_limits<uint16_t>::max());
static_assert(GL_NEAREST < std::numeric_limits<uint16_t>::max());
static_assert(GL_LINEAR_MIPMAP_NEAREST < std::numeric_limits<uint16_t>::max());
static_assert(GL_NEAREST_MIPMAP_NEAREST < std::numeric_limits<uint16_t>::max());
static_assert(GL_LINEAR_MIPMAP_LINEAR < std::numeric_limits<uint16_t>::max());
static_assert(GL_NEAREST_MIPMAP_LINEAR < std::numeric_limits<uint16_t>::max());
static_assert(GL_CLAMP_TO_EDGE < std::numeric_limits<uint16_t>::max());
static_assert(GL_REPEAT < std::numeric_limits<uint16_t>::max());
static_assert(GL_MIRRORED_REPEAT < std::numeric_limits<uint16_t>::max());

static uint16_t makeMinFilter(bool linearFiltering, MipFilter mipFiltering)
{
	switch(mipFiltering)
	{
		case MipFilter::NONE: return linearFiltering ? GL_LINEAR : GL_NEAREST;
		case MipFilter::NEAREST: return linearFiltering ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
		case MipFilter::LINEAR: return linearFiltering ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
		default: bug_unreachable("mipFiltering == %d", (int)mipFiltering);
	}
}

static uint16_t makeMagFilter(bool linearFiltering)
{
	return linearFiltering ? GL_LINEAR : GL_NEAREST;
}

static uint16_t makeWrapMode(WrapMode mode)
{
	switch(mode)
	{
		case WrapMode::REPEAT: return GL_REPEAT;
		case WrapMode::MIRROR_REPEAT: return GL_MIRRORED_REPEAT;
		case WrapMode::CLAMP: return GL_CLAMP_TO_EDGE;
	}
	bug_unreachable("invalid WrapMode");
}

SamplerParams asSamplerParams(TextureSamplerConfig config)
{
	auto minFilter = makeMinFilter(config.magLinearFilter, config.mipFilter);
	auto magFilter = makeMagFilter(config.minLinearFilter);
	auto xWrapMode = makeWrapMode(config.xWrapMode);
	auto yWrapMode = makeWrapMode(config.yWrapMode);
	return SamplerParams
	{
		.minFilter = minFilter,
		.magFilter = magFilter,
		.xWrapMode = xWrapMode,
		.yWrapMode = yWrapMode,
	};
}

GLTextureSampler::GLTextureSampler(RendererTask &rTask, TextureSamplerConfig config):
	sampler{GLSamplerRefDeleter{&rTask}},
	debugLabel{config.debugLabel ? config.debugLabel : ""}
{
	auto &r = rTask.renderer();
	if(!r.support.hasSamplerObjects)
		return;
	auto minFilter = makeMinFilter(config.magLinearFilter, config.mipFilter);
	auto magFilter = makeMagFilter(config.minLinearFilter);
	auto xWrapMode = makeWrapMode(config.xWrapMode);
	auto yWrapMode = makeWrapMode(config.yWrapMode);
	rTask.runSync(
		[=, this, &r = std::as_const(r)](GLTask::TaskContext ctx)
		{
			GLuint name;
			r.support.glGenSamplers(1, &name);
			sampler.get() = name;
			ctx.notifySemaphore();
			if(magFilter != GL_LINEAR) // GL_LINEAR is the default
				setSamplerParameteri(r, name, GL_TEXTURE_MAG_FILTER, magFilter);
			if(minFilter != GL_NEAREST_MIPMAP_LINEAR) // GL_NEAREST_MIPMAP_LINEAR is the default
				setSamplerParameteri(r, name, GL_TEXTURE_MIN_FILTER, minFilter);
			if(xWrapMode != GL_REPEAT) // GL_REPEAT is the default
				setSamplerParameteri(r, name, GL_TEXTURE_WRAP_S, xWrapMode);
			if(yWrapMode != GL_REPEAT) // GL_REPEAT​​ is the default
				setSamplerParameteri(r, name, GL_TEXTURE_WRAP_T, yWrapMode);
		});
	log.info("created sampler object:{:X} ({})", name(), config.debugLabel);
}

Renderer &TextureSampler::renderer() const
{
	return task().renderer();
}

RendererTask &TextureSampler::task() const
{
	return *sampler.get_deleter().rTaskPtr;
}

void destroyGLSamplerRef(RendererTask &rTask, GLSamplerRef name)
{
	rTask.run(
		[&r = std::as_const(rTask.renderer()), name]()
		{
			log.debug("deleting sampler object:{:X}", name);
			r.support.glDeleteSamplers(1, &name);
		});
}

TextureSampler::operator bool() const
{
	return sampler;
}

GLuint GLTextureSampler::name() const
{
	return sampler.get();
}

}
