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

#include "MDFN.hh"
#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/util/string.h>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

using namespace Mednafen;

extern FS::PathString sysCardPath;
extern Byte1Option optionArcadeCard;

namespace MDFN_IEN_PCE_FAST
{
// dummy HES functions
int PCE_HESLoad(const uint8 *buf, uint32 size) { return 0; };
void HES_Draw(MDFN_Surface *surface, MDFN_Rect *DisplayRect, int16 *SoundBuf, int32 SoundBufSize) { }
void HES_Close(void) { }
void HES_Reset(void) { }
uint8 ReadIBP(unsigned int A) { return 0; }
};

#define PCE_MODULE "pce_fast"

namespace Mednafen
{

NativeVFS NVFS;
MDFNGI *MDFNGameInfo = &EmulatedPCE_Fast;

uint64 MDFN_GetSettingUI(const char *name)
{
	std::string_view nameV{name};
	if(PCE_MODULE".ocmultiplier" == nameV)
		return 1;
	if(PCE_MODULE".cdspeed" == nameV)
		return 2;
	if(PCE_MODULE".cdpsgvolume" == nameV)
		return 100;
	if(PCE_MODULE".cddavolume" == nameV)
		return 100;
	if(PCE_MODULE".adpcmvolume" == nameV)
		return 100;
	if(PCE_MODULE".slstart" == nameV)
		return 12;
	if(PCE_MODULE".slend" == nameV)
		return 235;
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
	std::string_view nameV{name};
	if(PCE_MODULE".mouse_sensitivity" == nameV)
		return 0.50;
	bug_unreachable("unhandled settingF %s", name);
	return 0;
}

bool MDFN_GetSettingB(const char *name)
{
	std::string_view nameV{name};
	if("cheats" == nameV)
		return 0;
	if("filesys.disablesavegz" == nameV)
		return 0;
	if(PCE_MODULE".arcadecard" == nameV)
		return optionArcadeCard;
	if(PCE_MODULE".forcesgx" == nameV)
		return 0;
	if(PCE_MODULE".nospritelimit" == nameV)
		return 0;
	if(PCE_MODULE".forcemono" == nameV)
		return 0;
	if(PCE_MODULE".disable_softreset" == nameV)
		return 0;
	if(PCE_MODULE".adpcmlp" == nameV)
		return 0;
	if(PCE_MODULE".correct_aspect" == nameV)
		return 1;
	if("cdrom.lec_eval" == nameV)
		return 1;
	if("filesys.untrusted_fip_check" == nameV)
		return 0;
	bug_unreachable("unhandled settingB %s", name);
	return 0;
}

std::string MDFN_GetSettingS(const char *name)
{
	std::string_view nameV{name};
	if(PCE_MODULE".cdbios" == nameV)
	{
		return {};
	}
	bug_unreachable("unhandled settingS %s", name);
	return 0;
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
			FS::FileString ext{md5_context::asciistr(MDFNGameInfo->MD5, 0)};
			ext += '/';
			ext += cd1;
			auto path = EmuSystem::contentSaveFilePath(ext);
			if(type == MDFNMKF_SAV) logMsg("created save path %s", path.c_str());
			return std::string{path};
		}
		case MDFNMKF_FIRMWARE:
		{
			// pce-specific
			logMsg("getting firmware path %s", sysCardPath.data());
			return std::string(sysCardPath.data());
		}
		default:
			bug_unreachable("type == %d", type);
			return 0;
	}
}

void MDFN_DoSimpleCommand(int cmd)
{
	emuSys->DoSimpleCommand(cmd);
}

}
