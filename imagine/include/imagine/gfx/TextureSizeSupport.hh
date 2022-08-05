#pragma once

#include <imagine/config/defs.hh>
#include <imagine/gfx/defs.hh>
#include <imagine/util/used.hh>

namespace IG
{
class PixmapDesc;
}

namespace IG::Gfx
{

struct TextureSizeSupport
{
	int maxXSize{}, maxYSize{};
	IG_UseMemberIfOrConstant(Config::Gfx::OPENGL_ES == 1, bool, true, nonPow2){};
	bool nonPow2CanMipmap{};
	bool nonPow2CanRepeat{};

	bool supportsMipmaps(int imageX, int imageY) const;
};

}
