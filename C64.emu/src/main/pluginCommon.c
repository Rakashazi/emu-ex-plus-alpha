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

#ifdef __ANDROID__
#include <android/log.h>
#endif
#include <imagine/util/builtins.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>
#include "machine.h"
#include "maincpu.h"
#include "drive.h"
#include "lib.h"
#include "util.h"
#include "ioutil.h"
#include "uiapi.h"
#include "console.h"
#include "monitor.h"
#include "video.h"
#include "cmdline.h"
#include "gfxoutput.h"
#include "videoarch.h"
#include "init.h"
#include "resources.h"
#include "sysfile.h"
#include "log.h"
#include "archdep.h"
#include "palette.h"
#include "c64model.h"
#include "keyboard.h"
#include "autostart.h"
#include "kbdbuf.h"
#include "attach.h"
#include "raster.h"
#include "sound.h"
#include "cartridge.h"
#include "tape.h"
#include "viewport.h"
#include "vsync.h"

int console_mode = 0;
int video_disabled_mode = 0;
extern void (*vsync_hook)(void);

int vsync_do_vsync2(struct video_canvas_s *c, int been_skipped);

int c64ui_init(void) { return 0; }
int c64scui_init(void) { return 0; }
int scpu64ui_init(void) { return 0; }
int c64dtvui_init(void) { return 0; }
int c128ui_init(void) { return 0; }
int plus4ui_init(void) { return 0; }
int vic20ui_init(void) { return 0; }
int petui_init(void) { return 0; }
int cbm2ui_init(void) { return 0; }
int cbm5x0ui_init(void) { return 0; }
int video_init_cmdline_options(void) { return 0; }
void c64ui_shutdown(void) {}
void c64dtvui_shutdown(void) {}
void c128ui_shutdown(void) {}
void cbm2ui_shutdown() {}
void cbm5x0ui_shutdown() {}
void petui_shutdown() {}
void plus4ui_shutdown() {}
void scpu64ui_shutdown() {}
void vic20ui_shutdown() {}

#ifndef COMMON_KBD
int joystick_resources_init(void) { return 0; }
int c128_kbd_init(void) { return 0; }
int vic20_kbd_init(void) { return 0; }
int cbm2_kbd_init(void) { return 0; }
int pet_kbd_init(void) { return 0; }
int plus4_kbd_init(void) { return 0; }
int pet_kbd_resources_init(void) { return 0; }
void keyboard_register_caps_key(key_ctrl_caps_func_t func) {}
void keyboard_register_column4080_key(key_ctrl_column4080_func_t func) {}
#endif

log_t log_open(const char *id)
{
	return 0;
}

int log_close(log_t log)
{
	return 0;
}

void log_close_all(void) {}

int log_cmdline_options_init(void)
{
	return 0;
}

int log_resources_init(void)
{
	return 0;
}

void log_resources_shutdown(void) {}

int archdep_spawn(const char *name, char **argv, char **pstdout_redir, const char *stderr_redir)
{
  return -1;
}

char *archdep_quote_parameter(const char *name)
{
	/*not needed(?) */
	return lib_stralloc(name);
}

char *archdep_tmpnam(void)
{
	return NULL;
}

int archdep_file_is_gzip(const char *name)
{
	size_t l = strlen(name);

	if ((l < 4 || strcasecmp(name + l - 3, ".gz")) && (l < 3 || strcasecmp(name + l - 2, ".z")) && (l < 4 || toupper(name[l - 1]) != 'Z' || name[l - 4] != '.')) {
		return 0;
	}
	return 1;
}

char *archdep_make_backup_filename(const char *fname)
{
	return util_concat(fname, "~", NULL);
}

int archdep_file_set_gzip(const char *name)
{
    return 0;
}

char *archdep_default_save_resource_file_name(void)
{
	return NULL;
}

//FILE *archdep_open_default_log_file(void) { return stderr; }

int archdep_default_logger(const char *level_string, const char *txt) { return 0; }

void ui_update_menus(void) {}

int ui_extend_image_dialog(void)
{
	return 0;
}

void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2) {}
void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number) {}
void ui_display_joyport(BYTE *joyport) {}
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color) {}
int uicolor_alloc_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long *color_pixel,
                               BYTE *pixel_return) { return 0; }
void uicolor_free_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long color_pixel) {}
void uicolor_convert_color_table(unsigned int colnr, BYTE *data,
                                        long color_pixel, void *c) {}

char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
	static char buff[] = "";
	return buff;
}

void uimon_notify_change(void) {}

console_t *uimon_window_resume(void)
{
	return NULL;
}

void uimon_window_close(void) {}

console_t *uimon_window_open(void)
{
	return NULL;
}

void signals_pipe_set(void) {}

void signals_pipe_unset(void) {}

void vsid_ui_display_name(const char *name)
{
//    strncpy(vsidstrings[VSID_S_TITLE], name, 40);
//    log_message(LOG_DEFAULT, "Title: %s", vsidstrings[VSID_S_TITLE]);
}

void vsid_ui_display_author(const char *author)
{
//    strncpy(vsidstrings[VSID_S_AUTHOR], author, 40);
//    log_message(LOG_DEFAULT, "Author: %s", vsidstrings[VSID_S_AUTHOR]);
}

void vsid_ui_display_copyright(const char *copyright)
{
//    strncpy(vsidstrings[VSID_S_RELEASED], copyright, 40);
//    log_message(LOG_DEFAULT, "Released: %s", vsidstrings[VSID_S_RELEASED]);
}

void vsid_ui_display_sync(int sync)
{
//    sprintf(vsidstrings[VSID_S_SYNC], "Using %s sync", sync == MACHINE_SYNC_PAL ? "PAL" : "NTSC");
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_SYNC]);
}

void vsid_ui_display_sid_model(int model)
{
//    sprintf(vsidstrings[VSID_S_MODEL], "Using %s emulation", csidmodel[model > 19 ? 7 : model]);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_MODEL]);
}

void vsid_ui_set_default_tune(int nr)
{
//    sprintf(vsidstrings[VSID_S_DEFAULT],"Default tune: %d", nr);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_DEFAULT]);
//    sdl_vsid_default_tune = nr;
}

void vsid_ui_display_tune_nr(int nr)
{
//    sprintf(vsidstrings[VSID_S_PLAYING],"Playing tune: %d", nr);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_PLAYING]);
//    sdl_vsid_current_tune = nr;
//
//    if (sdl_vsid_state & SDL_VSID_ACTIVE) {
//        sdl_vsid_state |= SDL_VSID_REPAINT;
//    }
}

void vsid_ui_display_nr_of_tunes(int count)
{
//    sprintf(vsidstrings[VSID_S_TUNES],"Number of tunes: %d", count);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_TUNES]);
//    sdl_vsid_tunes = count;
}

void vsid_ui_display_irqtype(const char *irq) {}

void vsid_ui_display_time(unsigned int sec) {}

void vsid_ui_setdrv(char* driver_info_text) {}

char *archdep_filename_parameter(const char *name)
{
    /* nothing special(?) */
    return lib_stralloc(name);
}

int archdep_path_is_relative(const char *path)
{
    if (path == NULL) {
        return 0;
    }

    return *path != '/';
}

/* return malloc'd version of full pathname of orig_name */
int archdep_expand_path(char **return_path, const char *orig_name)
{
    /* Unix version.  */
    if (*orig_name == '/') {
        *return_path = lib_stralloc(orig_name);
    } else {
        static char *cwd;

        cwd = ioutil_current_dir();
        *return_path = util_concat(cwd, "/", orig_name, NULL);
        lib_free(cwd);
    }
    return 0;
}

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
	return NULL;
}

char *archdep_default_fliplist_file_name(void)
{
	return lib_stralloc("");
}

char *archdep_default_autostart_disk_image_file_name(void)
{
	return lib_stralloc("");
}

char *archdep_default_resource_file_name(void)
{
	return lib_stralloc("");
}

int archdep_mkdir(const char *pathname, int mode)
{
	return mkdir(pathname, mode);
}

int archdep_stat(const char *file_name, unsigned int *len, unsigned int *isdir)
{
	struct stat statbuf;

	if (stat(file_name, &statbuf) < 0) {
			*len = 0;
			*isdir = 0;
			return -1;
	}

	*len = statbuf.st_size;
	*isdir = S_ISDIR(statbuf.st_mode);

	return 0;
}

int archdep_rename(const char *oldpath, const char *newpath)
{
	return -1;
}

signed long kbd_arch_keyname_to_keynum(char *keyname)
{
	return (signed long)atoi(keyname);
}

int kbd_arch_get_host_mapping(void)
{
	return KBD_MAPPING_US;
}

void archdep_shutdown(void) {}
void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count) {}
void uimon_window_suspend(void) {}
void fullscreen_resume(void) {}
void fullscreen_capability(cap_fullscreen_t *cap_fullscreen) {}
void ui_display_tape_current_image(const char *image) {}
void ui_display_drive_current_image(unsigned int drive_number, const char *image) {}
void ui_display_tape_control_status(int control) {}
void ui_display_tape_motor_status(int motor) {}
void ui_display_tape_counter(int counter) {}
void ui_display_recording(int recording_status) {}
void ui_display_playback(int playback_status, char *version) {}
void ui_display_event_time(unsigned int current, unsigned int total) {}
void ui_display_volume(int vol) {}
void ui_cmdline_show_help(unsigned int num_options, cmdline_option_ram_t *options, void *userparam) {}
void ui_set_tape_status(int tape_status) {}
char* ui_get_file(const char *format,...) { return NULL; }
void ui_shutdown(void) {}
void ui_resources_shutdown(void) {}
ui_jam_action_t ui_jam_dialog(const char *format, ...) { return UI_JAM_NONE; }
int c64_kbd_init(void) { return 0; }
void kbd_arch_init(void) {}
void kbd_initialize_numpad_joykeys(int* joykeys) {}
int kbd_cmdline_options_init(void) { return 0; }
int kbd_resources_init(void) { return 0; }
int joystick_arch_init_resources(void) { return 0; }
void joy_arch_init_default_mapping(int joynum) {}
int joystick_init_resources(void) { return 0; }
int console_close_all(void) { return 0; }
void video_shutdown(void) {}
gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname) { return NULL; }
void gfxoutput_shutdown(void) {}
int video_arch_resources_init(void) { return 0; }
void video_arch_resources_shutdown(void) {}
char video_canvas_can_resize(video_canvas_t *canvas) { return 1; }
void video_canvas_destroy(struct video_canvas_s *canvas) {}
int ui_resources_init() { return 0; }
int gfxoutput_resources_init() { return 0; }
int signals_init(int do_core_dumps) { return 0; }
int gfxoutput_init() { return 0; }
int console_init() { return 0; }
int ui_init_finalize() { return 0; }
int gfxoutput_cmdline_options_init() { return 0; }
int ui_cmdline_options_init() { return 0; }
int screenshot_init() { return 0; }
int screenshot_record() { return 0; }
int screenshot_save(const char *drvname, const char *filename, struct video_canvas_s *canvas) { return 0; }
void screenshot_prepare_reopen() {}
void screenshot_try_reopen() {}
void sysfile_shutdown() {}
int sysfile_resources_init() { return 0; }
void sysfile_resources_shutdown() {}
int sysfile_cmdline_options_init() { return 0; }
int cmdline_register_options(const cmdline_option_t *c) { return 0; }
int cmdline_init() { return 0; }
int initcmdline_init() { return 0; }
void cmdline_shutdown() {}
void video_render_1x2_init() {}
void video_render_2x2_init() {}

#define DUMMY_VIDEO_RENDER(func) void func(video_render_color_tables_t *color_tab, \
const BYTE *src, BYTE *trg, \
unsigned int width, const unsigned int height, \
const unsigned int xs, const unsigned int ys, \
const unsigned int xt, const unsigned int yt, \
const unsigned int pitchs, const unsigned int pitcht, \
viewport_t *viewport, video_render_config_t *config) {}

DUMMY_VIDEO_RENDER(render_16_2x4_crt)
DUMMY_VIDEO_RENDER(render_32_scale2x)
DUMMY_VIDEO_RENDER(render_32_2x4_04)
DUMMY_VIDEO_RENDER(render_32_2x2_04)
DUMMY_VIDEO_RENDER(render_16_2x2_crt)
DUMMY_VIDEO_RENDER(render_08_2x4_04)
DUMMY_VIDEO_RENDER(render_08_scale2x)
DUMMY_VIDEO_RENDER(render_32_2x4_crt)
DUMMY_VIDEO_RENDER(render_24_2x4_crt)
DUMMY_VIDEO_RENDER(render_24_scale2x)
DUMMY_VIDEO_RENDER(render_16_scale2x)
DUMMY_VIDEO_RENDER(render_16_2x4_04)
DUMMY_VIDEO_RENDER(render_24_2x4_04)
DUMMY_VIDEO_RENDER(render_08_2x2_04)
DUMMY_VIDEO_RENDER(render_32_2x2_crt)
DUMMY_VIDEO_RENDER(render_24_2x2_crt)
DUMMY_VIDEO_RENDER(render_24_2x2_04)
DUMMY_VIDEO_RENDER(render_16_2x2_04)
DUMMY_VIDEO_RENDER(render_16_2x2_pal)
DUMMY_VIDEO_RENDER(render_16_2x2_ntsc)
DUMMY_VIDEO_RENDER(render_24_2x2_pal)
DUMMY_VIDEO_RENDER(render_24_2x2_ntsc)
DUMMY_VIDEO_RENDER(render_32_2x2_ntsc)
DUMMY_VIDEO_RENDER(render_32_2x2_pal)

int mousedrv_resources_init(void) { return 0; }
int mousedrv_cmdline_options_init(void) { return 0; }
void mousedrv_init(void) {}

void mousedrv_mouse_changed(void) {}

int mousedrv_get_x(void) { return 0; }
int mousedrv_get_y(void) { return 0; }
unsigned long mousedrv_get_timestamp(void) { return 0; }

void mouse_button(int bnumber, int state) {}
void mouse_move(int x, int y) {}

void vsyncarch_init(void) {}

// Number of timer units per second, unused
signed long vsyncarch_frequency(void)
{
  return 1000000;
}

// unused
unsigned long vsyncarch_gettime(void)
{
	return 0;
}

void vsyncarch_display_speed(double speed, double frame_rate, int warp_enabled) {}

void vsyncarch_sleep(signed long delay) {}

void vsyncarch_presync(void) {}

void vsyncarch_postsync(void) {}

int vsync_do_vsync(struct video_canvas_s *c, int been_skipped)
{
	sound_flush();
	kbdbuf_flush();
	vsync_hook();
	return vsync_do_vsync2(c, been_skipped);
}

VICE_API int vice_init()
{
	maincpu_early_init();
	machine_setup_context();
	drive_setup_context();
	machine_early_init();

	// Initialize system file locator
	sysfile_init(machine_name);

	if(init_resources() < 0)
	{
		return -1;
	}

	// Set factory defaults
	if(resources_set_defaults() < 0)
	{
		return -1;
	}

	/*if(log_init() < 0)
	{
		bug_exit("log_init()");
	}*/

	return 0;
}

void bug_doExit(const char *msg, ...)
{
	#ifdef __ANDROID__
	va_list args;
	va_start(args, msg);
	char str[256];
	vsnprintf(str, sizeof(str), msg, args);
	__android_log_assert("", "imagine", "%s", str);
	#else
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	fprintf(stderr, "\n");
	abort();
	#endif
}
