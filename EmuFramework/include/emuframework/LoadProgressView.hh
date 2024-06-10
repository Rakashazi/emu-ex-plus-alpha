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
#include <imagine/gfx/Quads.hh>

namespace EmuEx
{

using namespace IG;

class LoadProgressView : public View, public EmuAppHelper
{
public:
	using MessagePortType = IG::MessagePort<EmuSystem::LoadProgressMessage>;

	LoadProgressView(ViewAttachParams, const Input::Event &, EmuApp::CreateSystemCompleteDelegate);
	void setMax(int val);
	void setPos(int val);
	void setLabel(UTF16Convertible auto &&label) { text.resetString(IG_forward(label)); }
	void place() final;
	void draw(Gfx::RendererCommands&__restrict__, ViewDrawParams p = {}) const final;
	MessagePortType &messagePort();

private:
	MessagePortType msgPort{"LoadProgressView"};
	EmuApp::CreateSystemCompleteDelegate onComplete;
	Gfx::Text text;
	Gfx::IQuads progessBarQuads;
	Input::Event originalEvent;
	int pos{}, max{1};

	void updateProgressRect();
};

}
