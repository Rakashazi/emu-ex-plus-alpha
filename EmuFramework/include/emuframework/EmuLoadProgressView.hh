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

#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <imagine/base/MessagePort.hh>

namespace EmuEx
{

using namespace IG;

class EmuLoadProgressView : public View, public EmuAppHelper<EmuLoadProgressView>
{
public:
	using MessagePortType = IG::MessagePort<EmuSystem::LoadProgressMessage>;

	EmuLoadProgressView(ViewAttachParams, const Input::Event &, EmuApp::CreateSystemCompleteDelegate);
	void setMax(int val);
	void setPos(int val);
	void setLabel(IG::utf16String label);
	void place() final;
	bool inputEvent(const Input::Event &) final;
	void draw(Gfx::RendererCommands &__restrict__) final;
	MessagePortType &messagePort();

private:
	MessagePortType msgPort{"EmuLoadProgressView"};
	EmuApp::CreateSystemCompleteDelegate onComplete{};
	Gfx::Text text;
	Input::Event originalEvent{};
	int pos = 0, max = 0;
};

}
