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
#include <span>
#include <string_view>

struct keyboard_conv_t;

enum class ViceSystem: uint8_t
{
	C64 = 0,
	C64SC = 1,
	C64DTV = 2,
	C128 = 3,
	SUPER_CPU = 4,
	CBM2 = 5,
	CBM5X0 = 6,
	PET = 7,
	PLUS4 = 8,
	VIC20 = 9
};

struct VicePlugin
{
	static constexpr int SYSTEMS = 10;
	IG::SharedLibraryRef libHandle{};
	uint16_t (*joystick_value)[JOYPORT_MAX_PORTS]{};
	int *warp_mode_enabled{};
	std::span<const std::string_view> modelNames{};
	std::string_view configName{};
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
	struct drive_type_info_s *(*machine_drive_get_type_info_list_)(){};
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
	const char *(*tape_get_file_name_)(int port){};
	void (*datasette_control_)(int port, int command){};
	int (*file_system_attach_disk_)(unsigned int unit, unsigned int drive, const char *filename){};
	void (*file_system_detach_disk_)(unsigned int unit, unsigned int drive){};
	const char *(*file_system_get_disk_name_)(unsigned int unit, unsigned int drive){};
	int (*drive_check_type_)(unsigned int drive_type, unsigned int dnr){};
	int (*sound_register_device_)(const sound_device_t *pdevice){};
	void (*video_canvas_render_)(struct video_canvas_s *canvas, uint8_t *trg,
		int width, int height, int xs, int ys,
		int xt, int yt, int pitcht){};
	void (*video_render_setphysicalcolor_)(video_render_config_t *config,
		int index, uint32_t color, int depth){};
	void (*video_render_setrawrgb_)(video_render_color_tables_t *color_tab, unsigned int index,
		uint32_t r, uint32_t g, uint32_t b){};
	void (*video_render_initraw_)(struct video_render_config_s *videoconfig){};
	int (*vdrive_internal_create_format_disk_image_)(const char *filename, const char *diskname, unsigned int type);
	int (*cbmimage_create_image_)(const char *name, unsigned int type){};
	void (*keyboard_key_pressed_direct_)(signed long key, int mod, int pressed){};
	void (*keyboard_key_clear_)(void){};
	void (*vsync_set_warp_mode_)(int val){};
	int8_t defaultModelId{};
	int8_t modelIdBase;

	void init();
	void deinit();
	static bool hasSystemLib(ViceSystem system, const char *libBasePath);
	static const char *systemName(ViceSystem system);
	int8_t modelIdLimit() const { return modelIdBase + modelNames.size(); }
	int model_get() const;
	void model_set(int model);
	int resources_get_string(const char *name, const char **value_return) const;
	int resources_set_string(const char *name, const char *value);
	int resources_get_int(const char *name, int *value_return) const;
	int resources_set_int(const char *name, int value);
	int resources_get_default_value(const char *name, void *value_return) const;
	int machine_write_snapshot(const char *name, int save_roms, int save_disks, int even_mode) const;
	int machine_read_snapshot(const char *name, int event_mode) const;
	void machine_set_restore_key(int v);
	void machine_trigger_reset(const unsigned int mode);
	struct drive_type_info_s *machine_drive_get_type_info_list();
	void interrupt_maincpu_trigger_trap(void trap_func(uint16_t, void *data), void *data) const;
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
	const char *tape_get_file_name(int port);
	void datasette_control(int port, int command);
	int file_system_attach_disk(unsigned int unit, unsigned int drive, const char *filename);
	void file_system_detach_disk(unsigned int unit, unsigned int drive);
	const char *file_system_get_disk_name(unsigned int unit, unsigned int drive);
	int drive_check_type(unsigned int drive_type, unsigned int dnr);
	int sound_register_device(const sound_device_t *pdevice);
	void video_canvas_render(struct video_canvas_s *canvas, uint8_t *trg,
    int width, int height, int xs, int ys,
    int xt, int yt, int pitcht);
	void video_render_setphysicalcolor(video_render_config_t *config,
		int index, uint32_t color, int depth);
	void video_render_setrawrgb(video_render_color_tables_t *color_tab, unsigned int index,
		uint32_t r, uint32_t g, uint32_t b);
	void video_render_initraw(struct video_render_config_s *videoconfig);
	int vdrive_internal_create_format_disk_image(const char *filename, const char *diskname, unsigned int type);
	int cbmimage_create_image(const char *name, unsigned int type);
	void keyboard_key_pressed_direct(signed long key, int mod, int pressed) { keyboard_key_pressed_direct_(key, mod, pressed); }
	void keyboard_key_clear(void);
	void vsync_set_warp_mode(int val);

	explicit operator bool()
	{
		return libHandle;
	}
};

VicePlugin loadVicePlugin(ViceSystem system, const char *libBasePath);
