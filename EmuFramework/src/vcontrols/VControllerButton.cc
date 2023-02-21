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

void VControllerButton::setPos(WP pos, WRect viewBounds, _2DOrigin o)
{
	bounds_.setPos(pos, o);
	bounds_.fitIn(viewBounds);
	extendedBounds_.setPos(bounds_.pos(C2DO), C2DO);
	spr.setPos(bounds_);
}

void VControllerButton::setSize(WP size, WP extendedSize)
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
	return std::string{app.systemKeyName(key)};
}

void VControllerButton::drawBounds(Gfx::RendererCommands &__restrict__ cmds) const
{
	cmds.drawRect(extendedBounds_);
}

void VControllerButton::drawSprite(Gfx::RendererCommands &__restrict__ cmds) const
{
	if(color != Gfx::Color{})
	{
		auto col = color;
		col.a = cmds.color().a;
		cmds.setColor(col);
	}
	else
	{
		cmds.setColor({1.f, 1.f, 1.f, cmds.color().a});
	}
	sprite().draw(cmds);
}

}
