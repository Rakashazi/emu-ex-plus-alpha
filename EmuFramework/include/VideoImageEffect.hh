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

#include <gfx/Gfx.hh>
#include <gfx/GfxBufferImage.hh>
#include <pixmap/Pixmap.hh>

class VideoImageEffect
{
private:
	Gfx::Program prog[2];
	Gfx::Shader fShader[2] {0};
	int texDeltaU[2] {0};
	uint effect_ = NO_EFFECT;

public:
	enum
	{
		NO_EFFECT = 0,
		HQ2X = 1,

		MAX_EFFECT_VAL = HQ2X
	};

	constexpr	VideoImageEffect() {}
	void setEffect(uint effect);
	uint effect();
	void compile(const Gfx::BufferImage &img);
	void place(const IG::Pixmap &pix);
	Gfx::Program &program(uint imgMode);
	bool hasProgram() const;
};
