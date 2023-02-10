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
#include <imagine/base/Window.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/math/int.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

VControllerButtonGroup::VControllerButtonGroup(std::span<const unsigned> buttonCodes, _2DOrigin layoutOrigin, int8_t rowItems):
	layoutOrigin{layoutOrigin},
	rowItems{rowItems}
{
	buttons.reserve(buttonCodes.size());
	for(auto c : buttonCodes)
	{
		buttons.emplace_back(c);
	}
}

static int buttonsToLayout(const auto &buttons)
{
	int count{};
	for(const auto &b : buttons)
	{
		if(b.skipLayout || !b.enabled)
			continue;
		count++;
	}
	return count;
}

static void layoutButtons(auto &buttons, WRect layoutBounds, WRect viewBounds, WP size,
	int spacing, int stagger, int rowShift, int rowItems, _2DOrigin o)
{
	if(!rowItems)
		return;
	int rows = divRoundUp(buttonsToLayout(buttons), rowItems);
	int row{}, btnPos{}, y{o.yScaler() == -1 ? -size.y : 0};
	if(stagger < 0)
		y += stagger * (rowItems - 1);
	int x = -rowShift * (rows - 1);
	int staggerOffset = 0;
	for(auto &b : buttons)
	{
		if(b.skipLayout || !b.enabled)
			continue;
		WP pos = layoutBounds.pos(o) + WP{x, y + staggerOffset} + (size / 2);
		b.setPos(pos, viewBounds);
		x += size.x + spacing;
		staggerOffset -= stagger;
		if(++btnPos == rowItems)
		{
			row++;
			y += (size.y + spacing) * o.yScaler();
			staggerOffset = 0;
			x = -rowShift * ((rows - 1) - row);
			btnPos = 0;
		}
	}
}

void VControllerButtonGroup::setPos(WP pos, WindowRect viewBounds)
{
	bounds_.setPos(pos, C2DO);
	bounds_.fitIn(viewBounds);
	layoutButtons(buttons, bounds_, viewBounds, btnSize,
		spacingPixels, btnStagger, btnRowShift, rowItems, LB2DO);
}

void VControllerButtonGroup::setButtonSize(WP size)
{
	btnSize = size;
	setStaggerType(btnStaggerType);
	int btnsPerRow = std::min(buttonsToLayout(buttons), int(rowItems));
	int xSizePixel = size.x*btnsPerRow + spacingPixels*(btnsPerRow-1) + std::abs(btnRowShift*((int)rows()-1));
	int ySizePixel = size.y*rows() + spacingPixels*(rows()-1) + std::abs(btnStagger*((int)btnsPerRow-1));
	bounds_ = makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});
	WP extendedSize = paddingPixels();
	for(auto &b : buttons)
	{
		b.setSize(size, extendedSize);
	}
}

void VControllerButtonGroup::setStaggerType(uint8_t type)
{
	btnStaggerType = type;
	btnRowShift = 0;
	switch(type)
	{
		case 0:
			btnStagger = btnSize.y * -.75; break;
		case 1:
			btnStagger = btnSize.y * -.5; break;
		case 2:
			btnStagger = 0; break;
		case 3:
			btnStagger = btnSize.y * .5; break;
		case 4:
			btnStagger = btnSize.y * .75; break;
		default:
			btnStagger = btnSize.y + spacingPixels;
			btnRowShift = -(btnSize.y + spacingPixels);
			break;
	}
}

void VControllerButtonGroup::setSpacing(int16_t space, const Window &win)
{
	spacingMM100x = space;
	spacingPixels = makeEvenRoundedUp(win.widthMMInPixels(space / 100.));
	setStaggerType(btnStaggerType);
}

void VControllerButtonGroup::updateMeasurements(const Window &win)
{
	setSpacing(spacingMM100x, win);
	setButtonSize(btnSize);
}

void VControllerButtonGroup::transposeKeysForPlayer(const EmuApp &app, int player)
{
	for(auto &b : buttons)
	{
		b.key = app.transposeKeyForPlayer(b.key, player);
	}
}

int VControllerButtonGroup::rows() const
{
	return divRoundUp(buttonsToLayout(buttons), rowItems);
}

std::array<int, 2> VControllerButtonGroup::findButtonIndices(WP windowPos) const
{
	std::array<int, 2> btnOut{-1, -1};
	if(state == VControllerState::OFF)
		return btnOut;
	for(size_t count = 0; auto &b : buttons)
	{
		if(b.overlaps(windowPos))
		{
			btnOut[count++] = b.key;
			if(count == btnOut.size())
				break;
		}
	}
	return btnOut;
}

void VControllerButtonGroup::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden) const
{
	if(!VController::shouldDraw(state, showHidden))
		return;
	auto &basicEffect = cmds.basicEffect();
	if(showBoundingArea)
	{
		basicEffect.disableTexture(cmds);
		for(const auto &b : buttons)
		{
			if(!b.enabled)
				continue;
			b.drawBounds(cmds);
		}
	}
	drawButtons(cmds);
}

void VControllerButtonGroup::drawButtons(Gfx::RendererCommands &__restrict__ cmds) const
{
	cmds.basicEffect().enableTexture(cmds);
	for(auto &b : buttons)
	{
		if(!b.enabled)
			continue;
		b.drawSprite(cmds);
	}
}

static std::string namesString(auto &buttons, const EmuApp &app)
{
	if(buttons.empty())
		return "Empty Group";
	std::string s;
	for(const auto &b : buttons | std::ranges::views::take(buttons.size() - 1))
	{
		s += b.name(app);
		s += " | ";
	}
	s += buttons.back().name(app);
	return s;
}

std::string VControllerButtonGroup::name(const EmuApp &app) const
{
	return namesString(buttons, app);
}

VControllerUIButtonGroup::VControllerUIButtonGroup(std::span<const unsigned> buttonCodes, _2DOrigin layoutOrigin):
	rowItems{4},
	layoutOrigin{layoutOrigin}
{
	buttons.reserve(buttonCodes.size());
	for(auto c : buttonCodes)
	{
		buttons.emplace_back(c);
	}
}

void VControllerUIButtonGroup::setPos(WP pos, WindowRect viewBounds)
{
	bounds_.setPos(pos, C2DO);
	bounds_.fitIn(viewBounds);
	layoutButtons(buttons, bounds_, viewBounds, btnSize,
		0, 0, 0, rowItems, LT2DO);
}

void VControllerUIButtonGroup::setButtonSize(WP size)
{
	btnSize = size;
	int btnsPerRow = std::min(buttonsToLayout(buttons), int(rowItems));
	int xSizePixel = size.x * btnsPerRow;
	int ySizePixel = size.y * rows();
	bounds_ = makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});
	for(auto &b : buttons)
	{
		b.setSize(size);
	}
}

int VControllerUIButtonGroup::rows() const
{
	return divRoundUp(buttonsToLayout(buttons), rowItems);
}

void VControllerUIButtonGroup::draw(Gfx::RendererCommands &__restrict__ cmds, bool showHidden) const
{
	if(!VController::shouldDraw(state, showHidden))
		return;
	cmds.basicEffect().enableTexture(cmds);
	for(auto &b : buttons)
	{
		if(!b.enabled)
			continue;
		b.drawSprite(cmds);
	}
}

std::string VControllerUIButtonGroup::name(const EmuApp &app) const
{
	return namesString(buttons, app);
}

}
