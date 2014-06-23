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

#include <imagine/gui/BaseMenuView.hh>
#include <imagine/gui/AlertView.hh>
#include <EmuInput.hh>
class InputManagerView;

#ifdef CONFIG_BASE_ANDROID
#define BUTTONCONFIGVIEW_CHECK_SPURIOUS_EVENTS
#endif

class ButtonConfigSetView : public View
{
private:
	typedef DelegateFunc<void (const Input::Event &e)> SetDelegate;

	IG::WindowRect viewFrame;
	#ifdef INPUT_SUPPORTS_POINTER
	IG::WindowRect unbindB, cancelB;
	#endif
	char str[128] {0};
	Gfx::Text text;
	#ifdef INPUT_SUPPORTS_POINTER
	Gfx::Text unbind, cancel;
	#endif
	SetDelegate onSetD;
	const Input::Device *dev = nullptr;
	const Input::Device *savedDev = nullptr;
	InputManagerView &rootIMView;

	void initPointerUI();
	bool pointerUIIsInit();

public:
	constexpr ButtonConfigSetView(Base::Window &win, InputManagerView &rootIMView):
		View(win), rootIMView{rootIMView}
	{}

	IG::WindowRect &viewRect() { return viewFrame; }
	void init(Input::Device &dev, const char *actionName, bool withPointerInput, SetDelegate onSet);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw(Base::FrameTimeBase frameTime) override;
};

class ButtonConfigView : public BaseMenuView
{
private:
	struct BtnConfigMenuItem : public DualTextMenuItem
	{
		void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	};

	InputManagerView &rootIMView;
	TextMenuItem reset;
	MenuItem **text = nullptr;
	BtnConfigMenuItem *btn = nullptr;
	const KeyCategory *cat = nullptr;
	InputDeviceConfig *devConf = nullptr;
	#ifdef BUTTONCONFIGVIEW_CHECK_SPURIOUS_EVENTS
	Input::Time lastKeySetTime = 0;
	#endif

	void onSet(const Input::Event &e, int keyToSet);

public:
	ButtonConfigView(Base::Window &win, InputManagerView &rootIMView);

	void init(const KeyCategory *cat,
		InputDeviceConfig &devConf, bool highlightFirst);
	void inputEvent(const Input::Event &e) override;
	void deinit() override;
};
