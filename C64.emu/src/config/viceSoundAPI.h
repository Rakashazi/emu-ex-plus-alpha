#ifndef VICE_SOUND_H
#error must be included as part of sound.h
#endif

VICE_API int sound_init_dummy_device(void);
VICE_API int sound_register_device(const sound_device_t *pdevice);
