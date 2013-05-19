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

#include <util/gui/BaseMenuView.hh>
#include <gui/AlertView.hh>
#include <EmuInput.hh>

class ButtonConfigSetView : public View
{
private:
	typedef DelegateFunc<void (const Input::Event &e)> SetDelegate;

	Rect2<int> viewFrame;
	#ifdef INPUT_SUPPORTS_POINTER
	Rect2<int> unbindB, cancelB;
	#endif
	char str[128] {0};
	Gfx::Text text;
	#ifdef INPUT_SUPPORTS_POINTER
	Gfx::Text unbind, cancel;
	#endif
	SetDelegate onSetD;
	const Input::Device *dev = nullptr;
	const Input::Device *savedDev = nullptr;

	void initPointerUI();
	bool pointerUIIsInit();

public:
	constexpr ButtonConfigSetView() {}

	Rect2<int> &viewRect() { return viewFrame; }
	void init(Input::Device &dev, const char *actionName, bool withPointerInput, SetDelegate onSet);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Gfx::FrameTimeBase frameTime) override;
};

class ButtonConfigView : public BaseMenuView
{
private:
	struct BtnConfigMenuItem : public DualTextMenuItem
	{
		void draw(Coordinate xPos, Coordinate yPos, Coordinate xSize, Coordinate ySize, _2DOrigin align) const override;
	};

	TextMenuItem reset;
	MenuItem **text = nullptr;
	BtnConfigMenuItem *btn = nullptr;
	const KeyCategory *cat = nullptr;
	InputDeviceConfig *devConf = nullptr;

	void onSet(const Input::Event &e, int keyToSet);

public:
	ButtonConfigView();

	void init(const KeyCategory *cat,
		InputDeviceConfig &devConf, bool highlightFirst);
	void inputEvent(const Input::Event &e) override;
	void deinit() override;
};
