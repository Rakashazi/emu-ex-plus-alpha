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
#include <emuframework/AppKeyCode.hh>
#include <emuframework/EmuOptions.hh>
#include <emuframework/viewUtils.hh>
#include "InputOverridesView.hh"
#include "ProfileSelectView.hh"
#include "../InputDeviceData.hh"
#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/TextTableView.hh>
#include <imagine/gui/AlertView.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/gfx/RendererCommands.hh>
#include <imagine/util/format.hh>
#include <imagine/logger/logger.h>

namespace EmuEx
{

constexpr SystemLogger log{"InputOverridesView"};
constexpr auto confirmDeleteDeviceSettingsStr = "Delete device settings from the configuration file?";

InputOverridesView::InputOverridesView(ViewAttachParams attach,
	InputManager& inputManager_):
	TableView{"Input Overrides (Per Content)", attach, items},
	inputManager{inputManager_},
	deleteDeviceConfig
	{
		"Delete Saved Device Settings", attach,
		[this](TextMenuItem &item, View &, const Input::Event &e)
		{
			auto &savedSessionDevConfigs = inputManager.savedSessionDevConfigs;
			if(!savedSessionDevConfigs.size())
			{
				app().postMessage("No saved device settings");
				return;
			}
			auto multiChoiceView = makeViewWithName<TextTableView>(item, savedSessionDevConfigs.size());
			for(auto &ePtr : savedSessionDevConfigs)
			{
				multiChoiceView->appendItem(Input::Device::makeDisplayName(ePtr->name, ePtr->enumId),
					[this, &deleteDeviceConfig = *ePtr](const Input::Event &e)
					{
						pushAndShowModal(makeView<YesNoAlertView>(confirmDeleteDeviceSettingsStr,
							YesNoAlertView::Delegates
							{
								.onYes = [this, &deleteDeviceConfig]
								{
									inputManager.deleteDeviceSavedConfig(appContext(), deleteDeviceConfig);
									system().sessionOptionSet();
									dismissPrevious();
								}
							}), e);
					});
			}
			pushAndShow(std::move(multiChoiceView), e);
		}
	},
	deviceListHeading{"Individual Device Settings", attach}
{
	loadItems();
	inputManager.onUpdateDevices = [this]()
	{
		popTo(*this);
		auto selectedCell = selected;
		loadItems();
		highlightCell(selectedCell);
		place();
		show();
	};
}

InputOverridesView::~InputOverridesView()
{
	inputManager.onUpdateDevices = {};
}

void InputOverridesView::loadItems()
{
	auto ctx = appContext();
	items.clear();
	items.reserve(16);
	items.emplace_back(&deleteDeviceConfig);
	items.emplace_back(&deviceListHeading);
	inputDevNames.clear();
	inputDevNames.reserve(ctx.inputDevices().size());
	for(auto &devPtr : ctx.inputDevices())
	{
		auto &devItem = inputDevNames.emplace_back(inputDevData(*devPtr).displayName, attachParams(),
			[this, &dev = *devPtr](const Input::Event &e)
			{
				pushAndShowDeviceView(dev, e);
			});
		if(devPtr->hasKeys() && !devPtr->isPowerButton())
		{
			items.emplace_back(&devItem);
		}
		else
		{
			log.info("not adding device:{} to list", devPtr->name());
		}
	}
}

void InputOverridesView::pushAndShowDeviceView(const Input::Device &dev, const Input::Event &e)
{
	pushAndShow(makeViewWithName<InputOverridesDeviceView>(inputDevData(dev).displayName, *this, dev, inputManager), e);
}

constexpr std::string_view playerAsString(int p)
{
	assert(p != playerIndexUnset);
	if(p == playerIndexMulti)
		return "Multiple";
	return playerNumStrings[p];
}

InputOverridesDeviceView::InputOverridesDeviceView(UTF16String name, ViewAttachParams attach,
	InputOverridesView& rootIMView_, const Input::Device& dev, InputManager& inputManager_):
	TableView{std::move(name), attach, items},
	inputManager{inputManager_},
	rootIMView{rootIMView_},
	playerItems
	{
		[&]
		{
			DynArray<TextMenuItem> items{EmuSystem::maxPlayers + 2uz};
			items[0] = {"Default", attach, {.id = playerIndexUnset}};
			items[1] = {"Multiple", attach, {.id = playerIndexMulti}};
			for(auto i : iotaCount(EmuSystem::maxPlayers))
			{
				items[i + 2] = {playerNumStrings[i], attach, {.id = i}};
			}
			return items;
		}()
	},
	player
	{
		"Player", attach,
		MenuId{inputDevData(dev).devConf.savedSessionPlayer()},
		playerItems,
		{
			.onSetDisplayString = [this](auto idx, Gfx::Text &t)
			{
				if(idx == 0)
				{
					t.resetString(playerAsString(devConf.sessionConfig(inputManager).player));
					return true;
				}
				else
					return false;
			},
			.defaultItemOnSelect = [this](TextMenuItem& item)
			{
				auto playerVal = item.id;
				devConf.setSavedSessionPlayer(inputManager, playerVal);
				system().sessionOptionSet();
			}
		},
	},
	loadProfile
	{
		u"", attach,
		[this](const Input::Event& e)
		{
			auto profileSelectMenu = makeView<ProfileSelectView>(devConf.device().map(),
				devConf.sessionKeyConfName(), app(), ProfileSelectViewDesc{.hasDefaultItem = true});
			profileSelectMenu->onProfileChange = [this](std::string_view profile)
			{
				log.info("set session key profile:{}", profile);
				devConf.setSessionKeyConfName(inputManager, profile);
				system().sessionOptionSet();
				onShow();
			};
			pushAndShow(std::move(profileSelectMenu), e);
		}
	},
	devConf{inputDevData(dev).devConf}
{
	loadProfile.setName(std::format("Profile: {}", devConf.sessionConfig(inputManager).keyConfName));
	loadItems();
}

void InputOverridesDeviceView::loadItems()
{
	items.clear();
	if(EmuSystem::maxPlayers > 1)
	{
		items.emplace_back(&player);
	}
	items.emplace_back(&loadProfile);
}

void InputOverridesDeviceView::onShow()
{
	TableView::onShow();
	loadProfile.compile(std::format("Profile: {}", devConf.sessionConfig(inputManager).keyConfName));
}

}
