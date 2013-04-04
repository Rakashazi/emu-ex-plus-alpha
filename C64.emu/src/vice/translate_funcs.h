extern int current_language_index;
extern char *current_language;

extern int translate_resources_init(void);
extern void translate_resources_shutdown(void);
extern int translate_cmdline_options_init(void);
extern void translate_arch_language_init(void);
extern char *translate_text(int en_resource);
extern int translate_res(int en_resource);
