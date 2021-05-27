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
#include <emuframework/EmuApp.hh>
#include "VicePlugin.hh"

extern "C"
{
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

struct PluginConfig
{
	unsigned models;
	const char **modelStr;
	const char *getModelFuncName;
	const char *setModelFuncName;
	const char *borderModeStr;
};

static FS::PathString makePluginLibPath(const char *libName, const char *libBasePath)
{
	if(libBasePath && strlen(libBasePath))
		return FS::makePathStringPrintf("%s/%s", libBasePath, libName);
	else
		return FS::makePathString(libName);
}

template<class T>
static void loadSymbolCheck(T &symPtr, Base::SharedLibraryRef lib, const char *name)
{
	if(!Base::loadSymbol(symPtr, lib, name))
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
		Base::closeSharedLibrary(libHandle);
		libHandle = {};
	}
}

bool VicePlugin::hasSystemLib(ViceSystem system, const char *libBasePath)
{
	if(system < VICE_SYSTEM_C64 || system > VICE_SYSTEM_VIC20)
		return false;
	return FS::exists(makePluginLibPath(libName[system], libBasePath));
}

const char *VicePlugin::systemName(ViceSystem system)
{
	if(system < VICE_SYSTEM_C64 || system > VICE_SYSTEM_VIC20)
		return "";
	return systemNameStr[system];
}

int VicePlugin::model_get()
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

int VicePlugin::resources_get_string(const char *name, const char **value_return)
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

int VicePlugin::resources_get_int(const char *name, int *value_return)
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

int VicePlugin::resources_get_default_value(const char *name, void *value_return)
{
	if(resources_get_default_value_)
		return resources_get_default_value_(name, value_return);
	return -1;
}

int VicePlugin::machine_write_snapshot(const char *name, int save_roms, int save_disks, int even_mode)
{
	if(machine_write_snapshot_)
		return machine_write_snapshot_(name, save_roms, save_disks, even_mode);
	return -1;
}

int VicePlugin::machine_read_snapshot(const char *name, int event_mode)
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

void VicePlugin::interrupt_maincpu_trigger_trap(void trap_func(uint16_t, void *data), void *data)
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
		bcase CARTRIDGE_CBM2_8KB_1000:
			resources_get_string("Cart1Name", &filename);
		bcase CARTRIDGE_CBM2_8KB_2000:
			resources_get_string("Cart2Name", &filename);
		bcase CARTRIDGE_CBM2_16KB_4000:
			resources_get_string("Cart4Name", &filename);
		bcase CARTRIDGE_CBM2_16KB_6000:
			resources_get_string("Cart6Name", &filename);
		bdefault:
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

const char *VicePlugin::tape_get_file_name()
{
	if(tape_get_file_name_)
		return tape_get_file_name_();
	return "";
}

void VicePlugin::datasette_control(int command)
{
	if(datasette_control_)
		datasette_control_(command);
}

int VicePlugin::file_system_attach_disk(unsigned int unit, const char *filename)
{
	if(file_system_attach_disk_)
		return file_system_attach_disk_(unit, filename);
	return -1;
}

void VicePlugin::file_system_detach_disk(int unit)
{
	if(file_system_detach_disk_)
		file_system_detach_disk_(unit);
}

const char *VicePlugin::file_system_get_disk_name(unsigned int unit)
{
	if(file_system_get_disk_name_)
		return file_system_get_disk_name_(unit);
	return "";
}

int VicePlugin::drive_check_type(unsigned int drive_type, unsigned int dnr)
{
	if(drive_check_type_)
		return drive_check_type_(drive_type, dnr);
	return 0;
}

int VicePlugin::sound_register_device(sound_device_t *pdevice)
{
	if(sound_register_device_)
		return sound_register_device_(pdevice);
	return -1;
}

void VicePlugin::video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg,
	int width, int height, int xs, int ys,
	int xt, int yt, int pitcht, int depth)
{
	if(video_canvas_render_)
	{
		video_canvas_render_(canvas, trg, width, height, xs, ys,
			xt, yt, pitcht, depth);
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

void VicePlugin::video_render_setrawrgb(unsigned int index, uint32_t r, uint32_t g, uint32_t b)
{
	if(video_render_setrawrgb_)
	{
		video_render_setrawrgb_(index, r, g, b);
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

VicePlugin commonVicePlugin(void *lib, ViceSystem system)
{
	VicePlugin plugin{};
	loadSymbolCheck(plugin.keyarr, lib, "keyarr");
	loadSymbolCheck(plugin.rev_keyarr, lib, "rev_keyarr");
	loadSymbolCheck(plugin.joystick_value, lib, "joystick_value");
	loadSymbolCheck(plugin.keyconvmap, lib, "keyconvmap");
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
	loadSymbolCheck(plugin.interrupt_maincpu_trigger_trap_, lib, "interrupt_maincpu_trigger_trap");
	loadSymbolCheck(plugin.init_main_, lib, "init_main");
	assert(plugin.init_main_);
	loadSymbolCheck(plugin.maincpu_mainloop_, lib, "maincpu_mainloop");
	if(system == VICE_SYSTEM_PET)
	{
		loadSymbolCheck(plugin.autostart_autodetect_, lib, "autostart_autodetect");
		// no cart system
	}
	else if(system == VICE_SYSTEM_PLUS4)
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
	else if(system == VICE_SYSTEM_CBM2 || system == VICE_SYSTEM_CBM5X0)
	{
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return CARTRIDGE_CBM2_8KB_1000;
			};
		loadSymbolCheck(plugin.cartridge_attach_image_, lib, "cartridge_attach_image");
		loadSymbolCheck(plugin.cartridge_detach_image_, lib, "cartridge_detach_image");
	}
	else if(system == VICE_SYSTEM_VIC20)
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
		if(system != VICE_SYSTEM_C64DTV)
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
	return plugin;
}

VicePlugin loadVicePlugin(ViceSystem system, const char *libBasePath)
{
	constexpr PluginConfig pluginConf[]
	{
		// C64
		{
			std::size(c64ModelStr),
			c64ModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// C64 (accurate)
		{
			std::size(c64ModelStr),
			c64ModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// DTV
		{
			std::size(dtvModelStr),
			dtvModelStr,
			"dtvmodel_get",
			"dtvmodel_set",
			"VICIIBorderMode"
		},
		// C128
		{
			std::size(c128ModelStr),
			c128ModelStr,
			"c128model_get",
			"c128model_set",
			"VICIIBorderMode"
		},
		// C64 Super CPU
		{
			std::size(superCPUModelStr),
			superCPUModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// CBM-II 6x0
		{
			std::size(cbm2ModelStr),
			cbm2ModelStr,
			"cbm2model_get",
			"cbm2model_set",
			{}
		},
		// CBM-II 5x0
		{
			std::size(cbm5x0ModelStr),
			cbm5x0ModelStr,
			"cbm2model_get",
			"cbm2model_set",
			"VICIIBorderMode"
		},
		// PET
		{
			std::size(petModelStr),
			petModelStr,
			"petmodel_get",
			"petmodel_set",
			{}
		},
		// PLUS4
		{
			std::size(plus4ModelStr),
			plus4ModelStr,
			"plus4model_get",
			"plus4model_set",
			"TEDBorderMode"
		},
		// VIC20
		{
			std::size(vic20ModelStr),
			vic20ModelStr,
			"vic20model_get",
			"vic20model_set",
			"VICBorderMode"
		},
	};
	FS::PathString libPath = makePluginLibPath(libName[system], libBasePath);
	logMsg("loading VICE plugin:%s", libPath.data());
	auto lib = Base::openSharedLibrary(libPath.data(), Base::RESOLVE_ALL_SYMBOLS_FLAG);
	if(!lib)
	{
		return {};
	}
	auto plugin = commonVicePlugin(lib, system);
	auto &conf = pluginConf[system];
	plugin.models = conf.models;
	plugin.modelStr = conf.modelStr;
	loadSymbolCheck(plugin.model_get_, lib, conf.getModelFuncName);
	loadSymbolCheck(plugin.model_set_, lib, conf.setModelFuncName);
	plugin.borderModeStr = conf.borderModeStr;
	//logMsg("getModel() address:%p", plugin.model_get_);
	//logMsg("setModel() address:%p", plugin.model_set_);
	plugin.libHandle = lib;
	return plugin;
}
