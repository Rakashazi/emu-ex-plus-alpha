#ifndef VICE_TYPES_H
#error must be included as part of types.h
#endif

VICE_API int c64model_get(void);
VICE_API void c64model_set(int model);

VICE_API int c128model_get(void);
VICE_API void c128model_set(int model);

VICE_API int cart_getid_slotmain(void);

VICE_API int dtvmodel_get(void);
VICE_API void dtvmodel_set(int model);

VICE_API int cbm2model_get(void);
VICE_API void cbm2model_set(int model);

VICE_API int drive_check_type(unsigned int drive_type, unsigned int dnr);

VICE_API int petmodel_get(void);
VICE_API void petmodel_set(int model);

VICE_API int plus4model_get(void);
VICE_API void plus4model_set(int model);

VICE_API int vdrive_internal_create_format_disk_image(const char *filename, const char *diskname, unsigned int type);

VICE_API int vic20model_get(void);
VICE_API void vic20model_set(int model);

#ifndef NDEBUG
VICE_API void archdep_startup_log_error(const char *format, ...);
#endif

VICE_API char *archdep_default_rtc_file_name(void);

VICE_API const char *file_system_get_disk_name(unsigned int unit, unsigned int drive);
VICE_API int file_system_attach_disk(unsigned int unit, unsigned int drive, const char *filename);
VICE_API void file_system_detach_disk(unsigned int unit, unsigned int drive);

VICE_API int autostart_autodetect(const char *file_name,
                                const char *program_name,
                                unsigned int program_number,
                                unsigned int runmode);

VICE_API int cartridge_attach_image(int type, const char *filename);
VICE_API void cartridge_detach_image(int type);
VICE_API const char *cartridge_get_file_name(int type);

VICE_API int init_main(void);

VICE_API void interrupt_maincpu_trigger_trap(void (*trap_func)(uint16_t, void *data), void *data);

#ifndef NDEBUG
VICE_API int log_message(signed int log, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
VICE_API int log_warning(signed int log, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
VICE_API int log_error(signed int log, const char *format, ...)
    __attribute__((format(printf, 2, 3)));
VICE_API int log_debug(const char *format, ...)
    __attribute__((format(printf, 1, 2)));
VICE_API int log_verbose(const char *format, ...)
    __attribute__((format(printf, 1, 2)));
#endif

VICE_API void machine_trigger_reset(const unsigned int reset_mode);
VICE_API void machine_set_restore_key(int v);
VICE_API int machine_write_snapshot(const char *name, int save_roms,
                                  int save_disks, int even_mode);
VICE_API int machine_read_snapshot(const char *name, int even_mode);
//VICE_API void machine_shutdown(void); // TODO: not used currently
VICE_API struct drive_type_info_s *machine_drive_get_type_info_list(void);

VICE_API void maincpu_mainloop(void);

VICE_API int resources_set_int(const char *name, int value);
VICE_API int resources_set_string(const char *name, const char *value);
VICE_API int resources_get_int(const char *name, int *value_return);
VICE_API int resources_get_string(const char *name, const char **value_return);
VICE_API int resources_get_default_value(const char *name, void *value_return);

VICE_API const char *tape_get_file_name(int port);
VICE_API int tape_image_detach(unsigned int unit);
VICE_API int tape_image_attach(unsigned int unit, const char *name);

VICE_API void datasette_control(int port, int command);
VICE_API void ui_display_tape_counter(int port, int counter);

#ifndef NDEBUG
VICE_API void ui_error(const char *format, ...);
VICE_API int uimon_out(const char *buffer);
#endif

VICE_API int cbmimage_create_image(const char *name, unsigned int type);

VICE_API void keyboard_key_pressed_direct(signed long key, int mod, int pressed);
VICE_API void keyboard_key_clear(void);

VICE_API void vsync_set_warp_mode(int val);
