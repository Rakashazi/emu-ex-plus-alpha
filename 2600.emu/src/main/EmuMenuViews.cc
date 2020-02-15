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

#include <OSystem.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuSystemActionsView.hh>
#undef Debugger
#include "internal.hh"

class CustomAudioOptionView : public AudioOptionView
{
	void setResampleQuality(AudioSettings::ResamplingQuality val)
	{
		auto os = osystem.get();
		logMsg("set resampling quality:%d", (int)val);
		optionAudioResampleQuality = (uint8_t)val;
		os->setResampleQuality(val);
	}

	TextMenuItem resampleQualityItem[3]
	{
		{"Low", [this](){ setResampleQuality(AudioSettings::ResamplingQuality::nearestNeightbour); }},
		{"High", [this](){ setResampleQuality(AudioSettings::ResamplingQuality::lanczos_2); }},
		{"Ultra", [this](){ setResampleQuality(AudioSettings::ResamplingQuality::lanczos_3); }},
	};

	MultiChoiceMenuItem resampleQuality
	{
		"Resampling Quality",
		[]()
		{
			switch((AudioSettings::ResamplingQuality)optionAudioResampleQuality.val)
			{
				default: return 0;
				case AudioSettings::ResamplingQuality::lanczos_2: return 1;
				case AudioSettings::ResamplingQuality::lanczos_3: return 2;
			}
		}(),
		resampleQualityItem
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&resampleQuality);
	}
};

class CustomVideoOptionView : public VideoOptionView
{
	TextMenuItem tvPhosphorBlendItem[4]
	{
		{"70%", []() { setTVPhosphorBlend(70); }},
		{"80%", []() { setTVPhosphorBlend(80); }},
		{"90%", []() { setTVPhosphorBlend(90); }},
		{"100%", []() { setTVPhosphorBlend(100); }},
	};

	MultiChoiceMenuItem tvPhosphorBlend
	{
		"TV Phosphor Blending",
		[]()
		{
			switch(optionTVPhosphorBlend)
			{
				case 70: return 0;
				default: return 1;
				case 90: return 2;
				case 100: return 3;
			}
		}(),
		tvPhosphorBlendItem
	};

	static void setTVPhosphorBlend(uint val)
	{
		optionTVPhosphorBlend = val;
		setRuntimeTVPhosphor(optionTVPhosphor, val);
	}

public:
	CustomVideoOptionView(ViewAttachParams attach): VideoOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&systemSpecificHeading);
		item.emplace_back(&tvPhosphorBlend);
	}
};

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
				bool phospherInUse = osystem->console().properties().get(PropType::Display_Phosphor) == "YES";
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

	std::array<char, 12> autoVideoSystemStr{};

	MultiChoiceMenuItem videoSystem
	{
		"Video System",
		[this](int idx) -> const char*
		{
			if(idx == 0)
			{
				return autoVideoSystemStr.data();
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
		setRuntimeTVPhosphor(val, optionTVPhosphorBlend);
	}

	void setVideoSystem(int val, Input::Event e)
	{
		EmuSystem::sessionOptionSet();
		optionVideoSystem = val;
		EmuApp::promptSystemReloadDueToSetOption(attachParams(), e);
	}

	TextMenuItem inputPortsItem[3]
	{
		{"Auto", [](){ setInputPorts(Controller::Type::Unknown); }},
		{"Joystick", [](){ setInputPorts(Controller::Type::Joystick); }},
		{"Genesis Gamepad", [](){ setInputPorts(Controller::Type::Genesis); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports",
		[](int idx) -> const char*
		{
			if(idx == 0 && osystem->hasConsole())
			{
				return controllerTypeStr(osystem->console().leftController().type());
			}
			else
				return nullptr;
		},
		[]()
		{
			if((Controller::Type)optionInputPort1.val == Controller::Type::Joystick)
				return 1;
			else if((Controller::Type)optionInputPort1.val == Controller::Type::Genesis)
				return 2;
			else
				return 0;
		}(),
		inputPortsItem
	};

	static void setInputPorts(Controller::Type type)
	{
		EmuSystem::sessionOptionSet();
		optionInputPort1 = (uint8_t)type;
		if(osystem->hasConsole())
		{
			setControllerType(osystem->console(), type);
		}
	}

	std::array<MenuItem*, 3> menuItem
	{
		&tvPhosphor,
		&videoSystem,
		&inputPorts,
	};

public:
	ConsoleOptionView(ViewAttachParams attach):
		TableView
		{
			"Console Options",
			attach,
			menuItem
		}
	{
		if(osystem->hasConsole())
			string_copy(autoVideoSystemStr, osystem->console().about().DisplayFormat.c_str());
	}

	void onShow() final
	{
		if(osystem->hasConsole())
			string_copy(autoVideoSystemStr, osystem->console().about().DisplayFormat.c_str());
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
				pushAndShow(makeView<VCSSwitchesView>(), e);
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
				pushAndShow(makeView<ConsoleOptionView>(), e);
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

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::VIDEO_OPTIONS: return std::make_unique<CustomVideoOptionView>(attach);
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		default: return nullptr;
	}
}
