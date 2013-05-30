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

#include <gui/FSPicker/FSPicker.hh>
#include <EmuSystem.hh>

bool isMenuDismissKey(const Input::Event &e);
void startGameFromMenu();

class EmuFilePicker : public FSPicker
{
public:
	constexpr EmuFilePicker() { }
	static FsDirFilterFunc defaultFsFilter;
	static FsDirFilterFunc defaultBenchmarkFsFilter;

	void init(bool highlightFirst, FsDirFilterFunc filter = defaultFsFilter, bool singleDir = 0);
	void initForBenchmark(bool highlightFirst, bool singleDir = 0);

	void inputEvent(const Input::Event &e)
	{
		if(e.state == Input::PUSHED)
		{
			if(e.isDefaultCancelButton())
			{
				onCloseD(e);
				return;
			}

			if(isMenuDismissKey(e))
			{
				if(EmuSystem::gameIsRunning())
				{
					removeModalView();
					startGameFromMenu();
					return;
				}
			}
		}

		FSPicker::inputEvent(e);
	}
};

class GameFilePicker
{
public:
	static void onSelectFile(const char* name, const Input::Event &e);
	static void onClose(const Input::Event &e);
};
