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

#include <imagine/gui/TableView.hh>
#include <imagine/gui/AlertView.hh>
#include <emuframework/EmuInput.hh>
class InputManagerView;

#ifdef CONFIG_BASE_ANDROID
#define BUTTONCONFIGVIEW_CHECK_SPURIOUS_EVENTS
#endif

class ButtonConfigSetView : public View
{
private:
	typedef DelegateFunc<void (const Input::Event &e)> SetDelegate;

	IG::WindowRect viewFrame;
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	IG::WindowRect unbindB, cancelB;
	#endif
	char str[128]{};
	Gfx::Text text;
	#ifdef CONFIG_INPUT_POINTING_DEVICES
	Gfx::Text unbind, cancel;
	#endif
	SetDelegate onSetD;
	const Input::Device *dev{};
	const Input::Device *savedDev{};
	InputManagerView &rootIMView;

	void initPointerUI();
	bool pointerUIIsInit();

public:
	ButtonConfigSetView(Base::Window &win, InputManagerView &rootIMView):
		View(win), rootIMView{rootIMView}
	{}

	IG::WindowRect &viewRect() { return viewFrame; }
	void init(Input::Device &dev, const char *actionName, bool withPointerInput, SetDelegate onSet);
	void deinit() override;
	void place() override;
	void inputEvent(const Input::Event &e) override;
	void draw() override;
};

class ButtonConfigView : public TableView
{
private:
	struct BtnConfigMenuItem : public DualTextMenuItem
	{
		void draw(Gfx::GC xPos, Gfx::GC yPos, Gfx::GC xSize, Gfx::GC ySize, _2DOrigin align, const Gfx::ProjectionPlane &projP) const override;
	};

	InputManagerView &rootIMView;
	TextMenuItem reset;
	MenuItem **text{};
	BtnConfigMenuItem *btn{};
	const KeyCategory *cat{};
	InputDeviceConfig *devConf{};
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
