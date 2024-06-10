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

#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuInput.hh>
#include <imagine/gfx/GfxText.hh>
#include <imagine/gfx/Quads.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/gui/MenuItem.hh>
#include <string>
#include <string_view>

namespace EmuEx
{

using namespace IG;
class InputManagerView;
class InputDeviceConfig;
struct KeyCategory;

class ButtonConfigSetView : public View, public EmuAppHelper
{
public:
	using SetDelegate = DelegateFunc<void (const MappedKeys &)>;

	ButtonConfigSetView(ViewAttachParams attach, InputManagerView &rootIMView,
		Input::Device &dev, std::string_view actionName, SetDelegate onSet);
	void place() final;
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) final;
	void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const final;
	void onAddedToController(ViewController *, const Input::Event &) final;

private:
	IG::WindowRect unbindB, cancelB;
	Gfx::Text text;
	Gfx::Text unbind, cancel;
	Gfx::IQuads quads;
	SetDelegate onSetD;
	const Input::Device &dev;
	const Input::Device *savedDev{};
	InputManagerView &rootIMView;
	std::string actionStr;
	MappedKeys pushedKeys;

	void initPointerUI();
	bool pointerUIIsInit() const;
	void finalize();
};

class ButtonConfigView : public TableView, public EmuAppHelper
{
public:
	ButtonConfigView(ViewAttachParams attach, InputManagerView &rootIMView, const KeyCategory &cat, InputDeviceConfig &devConf);
	bool inputEvent(const Input::Event&, ViewInputEventParams p = {}) final;

private:
	InputManagerView &rootIMView;
	TextMenuItem reset;
	TextMenuItem resetDefaults;
	std::unique_ptr<DualTextMenuItem[]> btn;
	const KeyCategory &cat;
	InputDeviceConfig &devConf;
	SteadyClockTimePoint leftKeyPushTime{};

	void onSet(int catIdx, MappedKeys);
	void updateKeyNames(const KeyConfig &);
};

}
