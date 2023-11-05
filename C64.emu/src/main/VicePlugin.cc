/*  This file is part of C64.emu.

	C64.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	C64.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with C64.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <cstring>
#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/util/utility.h>
#include <imagine/util/format.hh>
#include <emuframework/EmuApp.hh>
#include "VicePlugin.hh"

extern "C"
{
	#include "c64model.h"
	#include "c64dtvmodel.h"
	#include "c128model.h"
	#include "cbm2model.h"
	#include "petmodel.h"
	#include "plus4model.h"
	#include "vic20model.h"
	#include "cartridge.h"
	#include "resources.h"
}

constexpr const char *systemNameStr[]
{
	"C64",
	"C64 (Cycle Accurate)",
	"C64DTV",
	"C128",
	"C64 SuperCPU",
	"CBM-II 6x0",
	"CBM-II 5x0",
	"PET",
	"Plus/4",
	"VIC-20",
};

constexpr const char *libName[]
{
	"libc64.so",
	"libc64sc.so",
	"libc64dtv.so",
	"libc128.so",
	"libscpu64.so",
	"libcbm2.so",
	"libcbm5x0.so",
	"libpet.so",
	"libplus4.so",
	"libvic.so",
};

constexpr std::string_view c64ModelStr[]
{
	"C64 PAL",
	"C64C PAL",
	"C64 old PAL",
	"C64 NTSC",
	"C64C NTSC",
	"C64 old NTSC",
	"Drean",
	"C64 SX PAL",
	"C64 SX NTSC",
	"Japanese",
	"C64 GS",
	"PET64 PAL",
	"PET64 NTSC",
	"MAX Machine",
};

constexpr std::string_view dtvModelStr[]
{
	"DTV v2 PAL",
	"DTV v2 NTSC",
	"DTV v3 PAL",
	"DTV v3 NTSC",
	"Hummer NTSC",
};

constexpr std::string_view c128ModelStr[]
{
	"C128 PAL",
	"C128D PAL",
	"C128DCR PAL",
	"C128 NTSC",
	"C128D NTSC",
	"C128DCR NTSC",
};

constexpr std::string_view superCPUModelStr[]
{
	"C64 PAL",
	"C64C PAL",
	"C64 old PAL",
	"C64 NTSC",
	"C64C NTSC",
	"C64 old NTSC",
	"Drean",
	"C64 SX PAL",
	"C64 SX NTSC",
	"Japanese",
	"C64 GS",
};

constexpr std::string_view cbm2ModelStr[]
{
	"CBM 610 PAL",
	"CBM 610 NTSC",
	"CBM 620 PAL",
	"CBM 620 NTSC",
	"CBM 620+ (1M) PAL",
	"CBM 620+ (1M) NTSC",
	"CBM 710 NTSC",
	"CBM 720 NTSC",
	"CBM 720+ (1M) NTSC",
};

constexpr std::string_view cbm5x0ModelStr[]
{
	"CBM 510 PAL",
	"CBM 510 NTSC",
};

constexpr std::string_view petModelStr[]
{
	"PET 2001-8N",
	"PET 3008",
	"PET 3016",
	"PET 3032",
	"PET 3032B",
	"PET 4016",
	"PET 4032",
	"PET 4032B",
	"PET 8032",
	"PET 8096",
	"PET 8296",
	"SuperPET",
};

constexpr std::string_view plus4ModelStr[]
{
	"C16/116 PAL",
	"C16/116 NTSC",
	"Plus4 PAL",
	"Plus4 NTSC",
	"V364 NTSC",
	"C232 NTSC",
};

constexpr std::string_view vic20ModelStr[]
{
	"VIC-20 PAL",
	"VIC-20 NTSC",
	"VIC-21",
	"VIC-1001",
};

struct PluginConfig
{
	std::span<const std::string_view> modelNames;
	std::string_view configName{};
	const char *getModelFuncName{};
	const char *setModelFuncName{};
	const char *borderModeStr{};
	int8_t defaultModelId{};
	int8_t modelIdBase{};
};

static IG::PathString makePluginLibPath(const char *libName, const char *libBasePath)
{
	if(libBasePath && strlen(libBasePath))
		return IG::pathString(libBasePath, libName);
	else
		return libName;
}

template<class T>
static void loadSymbolCheck(T &symPtr, IG::SharedLibraryRef lib, const char *name)
{
	if(!IG::loadSymbol(symPtr, lib, name))
		logErr("symbol:%s missing from plugin", name);
}

void VicePlugin::init()
{
	assert(libHandle);
	int (*vice_init)();
	loadSymbolCheck(vice_init, libHandle, "vice_init");
	vice_init();
}

void VicePlugin::deinit()
{
	if(libHandle)
	{
		// TODO: doesn't fully clean up all VICE heap allocations, don't use for now
		logMsg("doing machine_shutdown()");
		void (*machine_shutdown)();
		loadSymbolCheck(machine_shutdown, libHandle, "machine_shutdown");
		machine_shutdown();
		IG::closeSharedLibrary(libHandle);
		libHandle = {};
	}
}

bool VicePlugin::hasSystemLib(ViceSystem system, const char *libBasePath)
{
	if(system < ViceSystem::C64 || system > ViceSystem::VIC20)
		return false;
	return IG::FS::exists(makePluginLibPath(libName[std::to_underlying(system)], libBasePath));
}

const char *VicePlugin::systemName(ViceSystem system)
{
	if(system < ViceSystem::C64 || system > ViceSystem::VIC20)
		return "";
	return systemNameStr[std::to_underlying(system)];
}

int VicePlugin::model_get() const
{
	if(model_get_)
		return model_get_();
	return 0;
}

void VicePlugin::model_set(int model)
{
	if(model_set_)
		return model_set_(model);
}

int VicePlugin::resources_get_string(const char *name, const char **value_return) const
{
	if(resources_get_string_)
		return resources_get_string_(name, value_return);
	return -1;
}

int VicePlugin::resources_set_string(const char *name, const char *value)
{
	if(resources_set_string_)
		return resources_set_string_(name, value);
	return -1;
}

int VicePlugin::resources_get_int(const char *name, int *value_return) const
{
	if(resources_get_int_)
		return resources_get_int_(name, value_return);
	return -1;
}

int VicePlugin::resources_set_int(const char *name, int value)
{
	if(resources_set_int_)
		return resources_set_int_(name, value);
	return -1;
}

int VicePlugin::resources_get_default_value(const char *name, void *value_return) const
{
	if(resources_get_default_value_)
		return resources_get_default_value_(name, value_return);
	return -1;
}

int VicePlugin::machine_write_snapshot(const char *name, int save_roms, int save_disks, int even_mode) const
{
	if(machine_write_snapshot_)
		return machine_write_snapshot_(name, save_roms, save_disks, even_mode);
	return -1;
}

int VicePlugin::machine_read_snapshot(const char *name, int event_mode) const
{
	if(machine_read_snapshot_)
		return machine_read_snapshot_(name, event_mode);
	return -1;
}

void VicePlugin::machine_set_restore_key(int v)
{
	if(machine_set_restore_key_)
		machine_set_restore_key_(v);
}

void VicePlugin::machine_trigger_reset(const unsigned int mode)
{
	if(machine_trigger_reset_)
		machine_trigger_reset_(mode);
}

struct drive_type_info_s *VicePlugin::machine_drive_get_type_info_list()
{
	return machine_drive_get_type_info_list_();
}

void VicePlugin::interrupt_maincpu_trigger_trap(void trap_func(uint16_t, void *data), void *data) const
{
	if(interrupt_maincpu_trigger_trap_)
		interrupt_maincpu_trigger_trap_(trap_func, data);
}

int VicePlugin::init_main()
{
	if(init_main_)
		return init_main_();
	return -1;
}

void VicePlugin::maincpu_mainloop()
{
	assert(maincpu_mainloop_);
	maincpu_mainloop_();
}

int VicePlugin::autostart_autodetect(const char *file_name, const char *program_name,
	unsigned int program_number, unsigned int runmode)
{
	if(autostart_autodetect_)
		return autostart_autodetect_(file_name, program_name, program_number, runmode);
	return -1;
}

int VicePlugin::cart_getid_slotmain()
{
	if(cart_getid_slotmain_)
		return cart_getid_slotmain_();
	return -1;
}

const char *VicePlugin::cartridge_get_file_name(int type)
{
	if(cartridge_get_file_name_)
		return cartridge_get_file_name_(type);
	if(!libHandle)
		return "";
	const char *filename{};
	switch(type)
	{
		case CARTRIDGE_CBM2_8KB_1000:
			resources_get_string("Cart1Name", &filename);
			break;
		case CARTRIDGE_CBM2_8KB_2000:
			resources_get_string("Cart2Name", &filename);
			break;
		case CARTRIDGE_CBM2_16KB_4000:
			resources_get_string("Cart4Name", &filename);
			break;
		case CARTRIDGE_CBM2_16KB_6000:
			resources_get_string("Cart6Name", &filename);
			break;
		default:
			logErr("cartridge_get_file_name: unsupported type (%04x)", type);
	}
	return filename;
}

int VicePlugin::cartridge_attach_image(int type, const char *filename)
{
	if(cartridge_attach_image_)
		return cartridge_attach_image_(type, filename);
	return -1;
}

void VicePlugin::cartridge_detach_image(int type)
{
	if(cartridge_detach_image_)
		cartridge_detach_image_(type);
}

int VicePlugin::tape_image_attach(unsigned int unit, const char *name)
{
	if(tape_image_attach_)
		return tape_image_attach_(unit, name);
	return -1;
}

int VicePlugin::tape_image_detach(unsigned int unit)
{
	if(tape_image_detach_)
		return tape_image_detach_(unit);
	return -1;
}

const char *VicePlugin::tape_get_file_name(int port)
{
	if(tape_get_file_name_)
		return tape_get_file_name_(port);
	return "";
}

void VicePlugin::datasette_control(int port, int command)
{
	if(datasette_control_)
		datasette_control_(port, command);
}

int VicePlugin::file_system_attach_disk(unsigned int unit, unsigned int drive, const char *filename)
{
	if(file_system_attach_disk_)
		return file_system_attach_disk_(unit, drive, filename);
	return -1;
}

void VicePlugin::file_system_detach_disk(unsigned int unit, unsigned int drive)
{
	if(file_system_detach_disk_)
		file_system_detach_disk_(unit, drive);
}

const char *VicePlugin::file_system_get_disk_name(unsigned int unit, unsigned int drive)
{
	if(file_system_get_disk_name_)
		return file_system_get_disk_name_(unit, drive);
	return "";
}

int VicePlugin::drive_check_type(unsigned int drive_type, unsigned int dnr)
{
	if(drive_check_type_)
		return drive_check_type_(drive_type, dnr);
	return 0;
}

int VicePlugin::sound_register_device(const sound_device_t *pdevice)
{
	if(sound_register_device_)
		return sound_register_device_(pdevice);
	return -1;
}

void VicePlugin::video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg,
  int width, int height, int xs, int ys,
  int xt, int yt, int pitcht)
{
	if(video_canvas_render_)
	{
		video_canvas_render_(canvas, trg, width, height, xs, ys,
			xt, yt, pitcht);
	}
}

void VicePlugin::video_render_setphysicalcolor(video_render_config_t *config,
	int index, uint32_t color, int depth)
{
	if(video_render_setphysicalcolor_)
	{
		video_render_setphysicalcolor_(config, index, color, depth);
	}
}

void VicePlugin::video_render_setrawrgb(video_render_color_tables_t *color_tab, unsigned int index,
	uint32_t r, uint32_t g, uint32_t b)
{
	if(video_render_setrawrgb_)
	{
		video_render_setrawrgb_(color_tab, index, r, g, b);
	}
}

void VicePlugin::video_render_initraw(struct video_render_config_s *videoconfig)
{
	if(video_render_initraw_)
	{
		video_render_initraw_(videoconfig);
	}
}

int VicePlugin::vdrive_internal_create_format_disk_image(const char *filename, const char *diskname, unsigned int type)
{
	if(vdrive_internal_create_format_disk_image_)
	{
		return vdrive_internal_create_format_disk_image_(filename, diskname, type);
	}
	return -1;
}

int VicePlugin::cbmimage_create_image(const char *name, unsigned int type)
{
	return cbmimage_create_image_(name, type);
}

void VicePlugin::keyboard_key_clear(void)
{
	keyboard_key_clear_();
}

void VicePlugin::vsync_set_warp_mode(int val)
{
	vsync_set_warp_mode_(val);
}

VicePlugin commonVicePlugin(void *lib, ViceSystem system)
{
	VicePlugin plugin{};
	loadSymbolCheck(plugin.joystick_value, lib, "joystick_value");
	loadSymbolCheck(plugin.warp_mode_enabled, lib, "warp_mode_enabled");
	loadSymbolCheck(plugin.resources_get_string_, lib, "resources_get_string");
	loadSymbolCheck(plugin.resources_set_string_, lib, "resources_set_string");
	loadSymbolCheck(plugin.resources_get_int_, lib, "resources_get_int");
	loadSymbolCheck(plugin.resources_set_int_, lib, "resources_set_int");
	loadSymbolCheck(plugin.resources_get_default_value_, lib, "resources_get_default_value");
	loadSymbolCheck(plugin.machine_write_snapshot_, lib, "machine_write_snapshot");
	loadSymbolCheck(plugin.machine_read_snapshot_, lib, "machine_read_snapshot");
	loadSymbolCheck(plugin.machine_set_restore_key_, lib, "machine_set_restore_key");
	loadSymbolCheck(plugin.machine_trigger_reset_, lib, "machine_trigger_reset");
	loadSymbolCheck(plugin.machine_drive_get_type_info_list_, lib, "machine_drive_get_type_info_list");
	loadSymbolCheck(plugin.interrupt_maincpu_trigger_trap_, lib, "interrupt_maincpu_trigger_trap");
	loadSymbolCheck(plugin.init_main_, lib, "init_main");
	assert(plugin.init_main_);
	loadSymbolCheck(plugin.maincpu_mainloop_, lib, "maincpu_mainloop");
	if(system == ViceSystem::PET)
	{
		loadSymbolCheck(plugin.autostart_autodetect_, lib, "autostart_autodetect");
		// no cart system
	}
	else if(system == ViceSystem::PLUS4)
	{
		loadSymbolCheck(plugin.autostart_autodetect_, lib, "autostart_autodetect");
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return 0;
			};
		plugin.cartridge_get_file_name_ =
			[](int type)
			{
				return "";
			};
		loadSymbolCheck(plugin.cartridge_attach_image_, lib, "cartridge_attach_image");
		loadSymbolCheck(plugin.cartridge_detach_image_, lib, "cartridge_detach_image");
	}
	else if(system == ViceSystem::CBM2 || system == ViceSystem::CBM5X0)
	{
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return CARTRIDGE_CBM2_8KB_1000;
			};
		loadSymbolCheck(plugin.cartridge_attach_image_, lib, "cartridge_attach_image");
		loadSymbolCheck(plugin.cartridge_detach_image_, lib, "cartridge_detach_image");
	}
	else if(system == ViceSystem::VIC20)
	{
		loadSymbolCheck(plugin.autostart_autodetect_, lib, "autostart_autodetect");
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return CARTRIDGE_VIC20_DETECT;
			};
		loadSymbolCheck(plugin.cartridge_get_file_name_, lib, "cartridge_get_file_name");
		loadSymbolCheck(plugin.cartridge_attach_image_, lib, "cartridge_attach_image");
		loadSymbolCheck(plugin.cartridge_detach_image_, lib, "cartridge_detach_image");
	}
	else
	{
		loadSymbolCheck(plugin.autostart_autodetect_, lib, "autostart_autodetect");
		if(system != ViceSystem::C64DTV)
			loadSymbolCheck(plugin.cart_getid_slotmain_, lib, "cart_getid_slotmain");
		loadSymbolCheck(plugin.cartridge_get_file_name_, lib, "cartridge_get_file_name");
		loadSymbolCheck(plugin.cartridge_attach_image_, lib, "cartridge_attach_image");
		loadSymbolCheck(plugin.cartridge_detach_image_, lib, "cartridge_detach_image");
	}
	loadSymbolCheck(plugin.tape_image_attach_, lib, "tape_image_attach");
	loadSymbolCheck(plugin.tape_image_detach_, lib, "tape_image_detach");
	loadSymbolCheck(plugin.tape_get_file_name_, lib, "tape_get_file_name");
	loadSymbolCheck(plugin.datasette_control_, lib, "datasette_control");
	loadSymbolCheck(plugin.file_system_attach_disk_, lib, "file_system_attach_disk");
	loadSymbolCheck(plugin.file_system_detach_disk_, lib, "file_system_detach_disk");
	loadSymbolCheck(plugin.file_system_get_disk_name_, lib, "file_system_get_disk_name");
	loadSymbolCheck(plugin.drive_check_type_, lib, "drive_check_type");
	loadSymbolCheck(plugin.sound_register_device_, lib, "sound_register_device");
	loadSymbolCheck(plugin.video_canvas_render_, lib, "video_canvas_render");
	loadSymbolCheck(plugin.video_render_setphysicalcolor_, lib, "video_render_setphysicalcolor");
	loadSymbolCheck(plugin.video_render_setrawrgb_, lib, "video_render_setrawrgb");
	loadSymbolCheck(plugin.video_render_initraw_, lib, "video_render_initraw");
	loadSymbolCheck(plugin.vdrive_internal_create_format_disk_image_, lib, "vdrive_internal_create_format_disk_image");
	loadSymbolCheck(plugin.cbmimage_create_image_, lib, "cbmimage_create_image");
	loadSymbolCheck(plugin.keyboard_key_pressed_direct_, lib, "keyboard_key_pressed_direct");
	loadSymbolCheck(plugin.keyboard_key_clear_, lib, "keyboard_key_clear");
	loadSymbolCheck(plugin.vsync_set_warp_mode_, lib, "vsync_set_warp_mode");
	return plugin;
}

VicePlugin loadVicePlugin(ViceSystem system, const char *libBasePath)
{
	static constexpr PluginConfig pluginConf[]
	{
		// C64
		{
			c64ModelStr,
			"c64.config",
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode",
			C64MODEL_C64_NTSC
		},
		// C64 (accurate)
		{
			c64ModelStr,
			"c64sc.config",
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode",
			C64MODEL_C64_NTSC
		},
		// DTV
		{
			dtvModelStr,
			"c64dtv.config",
			"dtvmodel_get",
			"dtvmodel_set",
			"VICIIBorderMode",
			DTVMODEL_V3_NTSC
		},
		// C128
		{
			c128ModelStr,
			"c128.config",
			"c128model_get",
			"c128model_set",
			"VICIIBorderMode",
			C128MODEL_C128_NTSC
		},
		// C64 Super CPU
		{
			superCPUModelStr,
			"scpu64.config",
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode",
			C64MODEL_C64_NTSC
		},
		// CBM-II 6x0
		{
			cbm2ModelStr,
			"cbm2.config",
			"cbm2model_get",
			"cbm2model_set",
			{},
			CBM2MODEL_610_NTSC,
			CBM2MODEL_610_PAL
		},
		// CBM-II 5x0
		{
			cbm5x0ModelStr,
			"cbm5x0.config",
			"cbm2model_get",
			"cbm2model_set",
			"VICIIBorderMode",
			CBM2MODEL_510_NTSC,
			CBM2MODEL_510_PAL
		},
		// PET
		{
			petModelStr,
			"pet.config",
			"petmodel_get",
			"petmodel_set",
			{},
			PETMODEL_8032
		},
		// PLUS4
		{
			plus4ModelStr,
			"plus4.config",
			"plus4model_get",
			"plus4model_set",
			"TEDBorderMode",
			PLUS4MODEL_PLUS4_NTSC
		},
		// VIC20
		{
			vic20ModelStr,
			"vic20.config",
			"vic20model_get",
			"vic20model_set",
			"VICBorderMode",
			VIC20MODEL_VIC20_NTSC
		},
	};
	auto libPath = makePluginLibPath(libName[std::to_underlying(system)], libBasePath);
	logMsg("loading VICE plugin:%s", libPath.data());
	auto lib = IG::openSharedLibrary(libPath.data(), {.resolveAllSymbols = true});
	if(!lib)
	{
		return {};
	}
	auto plugin = commonVicePlugin(lib, system);
	auto &conf = pluginConf[std::to_underlying(system)];
	loadSymbolCheck(plugin.model_get_, lib, conf.getModelFuncName);
	loadSymbolCheck(plugin.model_set_, lib, conf.setModelFuncName);
	//logMsg("getModel() address:%p", plugin.model_get_);
	//logMsg("setModel() address:%p", plugin.model_set_);
	plugin.modelNames = conf.modelNames;
	plugin.configName = conf.configName;
	plugin.defaultModelId = conf.defaultModelId;
	plugin.modelIdBase = conf.modelIdBase;
	plugin.borderModeStr = conf.borderModeStr;
	plugin.libHandle = lib;
	return plugin;
}
