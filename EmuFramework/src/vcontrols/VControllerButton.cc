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

#define LOGTAG "VControllerButton"
#include <emuframework/VController.hh>
#include <emuframework/EmuApp.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

void VControllerButton::setPos(WPt pos, WRect viewBounds, _2DOrigin o)
{
	bounds_.setPos(pos, o);
	bounds_.fitIn(viewBounds);
	extendedBounds_.setPos(bounds_.pos(C2DO), C2DO);
	spr.setPos(bounds_);
}

void VControllerButton::setSize(WSize size, WSize extendedSize)
{
	size.y /= aspectRatio;
	bounds_ = makeWindowRectRel(bounds_.pos(C2DO), size);
	extendedBounds_ = bounds_ + WRect{{-extendedSize}, {extendedSize}};
}

void VControllerButton::setImage(Gfx::TextureSpan tex, int aR)
{
	spr.set(tex);
	aspectRatio = aR;
}

std::string VControllerButton::name(const EmuApp &app) const
{
	return std::string{app.inputManager.toString(key)};
}

void VControllerButton::drawBounds(Gfx::RendererCommands &__restrict__ cmds) const
{
	float brightness = isHighlighted ? 2.f : 1.f;
	cmds.setColor(Gfx::Color{.5f}.multiplyRGB(brightness));
	cmds.drawRect(extendedBounds_);
}

void VControllerButton::drawSprite(Gfx::RendererCommands &__restrict__ cmds) const
{
	sprite().draw(cmds);
}

void VControllerButton::setAlpha(float alpha)
{
	float brightness = isHighlighted ? 2.f : 1.f;
	Gfx::Color spriteColor{};
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
	for(auto &vtx : spr) { vtx.color = spriteColor; }
}

}
