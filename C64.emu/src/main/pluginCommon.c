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
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/time.h>
#include "machine.h"
#include "maincpu.h"
#include "drive.h"
#include "lib.h"
#include "util.h"
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
#include "keymap.h"
#include "autostart.h"
#include "kbdbuf.h"
#include "attach.h"
#include "raster.h"
#include "sound.h"
#include "cartridge.h"
#include "tape.h"
#include "viewport.h"
#include "vsync.h"
#include "zfile.h"
#include "mousedrv.h"
#include "rs232.h"
#include "coproc.h"

VICE_API int vice_init();

bool console_mode = false;
bool video_disabled_mode = false;
bool help_requested = false;
bool default_settings_requested = false;
extern void (*vsync_hook)(void);
int rs232_useip232[RS232_NUM_DEVICES];

void vsync_do_vsync2(struct video_canvas_s *c);
void execute_vsync_callbacks(void);

int c64ui_init_early() { return 0; }
int c64ui_init() { return 0; }
int c64scui_init_early() { return 0; }
int c64scui_init() { return 0; }
int scpu64ui_init_early() { return 0; }
int scpu64ui_init() { return 0; }
int c64dtvui_init_early() { return 0; }
int c64dtvui_init() { return 0; }
int c128ui_init_early() { return 0; }
int c128ui_init() { return 0; }
int plus4ui_init_early() { return 0; }
int plus4ui_init() { return 0; }
int vic20ui_init_early() { return 0; }
int vic20ui_init() { return 0; }
int petui_init_early() { return 0; }
int petui_init() { return 0; }
int cbm2ui_init_early() { return 0; }
int cbm2ui_init() { return 0; }
int cbm5x0ui_init_early() { return 0; }
int cbm5x0ui_init() { return 0; }
int video_init_cmdline_options() { return 0; }
void c64ui_shutdown() {}
void c64dtvui_shutdown() {}
void c128ui_shutdown() {}
void cbm2ui_shutdown() {}
void cbm5x0ui_shutdown() {}
void petui_shutdown() {}
void plus4ui_shutdown() {}
void scpu64ui_shutdown() {}
void vic20ui_shutdown() {}

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

char *archdep_tmpnam(void)
{
	return NULL;
}

char *archdep_make_backup_filename(const char *fname)
{
	return util_concat(fname, "~", NULL);
}

//FILE *archdep_open_default_log_file(void) { return stderr; }

int archdep_default_logger(const char *level_string, const char *txt) { return 0; }

char *archdep_default_portable_resource_file_name(void) { return NULL; }

int archdep_is_haiku(void) { return -1; }

void archdep_sound_enable_default_device_tracking(void) {}

bool archdep_is_exiting(void) { return false; }

const char *archdep_home_path(void) { return ""; }

int archdep_rtc_get_centisecond(void)
{
	struct timeval t;
	gettimeofday(&t, NULL);
	return (int)(t.tv_usec / 10000);
}

int archdep_real_path_equal(const char *path1, const char *path2)
{
	return !strcmp(path1, path2);
}

void ui_update_menus(void) {}

int ui_extend_image_dialog(void)
{
	return 0;
}

void ui_display_reset(int device, int mode) {}

void ui_display_drive_led(unsigned int drive_number,
	unsigned int drive_base,
	unsigned int led_pwm1,
	unsigned int led_pwm2) {}
void ui_display_drive_track(unsigned int drive_number,
  unsigned int drive_base,
  unsigned int half_track_number,
  unsigned int disk_side) {}
void ui_display_joyport(uint16_t *joyport) {}
void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color) {}
int uicolor_alloc_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long *color_pixel,
                               uint8_t *pixel_return) { return 0; }
void uicolor_free_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long color_pixel) {}
void uicolor_convert_color_table(unsigned int colnr, uint8_t *data,
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

console_t *uimon_window_open(bool display_now)
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

int archdep_path_is_relative(const char *path)
{
	return 0; // avoid all relative path processing
}

/* return malloc'd version of full pathname of orig_name */
int archdep_expand_path(char **return_path, const char *orig_name)
{
	// all path strings should be treated opaque and never expanded
	*return_path = lib_strdup(orig_name);
	return 0;
}

FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
	return NULL;
}

char *archdep_default_fliplist_file_name(void)
{
	return lib_strdup("");
}

char *archdep_default_autostart_disk_image_file_name(void)
{
	return lib_strdup("");
}

char *archdep_default_resource_file_name(void)
{
	return lib_strdup("");
}

int archdep_mkdir(const char *pathname, int mode)
{
	return mkdir(pathname, mode);
}

int archdep_rmdir(const char *pathname)
{
	return rmdir(pathname);
}

int archdep_stat(const char *file_name, size_t *len, unsigned int *isdir)
{
	if(strstr(file_name, "://"))
	{
		*len = 0;
		*isdir = 0;
		return 0;
	}

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

int archdep_kbd_get_host_mapping(void)
{
	return KBD_MAPPING_US;
}

void archdep_vice_exit(int excode)
{
	assert(!"Should never call archdep_vice_exit()");
}

FILE *archdep_fdopen(int fd, const char *mode) { return fdopen(fd, mode); }
int archdep_close(int fd) { return close(fd); }
int archdep_fseeko(FILE *stream, off_t offset, int whence) { return fseeko(stream, offset, whence); }
off_t archdep_ftello(FILE *stream) { return ftello(stream); }
int archdep_access(const char *pathname, int mode) { return -1; }
bool archdep_file_exists(const char *path) { return false; }

archdep_dir_t *archdep_opendir(const char *path, int mode) { return NULL; }
const char *archdep_readdir(archdep_dir_t *dir) { return NULL; }
void archdep_closedir(archdep_dir_t *dir) {}
void archdep_rewinddir(archdep_dir_t *dir) {}
void archdep_seekdir(archdep_dir_t *dir, int pos) {}
int archdep_telldir(const archdep_dir_t *dir) { return -1; }
int archdep_chdir(const char *path) { return -1; }
char *archdep_current_dir(void) { return calloc(1, 1); }
char *archdep_getcwd(char *buf, size_t size) { return NULL; }
int archdep_remove(const char *path) { return -1; }

const char *archdep_extra_title_text() { return NULL; }
void archdep_shutdown(void) {}
void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count) {}
void uimon_window_suspend(void) {}
void fullscreen_resume(void) {}
void fullscreen_capability(cap_fullscreen_t *cap_fullscreen) {}
void ui_display_tape_current_image(int port, const char *image) {}
void ui_display_drive_current_image(unsigned int unit_number, unsigned int drive_number, const char *image) {}
void ui_display_tape_control_status(int port, int control) {}
void ui_display_tape_motor_status(int port, int motor) {}
void ui_display_recording(int recording_status) {}
void ui_display_playback(int playback_status, char *version) {}
void ui_display_event_time(unsigned int current, unsigned int total) {}
void ui_display_volume(int vol) {}
void ui_cmdline_show_help(unsigned int num_options, cmdline_option_ram_t *options, void *userparam) {}
void ui_set_tape_status(int port, int tape_status) {}
char* ui_get_file(const char *format,...) { return NULL; }
void ui_shutdown(void) {}
void ui_resources_shutdown(void) {}
ui_jam_action_t ui_jam_dialog(const char *format, ...) { return UI_JAM_NONE; }
void ui_message(const char *format, ...) {}
int console_close_all(void) { return 0; }
void video_shutdown(void) {}
gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname) { return NULL; }
void gfxoutput_shutdown(void) {}
int video_arch_resources_init(void) { return 0; }
void video_arch_resources_shutdown(void) {}
char video_canvas_can_resize(video_canvas_t *canvas) { return 1; }
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
void screenshot_shutdown() {}
void sysfile_shutdown() {}
int sysfile_resources_init() { return 0; }
void sysfile_resources_shutdown() {}
int sysfile_cmdline_options_init() { return 0; }
int cmdline_register_options(const cmdline_option_t *c) { return 0; }
int cmdline_init() { return 0; }
int initcmdline_init() { return 0; }
void initcmdline_shutdown() {}
void initcmdline_check_attach(void) {}
void cmdline_shutdown() {}
int video_arch_get_active_chip() { return VIDEO_CHIP_VICII; }
void video_render_1x2_init() {}
void video_render_2x2_init() {}
int cmdline_get_autostart_mode(void) { return AUTOSTART_MODE_NONE; }
void ui_actions_shutdown() {}
int ui_action_get_id(const char *name) { return 0; }
char *archdep_default_joymap_file_name(void) { return ""; }

#define DUMMY_VIDEO_RENDER(func) void func(const video_render_color_tables_t *color_tab, \
const uint8_t *src, uint8_t *trg, \
unsigned int width, const unsigned int height, \
const unsigned int xs, const unsigned int ys, \
const unsigned int xt, const unsigned int yt, \
const unsigned int pitchs, \
const unsigned int pitcht, \
const unsigned int doublescan, \
video_render_config_t *config) {}

#define DUMMY_VIDEO_RENDER_SCALE2X(func) void func(const video_render_color_tables_t *color_tab, \
const uint8_t *src, uint8_t *trg, \
unsigned int width, const unsigned int height, \
const unsigned int xs, const unsigned int ys, \
const unsigned int xt, const unsigned int yt, \
const unsigned int pitchs, \
const unsigned int pitcht) {}

#define DUMMY_VIDEO_RENDER_CRT(func) void func(video_render_color_tables_t *colortab, \
const uint8_t *src, uint8_t *trg, \
unsigned int width, const unsigned int height, \
const unsigned int xs, const unsigned int ys, \
const unsigned int xt, const unsigned int yt, \
const unsigned int pitchs, const unsigned int pitcht, \
unsigned int viewport_first_line, unsigned int viewport_last_line, \
video_render_config_t *config) {}

DUMMY_VIDEO_RENDER(render_32_2x2)
DUMMY_VIDEO_RENDER(render_08_2x2_04)
DUMMY_VIDEO_RENDER(render_16_2x2_04)
DUMMY_VIDEO_RENDER(render_24_2x2_04)
DUMMY_VIDEO_RENDER(render_32_2x2_04)
DUMMY_VIDEO_RENDER(render_08_2x4_04)
DUMMY_VIDEO_RENDER(render_16_2x4_04)
DUMMY_VIDEO_RENDER(render_24_2x4_04)
DUMMY_VIDEO_RENDER(render_32_2x4_04)
DUMMY_VIDEO_RENDER(render_32_2x4)
DUMMY_VIDEO_RENDER_SCALE2X(render_08_scale2x)
DUMMY_VIDEO_RENDER_SCALE2X(render_16_scale2x)
DUMMY_VIDEO_RENDER_SCALE2X(render_24_scale2x)
DUMMY_VIDEO_RENDER_SCALE2X(render_32_scale2x)
DUMMY_VIDEO_RENDER_CRT(render_16_2x2_crt)
DUMMY_VIDEO_RENDER_CRT(render_24_2x2_crt)
DUMMY_VIDEO_RENDER_CRT(render_32_2x2_crt)
DUMMY_VIDEO_RENDER_CRT(render_16_2x4_crt)
DUMMY_VIDEO_RENDER_CRT(render_24_2x4_crt)
DUMMY_VIDEO_RENDER_CRT(render_32_2x4_crt)
DUMMY_VIDEO_RENDER_CRT(render_16_2x2_ntsc)
DUMMY_VIDEO_RENDER_CRT(render_16_2x2_pal)
DUMMY_VIDEO_RENDER_CRT(render_24_2x2_ntsc)
DUMMY_VIDEO_RENDER_CRT(render_24_2x2_pal)
DUMMY_VIDEO_RENDER_CRT(render_32_2x2_ntsc)
DUMMY_VIDEO_RENDER_CRT(render_32_2x2_pal)
DUMMY_VIDEO_RENDER_CRT(render_32_2x2_rgbi)
DUMMY_VIDEO_RENDER_CRT(render_32_2x4_rgbi)
DUMMY_VIDEO_RENDER_CRT(render_32_2x2_pal_u)

void mousedrv_mouse_changed(void) {}

int mousedrv_get_x(void) { return 0; }
int mousedrv_get_y(void) { return 0; }
unsigned long mousedrv_get_timestamp(void) { return 0; }

void mouse_button(int bnumber, int state) {}

//void joystick() {}
int joy_arch_init(void) { return 0; }
int joy_arch_set_device(int port_idx, int new_dev) { return 0; }
int joy_arch_resources_init(void) { return 0; }

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

void vsync_do_end_of_line(void)
{
	sound_flush();
}

void vsync_do_vsync(struct video_canvas_s *c)
{
	vsync_do_vsync2(c);
	vsync_hook();
	execute_vsync_callbacks();
	kbdbuf_flush();
}

bool vsync_should_skip_frame(struct video_canvas_s *c)
{
	return c->skipFrame;
}

int zfile_fclose(FILE *stream)
{
	return fclose(stream);
}

void zfile_shutdown(void) {}

int zfile_close_action(const char *filename, zfile_action_t action,
	const char *request_string)
{
	return 0;
}

int memmap_screenshot_save(const char *drvname, const char *filename, int x_size, int y_size, uint8_t *gfx, uint8_t *palette)
{
	return 0;
}

tick_t tick_per_second(void) { return TICK_PER_SECOND; }
tick_t tick_now_delta(tick_t previous_tick) { return 1; }
void tick_sleep(tick_t ticks) { assert(!"emulation thread should not explicitly call sleep for timing"); }

int fork_coproc(int *fd_wr, int *fd_rd, char *cmd, vice_pid_t *childpid) { return -1; }
void kill_coproc(vice_pid_t pid) {}

#ifdef NDEBUG
int log_message(log_t log, const char *format, ...) { return 0; }
int log_warning(log_t log, const char *format, ...) { return 0; }
int log_error(log_t log, const char *format, ...) { return 0; }
int log_debug(const char *format, ...) { return 0; }
int log_verbose(const char *format, ...) { return 0; }
void archdep_startup_log_error(const char *format, ...) {}
void ui_error(const char *format,...) {}
int uimon_out(const char *buffer) { return 0; }
#endif

int vice_init()
{
	lib_init();

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
