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

namespace Gfx
{

Texture Renderer::makeTexture(TextureConfig config)
{
	return {*this, config};
}

Texture Renderer::makeTexture(GfxImageSource &img, bool makeMipmaps)
{
	return {*this, img, makeMipmaps};
}

PixmapTexture Renderer::makePixmapTexture(TextureConfig config)
{
	return {*this, config};
}

PixmapTexture Renderer::makePixmapTexture(GfxImageSource &img, bool makeMipmaps)
{
	return {*this, img, makeMipmaps};
}

PixmapBufferTexture Renderer::makePixmapBufferTexture(TextureConfig config, TextureBufferMode mode, bool singleBuffer)
{
	return {*this, config, mode, singleBuffer};
}

TextureSampler Renderer::makeTextureSampler(TextureSamplerConfig config)
{
	return {*this, config};
}

TextureSampler &GLRenderer::commonTextureSampler(CommonTextureSampler sampler)
{
	switch(sampler)
	{
		default: bug_unreachable("sampler:%d", (int)sampler); [[fallthrough]];
		case CommonTextureSampler::CLAMP: return defaultClampSampler;
		case CommonTextureSampler::NEAREST_MIP_CLAMP: return defaultNearestMipClampSampler;
		case CommonTextureSampler::NO_MIP_CLAMP: return defaultNoMipClampSampler;
		case CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP: return defaultNoLinearNoMipClampSampler;
		case CommonTextureSampler::REPEAT: return defaultRepeatSampler;
		case CommonTextureSampler::NEAREST_MIP_REPEAT: return defaultNearestMipRepeatSampler;
	}
}

static TextureSamplerConfig commonTextureSamplerConfig(CommonTextureSampler sampler)
{
	TextureSamplerConfig conf;
	switch(sampler)
	{
		default: bug_unreachable("sampler:%d", (int)sampler); [[fallthrough]];
		case CommonTextureSampler::CLAMP:
			conf.setDebugLabel("CommonClamp");
			return conf;
		case CommonTextureSampler::NEAREST_MIP_CLAMP:
			conf.setDebugLabel("CommonNearestMipClamp");
			conf.setMipFilter(MIP_FILTER_NEAREST);
			return conf;
		case CommonTextureSampler::NO_MIP_CLAMP:
			conf.setDebugLabel("CommonNoMipClamp");
			conf.setMipFilter(MIP_FILTER_NONE);
			return conf;
		case CommonTextureSampler::NO_LINEAR_NO_MIP_CLAMP:
			conf.setDebugLabel("CommonNoLinearNoMipClamp");
			conf.setLinearFilter(false);
			conf.setMipFilter(MIP_FILTER_NONE);
			return conf;
		case CommonTextureSampler::REPEAT:
			conf.setDebugLabel("CommonRepeat");
			conf.setWrapMode(WRAP_REPEAT);
			return conf;
		case CommonTextureSampler::NEAREST_MIP_REPEAT:
			conf.setDebugLabel("CommonNearestMipRepeat");
			conf.setMipFilter(MIP_FILTER_NEAREST);
			conf.setWrapMode(WRAP_REPEAT);
			return conf;
	}
}

void Renderer::makeCommonTextureSampler(CommonTextureSampler sampler)
{
	auto &commonSampler = commonTextureSampler(sampler);
	if(!commonSampler)
		commonSampler = makeTextureSampler(commonTextureSamplerConfig(sampler));
}

}
