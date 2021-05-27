#pragma once

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

extern "C"
{
	#include "keyboard.h"
	#include "joystick.h"
	#include "interrupt.h"
	#include "sound.h"
	#include "video.h"
}

#include <imagine/base/sharedLibrary.hh>

struct keyboard_conv_t;

enum ViceSystem
{
	VICE_SYSTEM_C64 = 0,
	VICE_SYSTEM_C64SC = 1,
	VICE_SYSTEM_C64DTV = 2,
	VICE_SYSTEM_C128 = 3,
	VICE_SYSTEM_SUPER_CPU = 4,
	VICE_SYSTEM_CBM2 = 5,
	VICE_SYSTEM_CBM5X0 = 6,
	VICE_SYSTEM_PET = 7,
	VICE_SYSTEM_PLUS4 = 8,
	VICE_SYSTEM_VIC20 = 9
};

struct VicePlugin
{
	static constexpr int SYSTEMS = 10;
	Base::SharedLibraryRef libHandle{};
	int (*keyarr)[KBD_ROWS]{};
	int (*rev_keyarr)[KBD_COLS]{};
	uint8_t (*joystick_value)[JOYSTICK_NUM + 1]{};
	keyboard_conv_t **keyconvmap{};
	int *warp_mode_enabled{};
	int models = 0;
	const char **modelStr{};
	const char *borderModeStr{""};
	int (*model_get_)(){};
	void (*model_set_)(int model){};
	int (*resources_get_string_)(const char *name, const char **value_return){};
	int (*resources_set_string_)(const char *name, const char *value){};
	int (*resources_get_int_)(const char *name, int *value_return){};
	int (*resources_set_int_)(const char *name, int value){};
	int (*resources_get_default_value_)(const char *name, void *value_return){};
	int (*machine_write_snapshot_)(const char *name, int save_roms, int save_disks, int even_mode){};
	int (*machine_read_snapshot_)(const char *name, int event_mode){};
	void (*machine_set_restore_key_)(int v){};
	void (*machine_trigger_reset_)(const unsigned int mode){};
	void (*interrupt_maincpu_trigger_trap_)(void (*trap_func_)(uint16_t, void *data), void *data){};
	int (*init_main_)(){};
	void (*maincpu_mainloop_)(){};
	int (*autostart_autodetect_)(const char *file_name, const char *program_name,
		unsigned int program_number, unsigned int runmode){};
	int (*cart_getid_slotmain_)(){};
	const char *(*cartridge_get_file_name_)(int type){};
	int (*cartridge_attach_image_)(int type, const char *filename){};
	void (*cartridge_detach_image_)(int type){};
	int (*tape_image_attach_)(unsigned int unit, const char *name){};
	int (*tape_image_detach_)(unsigned int unit){};
	const char *(*tape_get_file_name_)(){};
	void (*datasette_control_)(int command){};
	int (*file_system_attach_disk_)(unsigned int unit, const char *filename){};
	void (*file_system_detach_disk_)(int unit){};
	const char *(*file_system_get_disk_name_)(unsigned int unit){};
	int (*drive_check_type_)(unsigned int drive_type, unsigned int dnr){};
	int (*sound_register_device_)(sound_device_t *pdevice){};
	void (*video_canvas_render_)(struct video_canvas_s *canvas, uint8_t *trg,
		int width, int height, int xs, int ys,
		int xt, int yt, int pitcht, int depth){};
	void (*video_render_setphysicalcolor_)(video_render_config_t *config,
		int index, uint32_t color, int depth){};
	void (*video_render_setrawrgb_)(unsigned int index, uint32_t r, uint32_t g, uint32_t b){};
	void (*video_render_initraw_)(struct video_render_config_s *videoconfig){};
	int (*vdrive_internal_create_format_disk_image_)(const char *filename, const char *diskname, unsigned int type);

	void init();
	void deinit();
	static bool hasSystemLib(ViceSystem system, const char *libBasePath);
	static const char *systemName(ViceSystem system);
	int model_get();
	void model_set(int model);
	int resources_get_string(const char *name, const char **value_return);
	int resources_set_string(const char *name, const char *value);
	int resources_get_int(const char *name, int *value_return);
	int resources_set_int(const char *name, int value);
	int resources_get_default_value(const char *name, void *value_return);
	int machine_write_snapshot(const char *name, int save_roms, int save_disks, int even_mode);
	int machine_read_snapshot(const char *name, int event_mode);
	void machine_set_restore_key(int v);
	void machine_trigger_reset(const unsigned int mode);
	void interrupt_maincpu_trigger_trap(void trap_func(uint16_t, void *data), void *data);
	int init_main();
	void maincpu_mainloop();
	int autostart_autodetect(const char *file_name, const char *program_name,
		unsigned int program_number, unsigned int runmode);
	int cart_getid_slotmain();
	const char *cartridge_get_file_name(int type);
	int cartridge_attach_image(int type, const char *filename);
	void cartridge_detach_image(int type);
	int tape_image_attach(unsigned int unit, const char *name);
	int tape_image_detach(unsigned int unit);
	const char *tape_get_file_name();
	void datasette_control(int command);
	int file_system_attach_disk(unsigned int unit, const char *filename);
	void file_system_detach_disk(int unit);
	const char *file_system_get_disk_name(unsigned int unit);
	int drive_check_type(unsigned int drive_type, unsigned int dnr);
	int sound_register_device(sound_device_t *pdevice);
	void video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg,
		int width, int height, int xs, int ys,
		int xt, int yt, int pitcht, int depth);
	void video_render_setphysicalcolor(video_render_config_t *config,
		int index, uint32_t color, int depth);
	void video_render_setrawrgb(unsigned int index, uint32_t r, uint32_t g, uint32_t b);
	void video_render_initraw(struct video_render_config_s *videoconfig);
	int vdrive_internal_create_format_disk_image(const char *filename, const char *diskname, unsigned int type);

	explicit operator bool()
	{
		return libHandle;
	}
};

static const char *c64ModelStr[]
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

static const char *dtvModelStr[]
{
	"DTV v2 PAL",
	"DTV v2 NTSC",
	"DTV v3 PAL",
	"DTV v3 NTSC",
	"Hummer NTSC",
};

static const char *c128ModelStr[]
{
	"C128 PAL",
	"C128DCR PAL",
	"C128 NTSC",
	"C128DCR NTSC",
};

static const char *superCPUModelStr[]
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

static const char *cbm2ModelStr[]
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

static const char *cbm5x0ModelStr[]
{
	"CBM 510 PAL",
	"CBM 510 NTSC",
};

static const char *petModelStr[]
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

static const char *plus4ModelStr[]
{
	"C16/116 PAL",
	"C16/116 NTSC",
	"Plus4 PAL",
	"Plus4 NTSC",
	"V364 NTSC",
	"C232 NTSC",
};

static const char *vic20ModelStr[]
{
	"VIC20 PAL",
	"VIC20 NTSC",
	"VIC21",
};

VicePlugin loadVicePlugin(ViceSystem system, const char *libBasePath);
