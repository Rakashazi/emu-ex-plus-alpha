#include <imagine/logger/logger.h>
#include <imagine/util/builtins.h>
#include <assert.h>
#include <sys/stat.h>
#include <ctype.h>

extern "C"
{
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
	#include "viciitypes.h"
	#include "sound.h"
	#include "cartridge.h"
	#include "tape.h"
	#include "viewport.h"
}

int console_mode = 0;
int video_disabled_mode = 0;

// only needed by c128cpu.c, define here to prevent linker errors
int maincpu_stretch = 0;
CLOCK c128cpu_memory_refresh_clk;

CLINK int c64ui_init(void) { return 0; }
CLINK int c64scui_init(void) { return 0; }
CLINK int video_init_cmdline_options(void) { return 0; }
CLINK void c64ui_shutdown(void) { }

CLINK void archdep_startup_log_error(const char *format, ...)
{
	if(!logger_isEnabled())
		return;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(0, format, ap);
	va_end(ap);
}

log_t log_open(const char *id)
{
	return 0;
}

int log_close(log_t log)
{
	return 0;
}

void log_close_all(void) { }

int log_cmdline_options_init(void)
{
	return 0;
}

int log_resources_init(void)
{
	return 0;
}

void log_resources_shutdown(void) { }

int log_message(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_M, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_warning(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_W, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_error(log_t log, const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_E, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_debug(const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_D, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

int log_verbose(const char *format, ...)
{
	if(!logger_isEnabled())
		return 0;
	va_list ap;
	va_start(ap, format);
	logger_vprintf(LOG_D, format, ap);
	logger_printf(LOG_M, "\n");
	va_end(ap);
	return 0;
}

CLINK int archdep_spawn(const char *name, char **argv, char **pstdout_redir, const char *stderr_redir)
{
	bug_exit("TODO");
  return -1;
}

CLINK char *archdep_quote_parameter(const char *name)
{
	/*not needed(?) */
	return lib_stralloc(name);
}

CLINK char *archdep_tmpnam(void)
{
	bug_exit("TODO");
	return nullptr;
}

CLINK int archdep_file_is_gzip(const char *name)
{
	size_t l = strlen(name);

	if ((l < 4 || strcasecmp(name + l - 3, ".gz")) && (l < 3 || strcasecmp(name + l - 2, ".z")) && (l < 4 || toupper(name[l - 1]) != 'Z' || name[l - 4] != '.')) {
		return 0;
	}
	return 1;
}

CLINK char *archdep_make_backup_filename(const char *fname)
{
	return util_concat(fname, "~", nullptr);
}

CLINK int archdep_file_set_gzip(const char *name)
{
    return 0;
}

CLINK char *archdep_default_save_resource_file_name(void)
{
	bug_exit("TODO");
	return nullptr;
}

//CLINK FILE *archdep_open_default_log_file(void) { return stderr; }

CLINK int archdep_default_logger(const char *level_string, const char *txt) { return 0; }

CLINK void ui_error(const char *format,...)
{
	if(!logger_isEnabled())
		return;
  va_list ap;
  va_start(ap, format);
  logger_vprintf(0, format, ap);
  va_end(ap);
}

CLINK void ui_update_menus(void) { }

CLINK int ui_extend_image_dialog(void)
{
	bug_exit("TODO");
	return 0;
}

CLINK void ui_display_drive_led(int drive_number, unsigned int pwm1, unsigned int led_pwm2) { }
CLINK void ui_display_drive_track(unsigned int drive_number, unsigned int drive_base, unsigned int half_track_number) { }
CLINK void ui_display_joyport(BYTE *joyport) { }
CLINK void ui_enable_drive_status(ui_drive_enable_t state, int *drive_led_color) { }
CLINK int uicolor_alloc_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long *color_pixel,
                               BYTE *pixel_return) { return 0; }
CLINK void uicolor_free_color(unsigned int red, unsigned int green,
                               unsigned int blue, unsigned long color_pixel) { }
CLINK void uicolor_convert_color_table(unsigned int colnr, BYTE *data,
                                        long color_pixel, void *c) { }

CLINK int uimon_out(const char *buffer)
{
	logger_printf(0, "uimon_out: %s", buffer);
	return 0;
}

CLINK char *uimon_get_in(char **ppchCommandLine, const char *prompt)
{
	bug_exit("TODO");
	static char buff[] = "";
	return buff;
}

CLINK void uimon_notify_change(void) { }

CLINK console_t *uimon_window_resume(void)
{
	bug_exit("TODO");
	return nullptr;
}

CLINK void uimon_window_close(void) { }

CLINK console_t *uimon_window_open(void)
{
	bug_exit("TODO");
	return nullptr;
}

CLINK void signals_pipe_set(void)
{
	bug_exit("TODO");
}

CLINK void signals_pipe_unset(void)
{
	bug_exit("TODO");
}

CLINK void vsid_ui_display_name(const char *name)
{
//    strncpy(vsidstrings[VSID_S_TITLE], name, 40);
//    log_message(LOG_DEFAULT, "Title: %s", vsidstrings[VSID_S_TITLE]);
}

CLINK void vsid_ui_display_author(const char *author)
{
//    strncpy(vsidstrings[VSID_S_AUTHOR], author, 40);
//    log_message(LOG_DEFAULT, "Author: %s", vsidstrings[VSID_S_AUTHOR]);
}

CLINK void vsid_ui_display_copyright(const char *copyright)
{
//    strncpy(vsidstrings[VSID_S_RELEASED], copyright, 40);
//    log_message(LOG_DEFAULT, "Released: %s", vsidstrings[VSID_S_RELEASED]);
}

CLINK void vsid_ui_display_sync(int sync)
{
//    sprintf(vsidstrings[VSID_S_SYNC], "Using %s sync", sync == MACHINE_SYNC_PAL ? "PAL" : "NTSC");
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_SYNC]);
}

CLINK void vsid_ui_display_sid_model(int model)
{
//    sprintf(vsidstrings[VSID_S_MODEL], "Using %s emulation", csidmodel[model > 19 ? 7 : model]);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_MODEL]);
}

CLINK void vsid_ui_set_default_tune(int nr)
{
//    sprintf(vsidstrings[VSID_S_DEFAULT],"Default tune: %d", nr);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_DEFAULT]);
//    sdl_vsid_default_tune = nr;
}

CLINK void vsid_ui_display_tune_nr(int nr)
{
//    sprintf(vsidstrings[VSID_S_PLAYING],"Playing tune: %d", nr);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_PLAYING]);
//    sdl_vsid_current_tune = nr;
//
//    if (sdl_vsid_state & SDL_VSID_ACTIVE) {
//        sdl_vsid_state |= SDL_VSID_REPAINT;
//    }
}

CLINK void vsid_ui_display_nr_of_tunes(int count)
{
//    sprintf(vsidstrings[VSID_S_TUNES],"Number of tunes: %d", count);
//    log_message(LOG_DEFAULT, "%s", vsidstrings[VSID_S_TUNES]);
//    sdl_vsid_tunes = count;
}

CLINK void vsid_ui_display_irqtype(const char *irq) { }

CLINK void vsid_ui_display_time(unsigned int sec) { }

CLINK void vsid_ui_setdrv(char* driver_info_text) { }

CLINK char *archdep_filename_parameter(const char *name)
{
    /* nothing special(?) */
    return lib_stralloc(name);
}

CLINK int archdep_path_is_relative(const char *path)
{
    if (path == NULL) {
        return 0;
    }

    return *path != '/';
}

/* return malloc'd version of full pathname of orig_name */
CLINK int archdep_expand_path(char **return_path, const char *orig_name)
{
	logMsg("expanding path: %s", orig_name);
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

CLINK FILE *archdep_mkstemp_fd(char **filename, const char *mode)
{
	bug_exit("TODO");
	return nullptr;
}

CLINK char *archdep_default_fliplist_file_name(void)
{
	static char buff[] = "";
	return buff;
}

CLINK char *archdep_default_autostart_disk_image_file_name(void)
{
	static char buff[] = "";
	return buff;
}

CLINK char *archdep_default_resource_file_name(void)
{
	bug_exit("TODO");
	static char buff[] = "";
	return buff;
}

CLINK int archdep_mkdir(const char *pathname, int mode)
{
	return mkdir(pathname, mode);
}

CLINK int archdep_stat(const char *file_name, unsigned int *len, unsigned int *isdir)
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

CLINK void archdep_shutdown(void) { }
CLINK void uimon_set_interface(monitor_interface_t **monitor_interface_init, int count) { }
CLINK void uimon_window_suspend(void) { }
CLINK void fullscreen_resume(void) { }
CLINK void fullscreen_capability(cap_fullscreen_t *cap_fullscreen) { }
CLINK void ui_display_tape_current_image(const char *image) { }
CLINK void ui_display_drive_current_image(unsigned int drive_number, const char *image) { }
CLINK void ui_display_tape_control_status(int control) { }
CLINK void ui_display_tape_motor_status(int motor) { }
CLINK void ui_display_tape_counter(int counter) { }
CLINK void ui_display_recording(int recording_status) { }
CLINK void ui_display_playback(int playback_status, char *version) { }
CLINK void ui_display_event_time(unsigned int current, unsigned int total) { }
CLINK void ui_display_volume(int vol) { }
CLINK void ui_cmdline_show_help(unsigned int num_options, cmdline_option_ram_t *options, void *userparam) { }
CLINK void ui_set_tape_status(int tape_status) { }
CLINK char* ui_get_file(const char *format,...) { return nullptr; }
CLINK void ui_shutdown(void) { }
CLINK void ui_resources_shutdown(void) { }
CLINK ui_jam_action_t ui_jam_dialog(const char *format, ...) { return UI_JAM_NONE; }
CLINK int c64_kbd_init(void) { return 0; }
CLINK void kbd_arch_init(void) { }
CLINK void kbd_initialize_numpad_joykeys(int* joykeys) { }
CLINK int kbd_cmdline_options_init(void) { return 0; }
CLINK int kbd_resources_init(void) { return 0; }
CLINK int joystick_arch_init_resources(void) { return 0; }
CLINK void joy_arch_init_default_mapping(int joynum) { }
CLINK int joystick_init_resources(void) { return 0; }
CLINK int console_close_all(void) { return 0; }
CLINK void video_shutdown(void) { }
CLINK gfxoutputdrv_t *gfxoutput_get_driver(const char *drvname) { return nullptr; }
CLINK void gfxoutput_shutdown(void) { }
CLINK int video_arch_resources_init(void) { return 0; }
CLINK void video_arch_resources_shutdown(void) { }
CLINK char video_canvas_can_resize(video_canvas_t *canvas) { return 1; }
CLINK void video_canvas_destroy(struct video_canvas_s *canvas) { }
CLINK int ui_resources_init() { return 0; }
CLINK int gfxoutput_resources_init() { return 0; }
CLINK int signals_init(int do_core_dumps) { return 0; }
CLINK int gfxoutput_init() { return 0; }
CLINK int console_init() { return 0; }
CLINK int ui_init_finalize() { return 0; }
CLINK int gfxoutput_cmdline_options_init() { return 0; }
CLINK int ui_cmdline_options_init() { return 0; }
CLINK int sound_init_fs_device() { return 0; }
CLINK int sound_init_dump_device() { return 0; }
CLINK int sound_init_wav_device() { return 0; }
CLINK int sound_init_voc_device() { return 0; }
CLINK int sound_init_iff_device() { return 0; }
CLINK int sound_init_aiff_device() { return 0; }
CLINK int sound_init_movie_device() { return 0; }
CLINK int screenshot_init() { return 0; }
CLINK int screenshot_record() { return 0; }
CLINK int screenshot_save(const char *drvname, const char *filename, struct video_canvas_s *canvas) { return 0; }
CLINK void screenshot_prepare_reopen() { }
CLINK void screenshot_try_reopen() { }
CLINK int sysfile_init(const char *emu_id) { return 0; }
CLINK void sysfile_shutdown() { }
CLINK int sysfile_resources_init() { return 0; }
CLINK void sysfile_resources_shutdown() { }
CLINK int sysfile_cmdline_options_init() { return 0; }
CLINK int cmdline_register_options(const cmdline_option_t *c) { return 0; }
CLINK int cmdline_init() { return 0; }
CLINK int initcmdline_init() { return 0; }
CLINK void cmdline_shutdown() { }
CLINK void video_render_2x2_init() { }

#define DUMMY_VIDEO_RENDER(func) CLINK void func(video_render_color_tables_t *color_tab, \
const BYTE *src, BYTE *trg, \
unsigned int width, const unsigned int height, \
const unsigned int xs, const unsigned int ys, \
const unsigned int xt, const unsigned int yt, \
const unsigned int pitchs, const unsigned int pitcht, \
viewport_t *viewport, video_render_config_t *config) { bug_exit("called dummy video render"); }

DUMMY_VIDEO_RENDER(render_16_2x4_crt)
DUMMY_VIDEO_RENDER(render_16_1x2_crt)
DUMMY_VIDEO_RENDER(render_32_scale2x)
DUMMY_VIDEO_RENDER(render_32_1x2_04)
DUMMY_VIDEO_RENDER(render_32_2x4_04)
DUMMY_VIDEO_RENDER(render_32_2x2_04)
DUMMY_VIDEO_RENDER(render_16_2x2_crt)
DUMMY_VIDEO_RENDER(render_08_2x4_04)
DUMMY_VIDEO_RENDER(render_08_scale2x)
DUMMY_VIDEO_RENDER(render_08_1x2_04)
DUMMY_VIDEO_RENDER(render_32_2x4_crt)
DUMMY_VIDEO_RENDER(render_32_1x2_crt)
DUMMY_VIDEO_RENDER(render_24_1x2_crt)
DUMMY_VIDEO_RENDER(render_24_2x4_crt)
DUMMY_VIDEO_RENDER(render_24_scale2x)
DUMMY_VIDEO_RENDER(render_16_scale2x)
DUMMY_VIDEO_RENDER(render_24_1x2_04)
DUMMY_VIDEO_RENDER(render_16_2x4_04)
DUMMY_VIDEO_RENDER(render_16_1x2_04)
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
