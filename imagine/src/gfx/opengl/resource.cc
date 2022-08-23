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
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/logger/logger.h>

namespace IG::Gfx
{

Texture Renderer::makeTexture(TextureConfig config)
{
	return {task(), config};
}

Texture Renderer::makeTexture(IG::Data::PixmapSource img, const TextureSampler *compatSampler, bool makeMipmaps)
{
	return {task(), img, compatSampler, makeMipmaps};
}

PixmapBufferTexture Renderer::makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode, bool singleBuffer)
{
	return {task(), config, mode, singleBuffer};
}

TextureSampler Renderer::makeTextureSampler(TextureSamplerConfig config)
{
	return {task(), config};
}

template <class CommonSamplers>
static auto &commonTextureSampler(CommonSamplers &commonSampler, CommonTextureSampler sampler)
{
	switch(sampler)
	{
		case CommonTextureSampler::CLAMP: return commonSampler.clamp;
		case CommonTextureSampler::NEAREST_MIP_CLAMP: return commonSampler.nearestMipClamp;
		case CommonTextureSampler::NO_MIP_CLAMP: return commonSampler.noMipClamp;
		case CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP: return commonSampler.noLinearNoMipClamp;
		case CommonTextureSampler::REPEAT: return commonSampler.repeat;
		case CommonTextureSampler::NEAREST_MIP_REPEAT: return commonSampler.nearestMipRepeat;
		default: bug_unreachable("sampler:%d", (int)sampler);
	}
}

const TextureSampler &Renderer::commonTextureSampler(CommonTextureSampler sampler) const
{
	auto &samplerObj = Gfx::commonTextureSampler(commonSampler, sampler);
	assert(samplerObj);
	return samplerObj;
}

static TextureSamplerConfig commonTextureSamplerConfig(CommonTextureSampler sampler)
{
	TextureSamplerConfig conf;
	switch(sampler)
	{
		case CommonTextureSampler::CLAMP:
			conf.debugLabel = "CommonClamp";
			return conf;
		case CommonTextureSampler::NEAREST_MIP_CLAMP:
			conf.debugLabel = "CommonNearestMipClamp";
			conf.mipFilter = MipFilter::NEAREST;
			return conf;
		case CommonTextureSampler::NO_MIP_CLAMP:
			conf.debugLabel = "CommonNoMipClamp";
			conf.mipFilter = MipFilter::NONE;
			return conf;
		case CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP:
			conf.debugLabel = "CommonNoLinearNoMipClamp";
			conf.setLinearFilter(false);
			conf.mipFilter = MipFilter::NONE;
			return conf;
		case CommonTextureSampler::REPEAT:
			conf.debugLabel = "CommonRepeat";
			conf.setWrapMode(WrapMode::REPEAT);
			return conf;
		case CommonTextureSampler::NEAREST_MIP_REPEAT:
			conf.debugLabel = "CommonNearestMipRepeat";
			conf.mipFilter = MipFilter::NEAREST;
			conf.setWrapMode(WrapMode::REPEAT);
			return conf;
		default: bug_unreachable("sampler:%d", (int)sampler);
	}
}

const TextureSampler &Renderer::makeCommonTextureSampler(CommonTextureSampler sampler)
{
	auto &samplerObj = Gfx::commonTextureSampler(commonSampler, sampler);
	if(!samplerObj)
		samplerObj = makeTextureSampler(commonTextureSamplerConfig(sampler));
	return samplerObj;
}

void destroyGLBuffer(RendererTask &task, NativeBuffer buff)
{
	logMsg("deleting GL buffer:%u", buff);
	task.run(
		[buff]()
		{
			glDeleteBuffers(1, &buff);
		});
}

}
