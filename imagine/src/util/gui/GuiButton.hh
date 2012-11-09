#pragma once

#include <engine-globals.h>
#include <util/rectangle2.h>
#include <gfx/GfxSprite.hh>

class GuiButton
{
public:
	Rect2<int> btn;
	Gfx::Sprite img;

	void draw ()
	{
		using namespace Gfx;
		loadTransforms(btn, LB2DO);
		img.draw(0);
	}

	void deinit ()
	{
		img.img->deinit();
		img.deinit();
	}

	CallResult init (ResourceImage *i)
	{
		img.init(0, 0, 1, 1, i);
		return OK;
	}
};
