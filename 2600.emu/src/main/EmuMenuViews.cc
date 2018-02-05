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

class CustomVideoOptionView : public VideoOptionView
{
	TextMenuItem tvPhosphorItem[3];
	MultiChoiceMenuItem tvPhosphor;
	TextMenuItem videoSystemItem[7];
	MultiChoiceMenuItem videoSystem;

	static void setTVPhosphor(uint val)
	{
		optionTVPhosphor.val = val;
		if(!EmuSystem::gameIsRunning())
		{
			return;
		}

		// change runtime phosphor value
		bool usePhosphor = false;
		if((int)optionTVPhosphor == TV_PHOSPHOR_AUTO)
		{
			usePhosphor = defaultGameProps.get(Display_Phosphor) == "YES";
		}
		else
		{
			usePhosphor = optionTVPhosphor;
		}
		bool phospherInUse = osystem.console().properties().get(Display_Phosphor) == "YES";
		logMsg("Phosphor effect %s", usePhosphor ? "on" : "off");
		if(usePhosphor != phospherInUse)
		{
			logMsg("toggling phoshpor on console");
			osystem.console().togglePhosphor();
		}
	}

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true},
	tvPhosphorItem
	{
		{"Off", []() { setTVPhosphor(0); }},
		{"On", []() { setTVPhosphor(1); }},
		{"Auto", []() { setTVPhosphor(TV_PHOSPHOR_AUTO); }},
	},
	tvPhosphor
	{
		"Simulate TV Phosphor",
		std::min((int)optionTVPhosphor, 2),
		tvPhosphorItem
	},
	videoSystemItem
	{
		{"Auto", []() { optionVideoSystem = 0; }},
		{"NTSC", []() { optionVideoSystem = 1; }},
		{"PAL", []() { optionVideoSystem = 2; }},
		{"SECAM", []() { optionVideoSystem = 3; }},
		{"NTSC 50", []() { optionVideoSystem = 4; }},
		{"PAL 60", []() { optionVideoSystem = 5; }},
		{"SECAM 60", []() { optionVideoSystem = 6; }},
	},
	videoSystem
	{
		"Video System",
		optionVideoSystem,
		videoSystemItem
	}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&tvPhosphor);
		item.emplace_back(&videoSystem);
	}
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

public:
	VCSSwitchesView(ViewAttachParams attach):
		TableView
		{
			"Switches",
			attach,
			[this](const TableView &)
			{
				return 3;
			},
			[this](const TableView &, uint idx) -> MenuItem&
			{
				MenuItem *item[]
				{
					&diff1,
					&diff2,
					&color
				};
				return *item[idx];
			}
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

	void reloadItems()
	{
		item.clear();
		item.emplace_back(&switches);
		loadStandardItems();
	}

public:
	CustomSystemActionsView(ViewAttachParams attach): EmuSystemActionsView{attach, true}
	{
		reloadItems();
	}

	void onShow() final
	{
		EmuSystemActionsView::onShow();
		switches.setActive(EmuSystem::gameIsRunning());
	}
};

View *EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return new CustomSystemActionsView(attach);
		case ViewID::VIDEO_OPTIONS: return new CustomVideoOptionView(attach);
		default: return nullptr;
	}
}
