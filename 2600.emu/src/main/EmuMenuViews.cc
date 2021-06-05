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
#include <stella/emucore/Paddles.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuSystemActionsView.hh>
#undef Debugger
#include "internal.hh"

class CustomAudioOptionView : public AudioOptionView
{
	void setResampleQuality(AudioSettings::ResamplingQuality val)
	{
		logMsg("set resampling quality:%d", (int)val);
		optionAudioResampleQuality = (uint8_t)val;
		osystem->setResampleQuality(val);
	}

	TextMenuItem resampleQualityItem[3]
	{
		{"Low", &defaultFace(), [this](){ setResampleQuality(AudioSettings::ResamplingQuality::nearestNeightbour); }},
		{"High", &defaultFace(), [this](){ setResampleQuality(AudioSettings::ResamplingQuality::lanczos_2); }},
		{"Ultra", &defaultFace(), [this](){ setResampleQuality(AudioSettings::ResamplingQuality::lanczos_3); }},
	};

	MultiChoiceMenuItem resampleQuality
	{
		"Resampling Quality", &defaultFace(),
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
		{"70%", &defaultFace(), []() { setTVPhosphorBlend(70); }},
		{"80%", &defaultFace(), []() { setTVPhosphorBlend(80); }},
		{"90%", &defaultFace(), []() { setTVPhosphorBlend(90); }},
		{"100%", &defaultFace(), []() { setTVPhosphorBlend(100); }},
	};

	MultiChoiceMenuItem tvPhosphorBlend
	{
		"TV Phosphor Blending", &defaultFace(),
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

class ConsoleOptionView : public TableView, public EmuAppHelper<ConsoleOptionView>
{
	TextMenuItem tvPhosphorItem[3]
	{
		{"Off", &defaultFace(), []() { setTVPhosphor(0); }},
		{"On", &defaultFace(), []() { setTVPhosphor(1); }},
		{"Auto", &defaultFace(), []() { setTVPhosphor(TV_PHOSPHOR_AUTO); }},
	};

	MultiChoiceMenuItem tvPhosphor
	{
		"Simulate TV Phosphor", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 2 && osystem->hasConsole())
			{
				bool phospherInUse = osystem->console().properties().get(PropType::Display_Phosphor) == "YES";
				t.setString(phospherInUse ? "On" : "Off");
				return true;
			}
			else
				return false;
		},
		optionTVPhosphor,
		tvPhosphorItem
	};

	TextMenuItem videoSystemItem[7]
	{
		{"Auto", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(0, e); }},
		{"NTSC", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(1, e); }},
		{"PAL", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(2, e); }},
		{"SECAM", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(3, e); }},
		{"NTSC 50", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(4, e); }},
		{"PAL 60", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(5, e); }},
		{"SECAM 60", &defaultFace(), [this](TextMenuItem &, View &, Input::Event e) { setVideoSystem(6, e); }},
	};

	MultiChoiceMenuItem videoSystem
	{
		"Video System", &defaultFace(),
		[this](int idx, Gfx::Text &t)
		{
			if(idx == 0 && osystem->hasConsole())
			{
				t.setString(osystem->console().about().DisplayFormat.c_str());
				return true;
			}
			else
				return false;
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
		app().promptSystemReloadDueToSetOption(attachParams(), e);
	}

	TextMenuItem inputPortsItem[4]
	{
		{"Auto", &defaultFace(), [this](){ setInputPorts(Controller::Type::Unknown); }},
		{"Joystick", &defaultFace(), [this](){ setInputPorts(Controller::Type::Joystick); }},
		{"Genesis Gamepad", &defaultFace(), [this](){ setInputPorts(Controller::Type::Genesis); }},
		{"Paddles", &defaultFace(), [this](){ setInputPorts(Controller::Type::Paddles); }},
	};

	MultiChoiceMenuItem inputPorts
	{
		"Input Ports", &defaultFace(),
		[](int idx, Gfx::Text &t)
		{
			if(idx == 0 && osystem->hasConsole())
			{
				t.setString(controllerTypeStr(osystem->console().leftController().type()));
				return true;
			}
			else
				return false;
		},
		[]()
		{
			if((Controller::Type)optionInputPort1.val == Controller::Type::Joystick)
				return 1;
			else if((Controller::Type)optionInputPort1.val == Controller::Type::Genesis)
				return 2;
			else if((Controller::Type)optionInputPort1.val == Controller::Type::Paddles)
				return 3;
			else
				return 0;
		}(),
		inputPortsItem
	};

	void setInputPorts(Controller::Type type)
	{
		EmuSystem::sessionOptionSet();
		optionInputPort1 = (uint8_t)type;
		if(osystem->hasConsole())
		{
			setControllerType(app(), osystem->console(), type);
		}
	}

	TextMenuItem dPaddleSensitivityItem[2]
	{
		{"Default", &defaultFace(), [this]() { setDPaddleSensitivity(1); }},
		{"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueInputView<int>(attachParams(), e, "Input 1 to 20", "",
					[this](EmuApp &app, auto val)
					{
						if(optionPaddleDigitalSensitivity.isValidVal(val))
						{
							setDPaddleSensitivity(val);
							dPaddleSensitivity.setSelected(std::size(dPaddleSensitivityItem) - 1, *this);
							dismissPrevious();
							return true;
						}
						else
						{
							app.postErrorMessage("Value not in range");
							return false;
						}
					});
				return false;
			}
		}
	};

	MultiChoiceMenuItem dPaddleSensitivity
	{
		"Digital Paddle Sensitivity", &defaultFace(),
		[this](uint32_t idx, Gfx::Text &t)
		{
			t.setString(string_makePrintf<4>("%u", optionPaddleDigitalSensitivity.val).data());
			return true;
		},
		[]()
		{
			switch(optionPaddleDigitalSensitivity)
			{
				case 1: return 0;
				default: return 1;
			}
		}(),
		dPaddleSensitivityItem
	};

	void setDPaddleSensitivity(uint8_t val)
	{
		EmuSystem::sessionOptionSet();
		optionPaddleDigitalSensitivity = val;
		Paddles::setDigitalSensitivity(optionPaddleDigitalSensitivity);
	}

	std::array<MenuItem*, 4> menuItem
	{
		&tvPhosphor,
		&videoSystem,
		&inputPorts,
		&dPaddleSensitivity,
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
		"Left (P1) Difficulty", &defaultFace(),
		p1DiffB,
		"A", "B",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			p1DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem diff2
	{
		"Right (P2) Difficulty", &defaultFace(),
		p2DiffB,
		"A", "B",
		[this](BoolMenuItem &item, View &, Input::Event e)
		{
			p2DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem color
	{
		"Color", &defaultFace(),
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
		"Console Switches", &defaultFace(),
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
		"Console Options", &defaultFace(),
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
