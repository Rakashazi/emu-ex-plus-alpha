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

#include <emuframework/VController.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/gfx/RendererCommands.hh>

namespace EmuEx
{

void VControllerButton::setPos(WPt pos, WRect viewBounds, _2DOrigin o)
{
	bounds_.setPos(pos, o);
	bounds_.fitIn(viewBounds);
	extendedBounds_.setPos(bounds_.pos(C2DO), C2DO);
}

void VControllerButton::setSize(WSize size, WSize extendedSize)
{
	size.y /= aspectRatio;
	bounds_ = makeWindowRectRel(bounds_.pos(C2DO), size);
	extendedBounds_ = bounds_ + WRect{{-extendedSize}, {extendedSize}};
}

void VControllerButton::setImage(Gfx::TextureSpan t, int aR)
{
	texture = t;
	aspectRatio = aR;
}

std::string VControllerButton::name(const InputManager &mgr) const
{
	return std::string{mgr.toString(key)};
}

void VControllerButton::setAlpha(float alpha)
{
	float brightness = isHighlighted ? 2.f : 1.f;
	if(color != Gfx::Color{})
	{
		spriteColor = color.multiplyRGB(alpha).multiplyRGB(brightness);
	}
	else
	{
		if(key.flags.turbo && key.flags.toggle)
			spriteColor = Gfx::Color{alpha * 2.f, alpha * 2.f, alpha, alpha}.multiplyRGB(brightness);
		else if(key.flags.turbo)
			spriteColor = Gfx::Color{alpha * 2.f, alpha, alpha, alpha}.multiplyRGB(brightness);
		else if(key.flags.toggle)
			spriteColor = Gfx::Color{alpha, alpha * 2.f, alpha, alpha}.multiplyRGB(brightness);
		else
			spriteColor = Gfx::Color{alpha}.multiplyRGB(brightness);
	}
}

}
