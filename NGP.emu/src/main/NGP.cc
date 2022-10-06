/*  This file is part of PCE.emu.

	PCE.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	PCE.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with PCE.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/fs/FS.hh>
#include <imagine/util/string.h>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/logger/logger.h>
#include <emuframework/Option.hh>
#include <emuframework/EmuApp.hh>
#include "MainSystem.hh"
#include <mednafen/hash/md5.h>
#include <mednafen/general.h>

#define EMU_MODULE "ngp"

namespace Mednafen
{

MDFNGI *MDFNGameInfo;

uint64 MDFN_GetSettingUI(const char *name)
{
	bug_unreachable("unhandled settingUI %s", name);
	return 0;
}

int64 MDFN_GetSettingI(const char *name)
{
	std::string_view nameV{name};
	if("filesys.state_comp_level" == nameV)
		return 6;
	bug_unreachable("unhandled settingI %s", name);
	return 0;
}

double MDFN_GetSettingF(const char *name)
{
	bug_unreachable("unhandled settingF %s", name);
	return 0;
}

bool MDFN_GetSettingB(const char *name)
{
	std::string_view nameV{name};
	if("cheats" == nameV)
		return 0;
	if(EMU_MODULE".language" == nameV)
		return static_cast<EmuEx::NgpSystem&>(EmuEx::gSystem()).optionNGPLanguage;
	if("filesys.untrusted_fip_check" == nameV)
		return 0;
	bug_unreachable("unhandled settingB %s", name);
	return 0;
}

std::string MDFN_GetSettingS(const char *name)
{
	bug_unreachable("unhandled settingS %s", name);
	return {};
}

std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
	switch(type)
	{
		case MDFNMKF_STATE:
		case MDFNMKF_SAV:
		case MDFNMKF_SAVBACK:
		{
			assert(cd1);
			IG::FileString ext{'.'};
			ext += md5_context::asciistr(MDFNGameInfo->MD5, 0);
			ext += '.';
			ext += cd1;
			auto path = EmuEx::gApp().contentSaveFilePath(ext);
			if(type == MDFNMKF_SAV) logMsg("save path:%s", path.c_str());
			return std::string{path};
		}
		default:
			bug_unreachable("type == %d", type);
			return {};
	}
}

void MDFN_DoSimpleCommand(int cmd)
{
	MDFNGameInfo->DoSimpleCommand(cmd);
}

}
