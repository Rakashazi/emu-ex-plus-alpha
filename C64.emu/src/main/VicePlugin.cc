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

#include <dlfcn.h>
#include <cstring>
#include <imagine/logger/logger.h>
#include <imagine/fs/FS.hh>
#include <imagine/base/Base.hh>
#include <imagine/util/utility.h>
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
	uint models;
	const char **modelStr;
	const char *getModelFuncName;
	const char *setModelFuncName;
	const char *borderModeStr;
};

static FS::PathString libPath{};

static FS::PathString makePluginLibPath(const char *libName)
{
	if(unlikely(!std::strlen(libPath.data())))
	{
		libPath = Base::libPath();
	}
	return FS::makePathStringPrintf("%s/%s", libPath.data(), libName);
}

void VicePlugin::deinit()
{
	if(libHandle)
	{
		// TODO: doesn't fully clean up all VICE heap allocations, don't use for now
		using ShutdownFunc = void (*)();
		ShutdownFunc machine_shutdown = (ShutdownFunc)dlsym(libHandle, "machine_shutdown");
		machine_shutdown();
		dlclose(libHandle);
		libHandle = {};
	}
}

bool VicePlugin::hasSystemLib(ViceSystem system)
{
	if(system < VICE_SYSTEM_C64 || system > VICE_SYSTEM_VIC20)
		return false;
	return FS::exists(makePluginLibPath(libName[system]));
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

void VicePlugin::interrupt_maincpu_trigger_trap(void trap_func(WORD, void *data), void *data)
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

int VicePlugin::sound_register_device(sound_device_t *pdevice)
{
	if(sound_register_device_)
		return sound_register_device_(pdevice);
	return -1;
}

char *VicePlugin::lib_stralloc(const char *str)
{
	if(lib_stralloc_)
		return lib_stralloc_(str);
	return nullptr;
}

void VicePlugin::video_canvas_render(struct video_canvas_s *canvas, BYTE *trg,
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
	int index, DWORD color, int depth)
{
	if(video_render_setphysicalcolor_)
	{
		video_render_setphysicalcolor_(config, index, color, depth);
	}
}

void VicePlugin::video_render_setrawrgb(unsigned int index, DWORD r, DWORD g, DWORD b)
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
	plugin.keyarr = (typeof plugin.keyarr)dlsym(lib, "keyarr");
	plugin.rev_keyarr = (typeof plugin.rev_keyarr)dlsym(lib, "rev_keyarr");
	plugin.joystick_value = (typeof plugin.joystick_value)dlsym(lib, "joystick_value");
	plugin.warp_mode_enabled = (typeof plugin.warp_mode_enabled)dlsym(lib, "warp_mode_enabled");
	plugin.resources_get_string_ = (typeof plugin.resources_get_string_)dlsym(lib, "resources_get_string");
	plugin.resources_set_string_ = (typeof plugin.resources_set_string_)dlsym(lib, "resources_set_string");
	plugin.resources_get_int_ = (typeof plugin.resources_get_int_)dlsym(lib, "resources_get_int");
	plugin.resources_set_int_ = (typeof plugin.resources_set_int_)dlsym(lib, "resources_set_int");
	plugin.machine_write_snapshot_ = (typeof plugin.machine_write_snapshot_)dlsym(lib, "machine_write_snapshot");
	plugin.machine_read_snapshot_ = (typeof plugin.machine_read_snapshot_)dlsym(lib, "machine_read_snapshot");
	plugin.machine_set_restore_key_ = (typeof plugin.machine_set_restore_key_)dlsym(lib, "machine_set_restore_key");
	plugin.machine_trigger_reset_ = (typeof plugin.machine_trigger_reset_)dlsym(lib, "machine_trigger_reset");
	plugin.interrupt_maincpu_trigger_trap_ = (typeof plugin.interrupt_maincpu_trigger_trap_)dlsym(lib, "interrupt_maincpu_trigger_trap");
	plugin.init_main_ = (typeof plugin.init_main_)dlsym(lib, "init_main");
	plugin.maincpu_mainloop_ = (typeof plugin.maincpu_mainloop_)dlsym(lib, "maincpu_mainloop");
	if(system == VICE_SYSTEM_PET)
	{
		plugin.autostart_autodetect_ = (typeof plugin.autostart_autodetect_)dlsym(lib, "autostart_autodetect");
		// no cart system
	}
	else if(system == VICE_SYSTEM_PLUS4)
	{
		plugin.autostart_autodetect_ = (typeof plugin.autostart_autodetect_)dlsym(lib, "autostart_autodetect");
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
		plugin.cartridge_attach_image_ = (typeof plugin.cartridge_attach_image_)dlsym(lib, "cartridge_attach_image");
		plugin.cartridge_detach_image_ = (typeof plugin.cartridge_detach_image_)dlsym(lib, "cartridge_detach_image");
	}
	else if(system == VICE_SYSTEM_CBM2 || system == VICE_SYSTEM_CBM5X0)
	{
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return CARTRIDGE_CBM2_8KB_1000;
			};
		plugin.cartridge_attach_image_ = (typeof plugin.cartridge_attach_image_)dlsym(lib, "cartridge_attach_image");
		plugin.cartridge_detach_image_ = (typeof plugin.cartridge_detach_image_)dlsym(lib, "cartridge_detach_image");
	}
	else if(system == VICE_SYSTEM_VIC20)
	{
		plugin.autostart_autodetect_ = (typeof plugin.autostart_autodetect_)dlsym(lib, "autostart_autodetect");
		plugin.cart_getid_slotmain_ =
			[]()
			{
				return CARTRIDGE_VIC20_DETECT;
			};
		plugin.cartridge_get_file_name_ = (typeof plugin.cartridge_get_file_name_)dlsym(lib, "cartridge_get_file_name");
		plugin.cartridge_attach_image_ = (typeof plugin.cartridge_attach_image_)dlsym(lib, "cartridge_attach_image");
		plugin.cartridge_detach_image_ = (typeof plugin.cartridge_detach_image_)dlsym(lib, "cartridge_detach_image");
	}
	else
	{
		plugin.autostart_autodetect_ = (typeof plugin.autostart_autodetect_)dlsym(lib, "autostart_autodetect");
		plugin.cart_getid_slotmain_ = (typeof plugin.cart_getid_slotmain_)dlsym(lib, "cart_getid_slotmain");
		plugin.cartridge_get_file_name_ = (typeof plugin.cartridge_get_file_name_)dlsym(lib, "cartridge_get_file_name");
		plugin.cartridge_attach_image_ = (typeof plugin.cartridge_attach_image_)dlsym(lib, "cartridge_attach_image");
		plugin.cartridge_detach_image_ = (typeof plugin.cartridge_detach_image_)dlsym(lib, "cartridge_detach_image");
	}
	plugin.tape_image_attach_ = (typeof plugin.tape_image_attach_)dlsym(lib, "tape_image_attach");
	plugin.tape_image_detach_ = (typeof plugin.tape_image_detach_)dlsym(lib, "tape_image_detach");
	plugin.tape_get_file_name_ = (typeof plugin.tape_get_file_name_)dlsym(lib, "tape_get_file_name");
	plugin.file_system_attach_disk_ = (typeof plugin.file_system_attach_disk_)dlsym(lib, "file_system_attach_disk");
	plugin.file_system_detach_disk_ = (typeof plugin.file_system_detach_disk_)dlsym(lib, "file_system_detach_disk");
	plugin.file_system_get_disk_name_ = (typeof plugin.file_system_get_disk_name_)dlsym(lib, "file_system_get_disk_name");
	plugin.sound_register_device_ = (typeof plugin.sound_register_device_)dlsym(lib, "sound_register_device");
	plugin.lib_stralloc_ = (typeof plugin.lib_stralloc_)dlsym(lib, "lib_stralloc");
	plugin.video_canvas_render_ = (typeof plugin.video_canvas_render_)dlsym(lib, "video_canvas_render");
	plugin.video_render_setphysicalcolor_ = (typeof plugin.video_render_setphysicalcolor_)dlsym(lib, "video_render_setphysicalcolor");
	plugin.video_render_setrawrgb_ = (typeof plugin.video_render_setrawrgb_)dlsym(lib, "video_render_setrawrgb");
	plugin.video_render_initraw_ = (typeof plugin.video_render_initraw_)dlsym(lib, "video_render_initraw");
	plugin.vdrive_internal_create_format_disk_image_ = (typeof plugin.vdrive_internal_create_format_disk_image_)dlsym(lib, "vdrive_internal_create_format_disk_image");
	return plugin;
}

VicePlugin loadVicePlugin(ViceSystem system)
{
	constexpr PluginConfig pluginConf[]
	{
		// C64
		{
			IG::size(c64ModelStr),
			c64ModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// C64 (accurate)
		{
			IG::size(c64ModelStr),
			c64ModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// DTV
		{
			IG::size(dtvModelStr),
			dtvModelStr,
			"dtvmodel_get",
			"dtvmodel_set",
			"VICIIBorderMode"
		},
		// C128
		{
			IG::size(c128ModelStr),
			c128ModelStr,
			"c128model_get",
			"c128model_set",
			"VICIIBorderMode"
		},
		// C64 Super CPU
		{
			IG::size(superCPUModelStr),
			superCPUModelStr,
			"c64model_get",
			"c64model_set",
			"VICIIBorderMode"
		},
		// CBM-II 6x0
		{
			IG::size(cbm2ModelStr),
			cbm2ModelStr,
			"cbm2model_get",
			"cbm2model_set"
		},
		// CBM-II 5x0
		{
			IG::size(cbm5x0ModelStr),
			cbm5x0ModelStr,
			"cbm2model_get",
			"cbm2model_set",
			"VICIIBorderMode"
		},
		// PET
		{
			IG::size(petModelStr),
			petModelStr,
			"petmodel_get",
			"petmodel_set"
		},
		// PLUS4
		{
			IG::size(plus4ModelStr),
			plus4ModelStr,
			"plus4model_get",
			"plus4model_set",
			"TEDBorderMode"
		},
		// VIC20
		{
			IG::size(vic20ModelStr),
			vic20ModelStr,
			"vic20model_get",
			"vic20model_set",
			"VICBorderMode"
		},
	};
	FS::PathString libPath = makePluginLibPath(libName[system]);
	logMsg("loading VICE plugin:%s", libPath.data());
	auto lib = dlopen(libPath.data(), RTLD_NOW);
	if(!lib)
	{
		logErr("dlopen error: %s", dlerror());
		return {};
	}
	auto plugin = commonVicePlugin(lib, system);
	auto &conf = pluginConf[system];
	plugin.models = conf.models;
	plugin.modelStr = conf.modelStr;
	plugin.model_get_ = (typeof plugin.model_get_)dlsym(lib, conf.getModelFuncName);
	plugin.model_set_ = (typeof plugin.model_set_)dlsym(lib, conf.setModelFuncName);
	plugin.borderModeStr = conf.borderModeStr;
	//logMsg("getModel() address:%p", plugin.model_get_);
	//logMsg("setModel() address:%p", plugin.model_set_);
	using ViceInitFunc = int (*)();
	ViceInitFunc vice_init = (ViceInitFunc)dlsym(lib, "vice_init");
	vice_init();
	plugin.libHandle = lib;
	return plugin;
}
