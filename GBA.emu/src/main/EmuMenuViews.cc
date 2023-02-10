/*  This file is part of GBA.emu.

	GBA.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	GBA.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with GBA.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuApp.hh>
#include <emuframework/SystemOptionView.hh>
#include <emuframework/AudioOptionView.hh>
#include <emuframework/FilePathOptionView.hh>
#include <emuframework/UserPathSelectView.hh>
#include <emuframework/EmuSystemActionsView.hh>
#include "EmuCheatViews.hh"
#include "MainApp.hh"
#include <imagine/gui/AlertView.hh>
#include <imagine/util/format.hh>
#include <vbam/gba/GBA.h>
#include <vbam/gba/RTC.h>
#include <vbam/gba/Sound.h>

namespace EmuEx
{

template <class T>
using MainAppHelper = EmuAppHelper<T, MainApp>;

class ConsoleOptionView : public TableView, public MainAppHelper<ConsoleOptionView>
{
	TextMenuItem rtcItem[3]
	{
		{"Auto", &defaultFace(), [this](){ setRTCEmulation(RtcMode::AUTO); }},
		{"Off",  &defaultFace(), [this](){ setRTCEmulation(RtcMode::OFF); }},
		{"On",   &defaultFace(), [this](){ setRTCEmulation(RtcMode::ON); }},
	};

	MultiChoiceMenuItem rtc
	{
		"RTC Emulation", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(rtcIsEnabled() ? "On" : "Off");
					return true;
				}
				return false;
			}
		},
		system().optionRtcEmulation.val,
		rtcItem
	};

	void setRTCEmulation(RtcMode val)
	{
		system().sessionOptionSet();
		system().optionRtcEmulation = std::to_underlying(val);
		system().setRTC(val);
	}

	TextMenuItem saveTypeItem[7]
	{
		{"Auto",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_AUTO)},
		{"EEPROM",          &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_EEPROM)},
		{"SRAM",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_SRAM)},
		{"Flash (64K)",     &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_FLASH, SIZE_FLASH512)},
		{"Flash (128K)",    &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_FLASH, SIZE_FLASH1M)},
		{"EEPROM + Sensor", &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_EEPROM_SENSOR)},
		{"None",            &defaultFace(), setSaveTypeDel(), packSaveTypeOverride(GBA_SAVE_NONE)},
	};

	MultiChoiceMenuItem saveType
	{
		"Save Type", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(saveTypeStr(system().detectedSaveType, system().detectedSaveSize));
					return true;
				}
				return false;
			}
		},
		(MenuItem::Id)system().optionSaveTypeOverride.val,
		saveTypeItem
	};

	TextMenuItem::SelectDelegate setSaveTypeDel()
	{
		return [this](TextMenuItem &item, const Input::Event &e)
		{
			if(system().optionSaveTypeOverride == (uint32_t)item.id())
				return true;
			static auto setSaveTypeOption = [](GbaApp &app, uint32_t optVal, ViewAttachParams attach, const Input::Event &e)
			{
				app.system().sessionOptionSet();
				app.system().optionSaveTypeOverride = optVal;
				app.promptSystemReloadDueToSetOption(attach, e);
			};
			if(saveMemoryHasContent())
			{
				auto ynAlertView = std::make_unique<YesNoAlertView>(attachParams(),
					"Really change save type? Existing data in .sav file may be lost so please make a backup before proceeding.");
				ynAlertView->setOnYes(
					[this, optVal = item.id()](const Input::Event &e)
					{
						setSaveTypeOption(app(), optVal, attachParams(), e);
					});
				pushAndShowModal(std::move(ynAlertView), e);
				return false;
			}
			else
			{
				setSaveTypeOption(app(), item.id(), attachParams(), e);
				return true;
			}
		};
	}

	#ifdef IG_CONFIG_SENSORS
	TextMenuItem hardwareSensorItem[5]
	{
		{"Auto",          &defaultFace(), setHardwareSensorDel(), to_underlying(GbaSensorType::Auto)},
		{"None",          &defaultFace(), setHardwareSensorDel(), to_underlying(GbaSensorType::None)},
		{"Accelerometer", &defaultFace(), setHardwareSensorDel(), to_underlying(GbaSensorType::Accelerometer)},
		{"Gyroscope",     &defaultFace(), setHardwareSensorDel(), to_underlying(GbaSensorType::Gyroscope)},
		{"Light",         &defaultFace(), setHardwareSensorDel(), to_underlying(GbaSensorType::Light)},
	};

	MultiChoiceMenuItem hardwareSensor
	{
		"Hardware Sensor", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(wise_enum::to_string(system().detectedSensorType));
					return true;
				}
				return false;
			}
		},
		(MenuItem::Id)system().sensorType,
		hardwareSensorItem
	};

	TextMenuItem::SelectDelegate setHardwareSensorDel()
	{
		return [this](TextMenuItem &item) { system().setSensorType((GbaSensorType)item.id()); };
	}
	#endif

	std::array<MenuItem*, Config::SENSORS ? 3 : 2> menuItem
	{
		&rtc
		, &saveType
		#ifdef IG_CONFIG_SENSORS
		, &hardwareSensor
		#endif
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

class CustomSystemActionsView : public EmuSystemActionsView
{
	TextMenuItem options
	{
		"Console Options", &defaultFace(),
		[this](TextMenuItem &, View &, Input::Event e)
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
		item.emplace_back(&options);
		loadStandardItems();
	}
};

class CustomAudioOptionView : public AudioOptionView, public MainAppHelper<CustomAudioOptionView>
{
	using MainAppHelper<CustomAudioOptionView>::system;
	using MainAppHelper<CustomAudioOptionView>::app;

	TextHeadingMenuItem mixer{"Mixer", &defaultBoldFace()};

	using VolumeChoiceItemArr = std::array<TextMenuItem, 3>;

	VolumeChoiceItemArr volumeLevelChoiceItems(bool gbVol)
	{
		return
		{
			TextMenuItem
			{
				"Default", &defaultFace(),
				[this, gbVol]() { soundSetVolume(gGba, 1.f, gbVol); },
				100
			},
			TextMenuItem
			{
				"Off", &defaultFace(),
				[this, gbVol]() { soundSetVolume(gGba, 0, gbVol); },
				0
			},
			TextMenuItem
			{
				"Custom Value", &defaultFace(),
				[this, gbVol](Input::Event e)
				{
					app().pushAndShowNewCollectValueRangeInputView<int, 0, 100>(attachParams(), e, "Input 0 to 100", "",
						[this, gbVol](EmuApp &app, auto val)
						{
							soundSetVolume(gGba, val / 100.f, gbVol);
							size_t idx = gbVol ? 1 : 0;
							volumeLevel[idx].setSelected((MenuItem::Id)val, *this);
							dismissPrevious();
							return true;
						});
					return false;
				}, MenuItem::DEFAULT_ID
			}
		};
	}

	std::array<VolumeChoiceItemArr, 2> volumeLevelItem
	{
		volumeLevelChoiceItems(false),
		volumeLevelChoiceItems(true),
	};

	MultiChoiceMenuItem volumeLevelMenuItem(bool gbVol)
	{
		return
		{
			gbVol ? "GB APU Volume" : "PCM Volume", &defaultFace(),
			{
				.onSetDisplayString = [this, gbVol](auto idx, Gfx::Text &t)
				{
					t.resetString(fmt::format("{}%", soundVolumeAsInt(gGba, gbVol)));
					return true;
				}
			},
			(MenuItem::Id)soundVolumeAsInt(gGba, gbVol),
			volumeLevelItem[gbVol ? 1 : 0]
		};
	}

	std::array<MultiChoiceMenuItem, 2> volumeLevel
	{
		volumeLevelMenuItem(false),
		volumeLevelMenuItem(true),
	};

	BoolMenuItem channelEnableItem(auto &&name, int mask)
	{
		return
		{
			IG_forward(name), &defaultFace(),
			bool(soundGetEnable(gGba) & mask),
			[this, mask](BoolMenuItem &item)
			{
				soundSetEnable(gGba, setOrClearBits(soundGetEnable(gGba), mask, item.flipBoolValue(*this)));
			}
		};
	}

	std::array<BoolMenuItem, 6> channelEnable
	{
		channelEnableItem("PCM #1", 0x100),
		channelEnableItem("PCM #2", 0x200),
		channelEnableItem("Pulse #1", 0x1),
		channelEnableItem("Pulse #2", 0x2),
		channelEnableItem("Wave", 0x4),
		channelEnableItem("Noise", 0x8),
	};

	std::array<TextMenuItem, 2> filteringLevelItem
	{
		TextMenuItem
		{
			"Default", &defaultFace(),
			[this]() { soundSetFiltering(gGba, .5f); },
			50
		},
		TextMenuItem
		{
			"Custom Value", &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 100>(attachParams(), e, "Input 0 to 100", "",
					[this](EmuApp &app, auto val)
					{
						soundSetFiltering(gGba, val / 100.f);
						filteringLevel.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		}
	};

	MultiChoiceMenuItem filteringLevel
	{
		"Filtering Level", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{}%", soundFilteringAsInt(gGba)));
				return true;
			}
		},
		(MenuItem::Id)soundFilteringAsInt(gGba),
		filteringLevelItem
	};

	BoolMenuItem filtering
	{
		"Filtering", &defaultFace(),
		soundGetInterpolation(gGba),
		[this](BoolMenuItem &item)
		{
			soundSetInterpolation(gGba, item.flipBoolValue(*this));
		}
	};

public:
	CustomAudioOptionView(ViewAttachParams attach): AudioOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&filtering);
		item.emplace_back(&filteringLevel);
		item.emplace_back(&mixer);
		item.emplace_back(&volumeLevel[0]);
		item.emplace_back(&channelEnable[0]);
		item.emplace_back(&channelEnable[1]);
		item.emplace_back(&volumeLevel[1]);
		item.emplace_back(&channelEnable[2]);
		item.emplace_back(&channelEnable[3]);
		item.emplace_back(&channelEnable[4]);
		item.emplace_back(&channelEnable[5]);
	}
};

class CustomSystemOptionView : public SystemOptionView, public MainAppHelper<CustomSystemOptionView>
{
	using MainAppHelper<CustomSystemOptionView>::system;
	using MainAppHelper<CustomSystemOptionView>::app;

	#ifdef IG_CONFIG_SENSORS
	TextMenuItem lightSensorScaleItem[5]
	{
		{"Darkness",      &defaultFace(), setLightSensorScaleDel(), 0},
		{"Indoor Light",  &defaultFace(), setLightSensorScaleDel(), 100},
		{"Overcast Day",  &defaultFace(), setLightSensorScaleDel(), 1000},
		{"Sunny Day",     &defaultFace(), setLightSensorScaleDel(), 10000},
		{"Custom Value",  &defaultFace(),
			[this](Input::Event e)
			{
				app().pushAndShowNewCollectValueRangeInputView<int, 0, 50000>(attachParams(), e, "Input 0 to 50000", "",
					[this](EmuApp &app, auto val)
					{
						system().lightSensorScaleLux = val;
						lightSensorScale.setSelected((MenuItem::Id)val, *this);
						dismissPrevious();
						return true;
					});
				return false;
			}, MenuItem::DEFAULT_ID
		}
	};

	MultiChoiceMenuItem lightSensorScale
	{
		"Light Sensor Scale", &defaultFace(),
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				t.resetString(fmt::format("{} lux", (int)system().lightSensorScaleLux));
				return true;
			}
		},
		(MenuItem::Id)system().lightSensorScaleLux,
		lightSensorScaleItem
	};

	TextMenuItem::SelectDelegate setLightSensorScaleDel()
	{
		return [this](TextMenuItem &item) { system().lightSensorScaleLux = item.id(); };
	}
	#endif

public:
	CustomSystemOptionView(ViewAttachParams attach): SystemOptionView{attach, true}
	{
		loadStockItems();
		#ifdef IG_CONFIG_SENSORS
		item.emplace_back(&lightSensorScale);
		#endif
	}
};

class CustomFilePathOptionView : public FilePathOptionView, public MainAppHelper<CustomFilePathOptionView>
{
	using MainAppHelper<CustomFilePathOptionView>::system;

	TextMenuItem cheatsPath
	{
		cheatsMenuName(appContext(), system().cheatsDir), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Cheats", system().userPath(system().cheatsDir),
				[this](CStringView path)
				{
					logMsg("set cheats path:%s", path.data());
					system().cheatsDir = path;
					cheatsPath.compile(cheatsMenuName(appContext(), path), renderer());
				}), e);
		}
	};

	TextMenuItem patchesPath
	{
		patchesMenuName(appContext(), system().patchesDir), &defaultFace(),
		[this](const Input::Event &e)
		{
			pushAndShow(makeViewWithName<UserPathSelectView>("Patches", system().userPath(system().patchesDir),
				[this](CStringView path)
				{
					logMsg("set patches path:%s", path.data());
					system().patchesDir = path;
					patchesPath.compile(patchesMenuName(appContext(), path), renderer());
				}), e);
		}
	};

public:
	CustomFilePathOptionView(ViewAttachParams attach): FilePathOptionView{attach, true}
	{
		loadStockItems();
		item.emplace_back(&cheatsPath);
		item.emplace_back(&patchesPath);
	}
};

std::unique_ptr<View> EmuApp::makeCustomView(ViewAttachParams attach, ViewID id)
{
	switch(id)
	{
		case ViewID::SYSTEM_ACTIONS: return std::make_unique<CustomSystemActionsView>(attach);
		case ViewID::SYSTEM_OPTIONS: return std::make_unique<CustomSystemOptionView>(attach);
		case ViewID::AUDIO_OPTIONS: return std::make_unique<CustomAudioOptionView>(attach);
		case ViewID::FILE_PATH_OPTIONS: return std::make_unique<CustomFilePathOptionView>(attach);
		case ViewID::EDIT_CHEATS: return std::make_unique<EmuEditCheatListView>(attach);
		case ViewID::LIST_CHEATS: return std::make_unique<EmuCheatsView>(attach);
		default: return nullptr;
	}
}

}
