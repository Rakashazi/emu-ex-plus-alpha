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
#include <imagine/base/Window.hh>
#include <imagine/gfx/Renderer.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gfx/BasicEffect.hh>
#include <imagine/gui/View.hh>
#include <imagine/util/math.hh>

namespace EmuEx
{

VControllerButtonGroup::VControllerButtonGroup(std::span<const KeyInfo> buttonCodes, _2DOrigin layoutOrigin, int8_t rowItems):
	layout
	{
		.rowItems = rowItems,
		.origin = layoutOrigin,
	}
{
	buttons.reserve(buttonCodes.size());
	for(auto c : buttonCodes)
	{
		buttons.emplace_back(c);
	}
}

VControllerButtonGroup::VControllerButtonGroup(const Config &conf):
	layout{conf.layout}
{
	buttons.reserve(conf.keys.size());
	for(auto c : conf.keys)
	{
		buttons.emplace_back(c);
	}
}

VControllerButtonGroup::Config VControllerButtonGroup::config() const
{
	Config conf{.layout = layout};
	conf.keys.reserve(buttons.size());
	for(const auto &b : buttons) { conf.keys.emplace_back(b.key); }
	return conf;
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

static void layoutButtons(auto &buttons, WRect layoutBounds, WRect viewBounds, WSize size,
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
		auto pos = layoutBounds.pos(o) + WPt{x, y + staggerOffset} + (size / 2);
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

static bool allButtonsAreHalfHeight(const auto &btns)
{
	return std::ranges::all_of(btns, [](auto &b){ return !b.isFullHeight(); });
}

void VControllerButtonGroup::setPos(WPt pos, WindowRect viewBounds)
{
	bounds_.setPos(pos, C2DO);
	bounds_.fitIn(viewBounds);
	int ySize = allButtonsAreHalfHeight(buttons) ? btnSize / 2 : btnSize;
	layoutButtons(buttons, bounds_, viewBounds, {btnSize, ySize},
		spacingPixels, btnStagger, btnRowShift, layout.rowItems, LB2DO);
	updateSprites();
}

void VControllerButtonGroup::setButtonSize(int sizePx)
{
	btnSize = sizePx;
	setStaggerType(layout.staggerType);
	int ySizePx = allButtonsAreHalfHeight(buttons) ? sizePx / 2 : sizePx;
	int btnsPerRow = std::min(buttonsToLayout(buttons), int(layout.rowItems));
	int xSizePixel = sizePx * btnsPerRow + spacingPixels*(btnsPerRow-1) + std::abs(btnRowShift*((int)rows()-1));
	int ySizePixel = ySizePx * rows() + spacingPixels*(rows()-1) + std::abs(btnStagger*((int)btnsPerRow-1));
	bounds_ = makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});
	auto extendedSize = paddingPixels();
	for(auto &b : buttons)
	{
		b.setSize({sizePx, sizePx}, extendedSize);
	}
	updateSprites();
}

void VControllerButtonGroup::setStaggerType(uint8_t type)
{
	layout.staggerType = type;
	btnRowShift = 0;
	switch(type)
	{
		case 0:
			btnStagger = btnSize * -.75f; break;
		case 1:
			btnStagger = btnSize * -.5f; break;
		case 2:
			btnStagger = 0; break;
		case 3:
			btnStagger = btnSize * .5f; break;
		case 4:
			btnStagger = btnSize * .75f; break;
		default:
			btnStagger = btnSize + spacingPixels;
			btnRowShift = -(btnSize + spacingPixels);
			break;
	}
}

bool VControllerButtonGroup::setSpacing(int8_t space, const Window &win)
{
	if(space < 0 || space > 8)
		return false;
	layout.spacingMM = space;
	spacingPixels = makeEvenRoundedUp(win.widthMMInPixels(space));
	setStaggerType(layout.staggerType);
	return true;
}

void VControllerButtonGroup::updateMeasurements(const Window &win)
{
	setSpacing(layout.spacingMM, win);
	setButtonSize(btnSize);
}

void VControllerButtonGroup::transposeKeysForPlayer(const InputManager &mgr, int player)
{
	for(auto &b : buttons)
	{
		b.key = mgr.transpose(b.key, player);
	}
}

int VControllerButtonGroup::rows() const
{
	assert(layout.rowItems);
	return divRoundUp(buttonsToLayout(buttons), layout.rowItems);
}

std::array<KeyInfo, 2> VControllerButtonGroup::findButtonIndices(WPt windowPos) const
{
	std::array<KeyInfo, 2> btnOut{};
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

void VControllerButtonGroup::drawBounds(Gfx::RendererCommands &__restrict__ cmds) const
{
	if(!layout.showBoundingArea || !enabledBtns)
		return;
	cmds.basicEffect().disableTexture(cmds);
	cmds.drawQuads(boundQuads, 0, enabledBtns);
}

static std::string namesString(auto &buttons, const InputManager &app)
{
	if(buttons.empty())
		return "Empty Group";
	std::string s{buttons.front().name(app)};
	for(const auto &b : buttons | std::ranges::views::drop(1))
	{
		s += " | ";
		s += b.name(app);
	}
	return s;
}

std::string VControllerButtonGroup::name(const InputManager &app) const
{
	return namesString(buttons, app);
}

static bool isValidRowItemCount(int val) { return val >= 1 && val <= 5; }
static bool isValidSpacing(int val) { return val >= 0 && val <= 8; }
static bool isValidPadding(int val) { return val >= 0 && val <= 30; }
static bool isValidStaggerType(int val) { return val >= 0 && val <= 5; }

void VControllerButtonGroup::Config::validate(const InputManager &mgr)
{
	for(auto &k : keys) { k =	mgr.validateSystemKey(k, false); }
	if(!isValidRowItemCount(layout.rowItems))
		layout.rowItems = 2;
	if(!isValidSpacing(layout.spacingMM))
		layout.spacingMM = defaultButtonSpacingMM;
	if(!isValidPadding(layout.xPadding))
		layout.xPadding = 0;
	if(!isValidPadding(layout.yPadding))
		layout.yPadding = 0;
	if(!isValidStaggerType(layout.staggerType))
		layout.staggerType = 2;
}

VControllerUIButtonGroup::VControllerUIButtonGroup(std::span<const KeyInfo> buttonCodes, _2DOrigin layoutOrigin):
	layout
	{
		.rowItems = 4,
		.origin = layoutOrigin,
	}
{
	buttons.reserve(buttonCodes.size());
	for(auto c : buttonCodes)
	{
		buttons.emplace_back(c);
	}
}

VControllerUIButtonGroup::VControllerUIButtonGroup(const Config &conf):
	layout{conf.layout}
{
	buttons.reserve(conf.keys.size());
	for(auto c : conf.keys) { buttons.emplace_back(c); }
}

VControllerUIButtonGroup::Config VControllerUIButtonGroup::config() const
{
	Config conf{.layout = layout};
	conf.keys.reserve(buttons.size());
	for(const auto &b : buttons) { conf.keys.emplace_back(b.key); }
	return conf;
}

void VControllerUIButtonGroup::setPos(WPt pos, WRect viewBounds)
{
	bounds_.setPos(pos, C2DO);
	bounds_.fitIn(viewBounds);
	layoutButtons(buttons, bounds_, viewBounds, {btnSize, btnSize},
		0, 0, 0, layout.rowItems, LT2DO);
	updateSprites();
}

void VControllerUIButtonGroup::setButtonSize(int sizePx)
{
	btnSize = sizePx;
	int btnsPerRow = std::min(buttonsToLayout(buttons), int(layout.rowItems));
	int xSizePixel = sizePx * btnsPerRow;
	int ySizePixel = sizePx * rows();
	bounds_ = makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});
	for(auto &b : buttons)
	{
		b.setSize({sizePx, sizePx});
	}
	updateSprites();
}

int VControllerUIButtonGroup::rows() const
{
	assert(layout.rowItems);
	return divRoundUp(buttonsToLayout(buttons), layout.rowItems);
}

std::string VControllerUIButtonGroup::name(const InputManager &mgr) const
{
	return namesString(buttons, mgr);
}

void VControllerUIButtonGroup::Config::validate(const InputManager &mgr)
{
	for(auto &k : keys) { k =	mgr.validateSystemKey(k, true); }
	if(!isValidRowItemCount(layout.rowItems))
		layout.rowItems = 2;
}

template<class Group>
void BaseVControllerButtonGroup<Group>::drawButtons(Gfx::RendererCommands &__restrict__ cmds) const
{
	auto &g = static_cast<const Group&>(*this);
	if(!g.enabledBtns)
		return;
	cmds.drawQuads(g.quads, 0, g.enabledBtns);
}

template<class Group>
void BaseVControllerButtonGroup<Group>::updateSprites()
{
	auto &g = static_cast<Group&>(*this);
	if(!g.quads.hasTask())
		return;
	g.quads.reset({.size = g.buttons.size()});
	Gfx::MappedBuffer<Gfx::Vertex2IColI> boundQuadsMap;
	if constexpr(requires {g.boundQuads;})
	{
		g.boundQuads.reset({.size = g.buttons.size()});
		boundQuadsMap = g.boundQuads.map(Gfx::BufferMapMode::indirect);
	}
	auto quadsMap = g.quads.map(Gfx::BufferMapMode::indirect);
	g.enabledBtns = 0;
	for(auto &b : g.buttons)
	{
		if(!b.enabled)
			continue;
		b.spriteIdx = g.enabledBtns++;
		if constexpr(requires {g.boundQuads;})
			b.updateSprite(quadsMap, boundQuadsMap);
		else
			b.updateSprite(quadsMap);
	}
}

template<class Group>
void BaseVControllerButtonGroup<Group>::updateSprite(VControllerButton &b)
{
	auto &g = static_cast<Group&>(*this);
	assert(b.spriteIdx < g.quads.size());
	if constexpr(requires {g.boundQuads;})
		b.updateSprite(g.quads, g.boundQuads);
	else
		b.updateSprite(g.quads);
}

template<class Group>
void BaseVControllerButtonGroup<Group>::setAlpha(float alpha)
{
	auto &g = static_cast<Group&>(*this);
	for(auto &b : g.buttons) { b.setAlpha(alpha); }
	updateSprites();
}

template class BaseVControllerButtonGroup<VControllerButtonGroup>;
template class BaseVControllerButtonGroup<VControllerUIButtonGroup>;

}
