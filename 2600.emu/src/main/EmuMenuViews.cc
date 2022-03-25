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
#include <SoundEmuEx.hh>
#include <stella/emucore/Paddles.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define Debugger DebuggerMac
#include <emuframework/OptionView.hh>
#include <emuframework/EmuApp.hh>
#include <emuframework/EmuAppHelper.hh>
#include <emuframework/EmuSystemActionsView.hh>
#undef Debugger
#include "internal.hh"
#include <imagine/util/format.hh>

namespace EmuEx
{

class CustomAudioOptionView : public AudioOptionView
{
	TextMenuItem::SelectDelegate setResampleQualityDel()
	{
		return [](TextMenuItem &item)
		{
			logMsg("set resampling quality:%d", item.id());
			optionAudioResampleQuality = item.id();
			osystem->soundEmuEx().setResampleQuality((AudioSettings::ResamplingQuality)item.id());
		};
	}

	TextMenuItem resampleQualityItem[3]
	{
		{"Low",   &defaultFace(), setResampleQualityDel(), (int)AudioSettings::ResamplingQuality::nearestNeightbour},
		{"High",  &defaultFace(), setResampleQualityDel(), (int)AudioSettings::ResamplingQuality::lanczos_2},
		{"Ultra", &defaultFace(), setResampleQualityDel(), (int)AudioSettings::ResamplingQuality::lanczos_3},
	};

	MultiChoiceMenuItem resampleQuality
	{
		"Resampling Quality", &defaultFace(),
		(MenuItem::Id)optionAudioResampleQuality.val,
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
		{"70%",  &defaultFace(), setTVPhosphorBlendDel(), 70},
		{"80%",  &defaultFace(), setTVPhosphorBlendDel(), 80},
		{"90%",  &defaultFace(), setTVPhosphorBlendDel(), 90},
		{"100%", &defaultFace(), setTVPhosphorBlendDel(), 100},
	};

	MultiChoiceMenuItem tvPhosphorBlend
	{
		"TV Phosphor Blending", &defaultFace(),
		(MenuItem::Id)optionTVPhosphorBlend.val,
		tvPhosphorBlendItem
	};

	TextMenuItem::SelectDelegate setTVPhosphorBlendDel()
	{
		return [this](TextMenuItem &item)
		{
			optionTVPhosphorBlend = item.id();
			setRuntimeTVPhosphor(system(), optionTVPhosphor, item.id());
		};
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
		{"Off",  &defaultFace(), setTVPhosphorDel(), 0},
		{"On",   &defaultFace(), setTVPhosphorDel(), 1},
		{"Auto", &defaultFace(), setTVPhosphorDel(), TV_PHOSPHOR_AUTO},
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
		(MenuItem::Id)optionTVPhosphor.val,
		tvPhosphorItem
	};

	TextMenuItem videoSystemItem[7]
	{
		{"Auto",     &defaultFace(), setVideoSystemDel(), 0},
		{"NTSC",     &defaultFace(), setVideoSystemDel(), 1},
		{"PAL",      &defaultFace(), setVideoSystemDel(), 2},
		{"SECAM",    &defaultFace(), setVideoSystemDel(), 3},
		{"NTSC 50",  &defaultFace(), setVideoSystemDel(), 4},
		{"PAL 60",   &defaultFace(), setVideoSystemDel(), 5},
		{"SECAM 60", &defaultFace(), setVideoSystemDel(), 6},
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
		(MenuItem::Id)optionVideoSystem.val,
		videoSystemItem
	};

	TextMenuItem::SelectDelegate setTVPhosphorDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			optionTVPhosphor = item.id();
			setRuntimeTVPhosphor(system(), item.id(), optionTVPhosphorBlend);
		};
	}

	TextMenuItem::SelectDelegate setVideoSystemDel()
	{
		return [this](TextMenuItem &item, const Input::Event &e)
		{
			system().sessionOptionSet();
			optionVideoSystem = item.id();
			app().promptSystemReloadDueToSetOption(attachParams(), e);
		};
	}

	TextMenuItem inputPortsItem[4]
	{
		{"Auto",            &defaultFace(), setInputPortsDel(), (int)Controller::Type::Unknown},
		{"Joystick",        &defaultFace(), setInputPortsDel(), (int)Controller::Type::Joystick},
		{"Genesis Gamepad", &defaultFace(), setInputPortsDel(), (int)Controller::Type::Genesis},
		{"Paddles",         &defaultFace(), setInputPortsDel(), (int)Controller::Type::Paddles},
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
		(MenuItem::Id)optionInputPort1.val,
		inputPortsItem
	};

	TextMenuItem::SelectDelegate setInputPortsDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			optionInputPort1 = item.id();
			if(osystem->hasConsole())
			{
				setControllerType(app(), osystem->console(), (Controller::Type)item.id());
			}
		};
	}

	TextMenuItem aPaddleRegionItem[4]
	{
		{"Off",        &defaultFace(), setAPaddleRegionDel(), (int)PaddleRegionMode::OFF},
		{"Left Half",  &defaultFace(), setAPaddleRegionDel(), (int)PaddleRegionMode::LEFT},
		{"Right Half", &defaultFace(), setAPaddleRegionDel(), (int)PaddleRegionMode::RIGHT},
		{"Full",       &defaultFace(), setAPaddleRegionDel(), (int)PaddleRegionMode::FULL},
	};

	MultiChoiceMenuItem aPaddleRegion
	{
		"Analog Paddle Region", &defaultFace(),
		(MenuItem::Id)optionPaddleAnalogRegion.val,
		aPaddleRegionItem
	};

	TextMenuItem::SelectDelegate setAPaddleRegionDel()
	{
		return [this](TextMenuItem &item)
		{
			system().sessionOptionSet();
			updatePaddlesRegionMode(app(), (PaddleRegionMode)item.id());
		};
	}

	TextMenuItem dPaddleSensitivityItem[2]
	{
		{"Default", &defaultFace(), [this]() { setDPaddleSensitivity(1); }, 1},
		{"Custom Value", &defaultFace(),
			[this](const Input::Event &e)
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
			t.setString(fmt::format("{}", optionPaddleDigitalSensitivity.val));
			return true;
		},
		(MenuItem::Id)optionPaddleDigitalSensitivity.val,
		dPaddleSensitivityItem
	};

	void setDPaddleSensitivity(uint8_t val)
	{
		system().sessionOptionSet();
		optionPaddleDigitalSensitivity = val;
		Paddles::setDigitalSensitivity(optionPaddleDigitalSensitivity);
	}

	std::array<MenuItem*, 5> menuItem
	{
		&tvPhosphor,
		&videoSystem,
		&inputPorts,
		&aPaddleRegion,
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
		[this](BoolMenuItem &item)
		{
			p1DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem diff2
	{
		"Right (P2) Difficulty", &defaultFace(),
		p2DiffB,
		"A", "B",
		[this](BoolMenuItem &item)
		{
			p2DiffB = item.flipBoolValue(*this);
		}
	};

	BoolMenuItem color
	{
		"Color", &defaultFace(),
		vcsColor,
		[this](BoolMenuItem &item)
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
		[this](const Input::Event &e)
		{
			if(system().hasContent())
			{
				pushAndShow(makeView<VCSSwitchesView>(), e);
			}
		}
	};

	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](const Input::Event &e)
		{
			if(system().hasContent())
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

}
