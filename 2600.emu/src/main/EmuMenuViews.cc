/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

// TODO: Some Stella types collide with MacTypes.h
#define BytePtr BytePtrMac
#define Debugger DebuggerMac
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#undef BytePtr
#undef Debugger
#undef HAVE_UNISTD_H
#include "internal.hh"

class ConsoleOptionView : public TableView
{
	TextMenuItem tvPhosphorItem[3]
	{
		{"Off", []() { setTVPhosphor(0); }},
		{"On", []() { setTVPhosphor(1); }},
		{"Auto", []() { setTVPhosphor(TV_PHOSPHOR_AUTO); }},
	};

	MultiChoiceMenuItem tvPhosphor
	{
		"Simulate TV Phosphor",
		[this](int idx) -> const char*
		{
			if(idx == 2)
			{
				bool phospherInUse = osystem.console().properties().get(Display_Phosphor) == "YES";
				return phospherInUse ? "On" : "Off";
			}
			else
				return nullptr;
		},
		optionTVPhosphor,
		tvPhosphorItem
	};

	TextMenuItem videoSystemItem[7]
	{
		{"Auto", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(0, e); }},
		{"NTSC", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(1, e); }},
		{"PAL", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(2, e); }},
		{"SECAM", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(3, e); }},
		{"NTSC 50", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(4, e); }},
		{"PAL 60", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(5, e); }},
		{"SECAM 60", [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(6, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return osystem.console().about().DisplayFormat.c_str();
			}
			else
				return nullptr;
		},
		optionVideoSystem,
		videoSystemItem
	};

	static void setTVPhosphor(uint val)
	{
		EmuSystem::sessionOptionSet();
		optionTVPhosphor = val;
		setRuntimeTVPhosphor(val);
	}

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		EmuApp::promptSystemReloadDueToSetOption(attachParams(), e);
	}

	std::array<MenuItem*, 2> menuItem
	{
		&tvPhosphor,
		&videoSystem
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{}
};

class VCSSwitchesView : public TableView
{
	BoolMenuItem diff1
	{
		"Left (P1) Difficulty",
		p1DiffB,
		"A", "B",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			p1DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem diff2
	{
		"Right (P2) Difficulty",
		p2DiffB,
		"A", "B",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			p2DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem color
	{
		"Color",
		vcsColor,
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			vcsColor = item.flipBoolValue(*this);
		}
	};

	std::array<MenuItem*, 3> menuItem
	{
		&diff1,
		&diff2,
		&color
	};

public:
	VCSSwitchesView(ViewAttachParams attach):
		TableView
		{
			"Switches",
			attach,
			menuItem
		}
	{}

	void onShow() final
	{
		diff1.setBoolValue(p1DiffB, *this);
		diff2.setBoolValue(p2DiffB, *this);
		color.setBoolValue(vcsColor, *this);
	}

};

class CustomSystemActionsView : public EmuSystemActionsView
{
private:
	TextMenuItem switches
	{
		"Console Switches",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &vcsSwitchesView = *new VCSSwitchesView{attachParams()};
				pushAndShow(vcsSwitchesView, e);
			}
		}
	};

	TextMenuItem options
	{
		"Console Options",
		[this](TextMenuItem &, View &, Input::Event e)
		{
			if(EmuSystem::gameIsRunning())
			{
				auto &optionView = *new ConsoleOptionView{attachParams()};
				pushAndShow(optionView, e);
			}
		}
	};

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		item.emplace_back(&switches);
		item.emplace_back(&options);
		loadStandardItems();
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return new CustomSystemActionsView(attach);
		default: return nullptr;
	}
}
