#pragma once

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>

namespace IG
{
class PixmapDesc;
}

namespace IG::Gfx
{

struct TextureSizeSupport
{
	uint32_t maxXSize{}, maxYSize{};
	bool nonPow2 = Config::Gfx::OPENGL_ES >= 2;
	bool nonPow2CanMipmap{};
	bool nonPow2CanRepeat{};
	static constexpr bool nonSquare = true;
	static constexpr bool forcePow2 = false;

	IG::PixmapDesc makePixmapDescWithSupportedSize(IG::PixmapDesc desc) const;
	IG::WP makeSupportedSize(IG::WP size) const;
	bool supportsMipmaps(uint32_t imageX, uint32_t imageY) const;
};

}
