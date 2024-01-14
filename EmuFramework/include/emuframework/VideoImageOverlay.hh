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
#include <imagine/gfx/Quads.hh>
#include <imagine/util/enum.hh>

namespace EmuEx
{

using namespace IG;

WISE_ENUM_CLASS((ImageOverlayId, uint8_t),
	(SCANLINES, 1),
	(SCANLINES_2, 2),
	(LCD, 10),
	(CRT_MASK, 20),
	(CRT_MASK_2, 21),
	(CRT_GRILLE, 30),
	(CRT_GRILLE_2, 31));

class VideoImageOverlay
{
public:
	constexpr	VideoImageOverlay() = default;
	void setEffect(Gfx::Renderer &, ImageOverlayId, Gfx::ColorSpace);
	void setIntensity(float intensity);
	float intensityLevel() const { return intensity; }
	void place(WRect contentRect, WSize videoPixels, Rotation);
	void draw(Gfx::RendererCommands &cmds, Gfx::Vec3 brightness);

private:
	struct Vertex
	{
		glm::i16vec2 pos;
		glm::vec2 texCoord;
	};
	using Quad = Gfx::BaseQuad<Vertex>;
	using Quads = Gfx::ObjectVertexArray<Quad>;

	Quads quad;
	Gfx::Texture texture;
	float intensity = 0.75f;
	ImageOverlayId overlayId{};
	bool multiplyBlend{};
};

}
