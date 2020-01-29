#pragma once

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/pixmap/Pixmap.hh>

namespace Gfx
{

class TextureSizeSupport
{
public:
	bool nonPow2 = Config::Gfx::OPENGL_ES_MAJOR_VERSION >= 2;
	static constexpr bool nonSquare = true;
	bool nonPow2CanMipmap = false;
	bool nonPow2CanRepeat = false;
	uint32_t maxXSize = 0, maxYSize = 0;
	static constexpr bool forcePow2 = false;

	constexpr TextureSizeSupport() {}
	IG::PixmapDesc makePixmapDescWithSupportedSize(IG::PixmapDesc desc) const;
	IG::WP makeSupportedSize(IG::WP size) const;
	bool supportsMipmaps(uint32_t imageX, uint32_t imageY) const;
};

}
