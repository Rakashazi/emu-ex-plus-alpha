/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/AssetManager.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gui/View.hh>
#include <imagine/data-type/image/PixmapSource.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"AssetManager"};

constexpr AssetDesc assetDesc[wise_enum::size<AssetID>]
{
	// arrow, accept, close, more
	{AssetFileID::ui, {{},       {.25, .25}}},
	{AssetFileID::ui, {{.25, 0}, {.5,  .25}}},
	{AssetFileID::ui, {{.5,  0}, {.75, .25}}},
	{AssetFileID::ui, {{.75, 0}, {1.,  .25}}},
	// fast, slow, speed, menu
	{AssetFileID::ui, {{0,   .25}, {.25, .5}}},
	{AssetFileID::ui, {{.25, .25}, {.5,  .5}}},
	{AssetFileID::ui, {{.5,  .25}, {.75, .5}}},
	{AssetFileID::ui, {{.75, .25}, {1.,  .5}}},
	// leftSwitch, rightSwitch, load, save
	{AssetFileID::ui, {{0,   .5}, {.25, .75}}},
	{AssetFileID::ui, {{.25, .5}, {.5,  .75}}},
	{AssetFileID::ui, {{.5,  .5}, {.75, .75}}},
	{AssetFileID::ui, {{.75, .5}, {1.,  .75}}},
	// display, screenshot, openFile, rewind
	{AssetFileID::ui, {{0,   .75}, {.25, 1.}}},
	{AssetFileID::ui, {{.25, .75}, {.5,  1.}}},
	{AssetFileID::ui, {{.5,  .75}, {.75, 1.}}},
	{AssetFileID::ui, {{.75, .75}, {1.,  1.}}},
	{AssetFileID::gamepadOverlay, {{}, {1.f, 1.f}}},
	{AssetFileID::keyboardOverlay, {{}, {1.f, 1.f}}},
};

Gfx::TextureSpan AssetManager::get(Gfx::Renderer& r, AssetID assetID) const
{
	assumeExpr(to_underlying(assetID) < wise_enum::size<AssetID>);
	return get(r, assetDesc[to_underlying(assetID)]);
}

Gfx::TextureSpan AssetManager::get(Gfx::Renderer& r, AssetDesc desc) const
{
	auto &res = assetBuffImg[desc.fileIdx()];
	if(!res)
	{
		try
		{
			res = r.makeTexture(pixmapReader.loadAsset(desc.filename()), View::imageSamplerConfig);
		}
		catch(...)
		{
			log.error("error loading asset:{}", desc.filename());
		}
	}
	return {&res, desc.texBounds};
}

Gfx::TextureSpan AssetManager::collectTextCloseAsset(Gfx::Renderer& r) const
{
	return Config::envIsAndroid ? Gfx::TextureSpan{} : get(r, AssetID::close);
}

}
