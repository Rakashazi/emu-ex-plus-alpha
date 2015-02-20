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

#include <imagine/gfx/GfxSprite.hh>

class VideoImageOverlay
{
	Gfx::Texture img;
	IG::Pixmap pix {PixelFormatIA88};
	Gfx::Sprite spr;
	uint effect = NO_EFFECT;

public:
	Gfx::GC intensity = 0.25;

	enum
	{
		NO_EFFECT = 0,
		SCANLINES = 1, SCANLINES_2 = 2,
		CRT = 10,
		CRT_RGB = 20, CRT_RGB_2,

		MAX_EFFECT_VAL = CRT_RGB_2
	};

	constexpr	VideoImageOverlay() {}
	void setEffect(uint effect);
	void place(const Gfx::Sprite &disp, uint lines);
	void draw();
};
