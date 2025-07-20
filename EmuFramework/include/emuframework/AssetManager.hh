#pragma once

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


#include <imagine/gfx/Texture.hh>
#include <imagine/data-type/image/PixmapReader.hh>
#include <imagine/util/enum.hh>
#include <imagine/util/rectangle2.h>

namespace EmuEx
{

using namespace IG;

WISE_ENUM_CLASS((AssetFileID, size_t),
	ui,
	gamepadOverlay,
	keyboardOverlay);

WISE_ENUM_CLASS((AssetID, size_t),
	arrow,
	accept,
	close,
	more,
	fast,
	slow,
	speed,
	menu,
	leftSwitch,
	rightSwitch,
	load,
	save,
	display,
	screenshot,
	openFile,
	rewind,
	gamepadOverlay,
	keyboardOverlay);

constexpr const char *assetFilename[wise_enum::size<AssetFileID>]
{
	"ui.png",
	"gpOverlay.png",
	"kbOverlay.png",
};

struct AssetDesc
{
	AssetFileID fileID;
	FRect texBounds;
	WSize aspectRatio{1, 1};

	constexpr size_t fileIdx() const { return to_underlying(fileID); }
	constexpr auto filename() const { return assetFilename[fileIdx()]; }
};

class AssetManager
{
public:
	AssetManager(ApplicationContext ctx): pixmapReader{ctx} {}
	Gfx::TextureSpan get(Gfx::Renderer&, AssetID) const;
	Gfx::TextureSpan get(Gfx::Renderer&, AssetDesc) const;
	Gfx::TextureSpan collectTextCloseAsset(Gfx::Renderer&) const;

private:
	mutable Gfx::Texture assetBuffImg[wise_enum::size<AssetFileID>];
public:
	[[no_unique_address]] IG::Data::PixmapReader pixmapReader;
};

}
