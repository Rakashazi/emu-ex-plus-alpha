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
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/gui/View.hh>
#include <imagine/logger/logger.h>

void VControllerButton::setPos(IG::WP pos, Gfx::ProjectionPlane projP, _2DOrigin o)
{
	bounds_.setPos(pos, o);
	bounds_.fitIn(projP.viewport().bounds());
	extendedBounds_.setPos(bounds_.pos(C2DO), C2DO);
	IG::WindowRect spriteBounds{{0, 0}, {bounds_.xSize(), (int)(bounds_.ySize() / aspectRatio)}};
	spriteBounds.setPos(bounds_.pos(C2DO), C2DO);
	spr.setPos(spriteBounds, projP);
}

void VControllerButton::setSize(IG::WP size, IG::WP extendedSize)
{
	bounds_ = IG::makeWindowRectRel(bounds_.pos(C2DO), size);
	extendedBounds_ = bounds_ + IG::WindowRect{{-extendedSize}, {extendedSize}};
}

void VControllerButton::setImage(Gfx::TextureSpan img, float aR)
{
	spr.setImg(img);
	aspectRatio = aR;
}

void VControllerButton::setState(VControllerState state)
{
	state_ = state;
}

void VControllerButton::setShowBounds(bool on)
{
	showBoundingArea = on;
}

void VControllerButton::setShouldSkipLayout(bool on)
{
	skipLayout = on;
}

void VControllerButton::setEnabled(bool on)
{
	disabled = !on;
}

void VControllerButton::draw(Gfx::RendererCommands &cmds, std::optional<Gfx::Color> col, bool showHidden) const
{
	if(!VController::shouldDraw(state(), showHidden))
		return;
	if(col)
		cmds.setColor(*col);
	cmds.set(View::imageCommonTextureSampler);
	spr.setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
	spr.draw(cmds);
}

VControllerButtonGroup::VControllerButtonGroup(int size):
	btns{(size_t)size}
{}

void VControllerButtonGroup::setPos(IG::WP pos, Gfx::ProjectionPlane projP)
{
	int btnsPerRow = buttonsPerRow();
	//logMsg("laying out %d buttons in %d row(s)", buttonsToLayout(), rows());
	bounds_.setPos(pos, C2DO);
	bounds_.fitIn(projP.viewport().bounds());
	auto btnArea = bounds_;
	int row{}, btnPos{}, y{-btnSize.y};
	int stagger = btnStagger;
	if(stagger < 0)
		y += stagger*(btnsPerRow-1);
	int x = -btnRowShift*(rows()-1);
	int staggerOffset = 0;
	for(auto &b : btns)
	{
		if(b.shouldSkipLayout() || !b.isEnabled())
			continue;
		IG::WP pos = btnArea.pos(LB2DO) + IG::WP{x, y + staggerOffset} + (btnSize/2);
		b.setPos(pos, projP);
		x += btnSize.x + btnSpace;
		staggerOffset -= stagger;
		if(++btnPos == btnsPerRow)
		{
			row++;
			y -= btnSize.y + btnSpace;
			staggerOffset = 0;
			x = -btnRowShift*((rows()-1)-row);
			btnPos = 0;
		}
	}
}

void VControllerButtonGroup::setState(VControllerState state)
{
	state_ = state;
}

void VControllerButtonGroup::setButtonSize(IG::WP size, IG::WP extendedSize)
{
	btnSize = size;
	setStaggerType(btnStaggerType);
	int btnsPerRow = buttonsPerRow();
	int xSizePixel = size.x*btnsPerRow + btnSpace*(btnsPerRow-1) + std::abs(btnRowShift*((int)rows()-1));
	int ySizePixel = size.y*rows() + btnSpace*(rows()-1) + std::abs(btnStagger*((int)btnsPerRow-1));
	bounds_ = IG::makeWindowRectRel({0, 0}, {xSizePixel, ySizePixel});
	for(auto &b : btns)
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
		bcase 0:
			btnStagger = btnSize.y * -.75;
		bcase 1:
			btnStagger = btnSize.y * -.5;
		bcase 2:
			btnStagger = 0;
		bcase 3:
			btnStagger = btnSize.y * .5;
		bcase 4:
			btnStagger = btnSize.y * .75;
		bdefault:
			btnStagger = btnSize.y + btnSpace;
			btnRowShift = -(btnSize.y + btnSpace);
	}
}

void VControllerButtonGroup::setSpacing(int16_t space)
{
	btnSpace = space;
	setStaggerType(btnStaggerType);
}

void VControllerButtonGroup::setShowBounds(bool on)
{
	showBoundingArea = on;
}

int VControllerButtonGroup::rows() const
{
	return buttonsToLayout() <= 3 ? 1 : 2;
}

int VControllerButtonGroup::buttonsToLayout() const
{
	int count{};
	for(const auto &b : btns)
	{
		if(b.shouldSkipLayout() || !b.isEnabled())
			continue;
		count++;
	}
	return count;
}

int VControllerButtonGroup::buttonsPerRow() const
{
	return buttonsToLayout() / rows();
}

std::array<int, 2> VControllerButtonGroup::findButtonIndices(IG::WP windowPos) const
{
	std::array<int, 2> btnOut{-1, -1};
	if(state() == VControllerState::OFF)
		return btnOut;
	for(unsigned count = 0; auto &b : buttons())
	{
		if(b.isEnabled() && b.realBounds().overlaps(windowPos))
		{
			btnOut[count++] = std::distance(buttons().data(), &b);
			if(count == btnOut.size())
				break;
		}
	}
	return btnOut;
}

void VControllerButtonGroup::draw(Gfx::RendererCommands &cmds, Gfx::ProjectionPlane projP, bool showHidden) const
{
	if(!VController::shouldDraw(state(), showHidden))
		return;
	cmds.set(View::imageCommonTextureSampler);
	if(showBoundingArea)
	{
		cmds.setCommonProgram(Gfx::CommonProgram::NO_TEX);
		for(const auto &b : btns)
		{
			if(!b.isEnabled())
				continue;
			Gfx::GeomRect::draw(cmds, b.realBounds(), projP);
		}
	}
	//cmds.setCommonProgram(Gfx::CommonProgram::NO_TEX);
	//Gfx::GeomRect::draw(cmds, bounds(), projP);
	btns[0].sprite().setCommonProgram(cmds, Gfx::IMG_MODE_MODULATE);
	for(auto &b : btns)
	{
		if(!b.isEnabled())
			continue;
		b.sprite().draw(cmds);
	}
}

std::vector<VControllerButton> &VControllerButtonGroup::buttons()
{
	return btns;
}

const std::vector<VControllerButton> &VControllerButtonGroup::buttons() const
{
	return btns;
}
