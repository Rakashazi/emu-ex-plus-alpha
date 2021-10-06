ifndef inc_audio_pa
inc_audio_pa := 1

configDefs += CONFIG_AUDIO_PULSEAUDIO

ifeq ($(pulseAudioMainLoop), glib)
 configDefs += CONFIG_AUDIO_PULSEAUDIO_GLIB
 include $(IMAGINE_PATH)/make/package/pulseaudio-glib.mk
else
 include $(IMAGINE_PATH)/make/package/pulseaudio.mk
endif

SRC += audio/OutputStream.cc audio/pulseaudio/pulseaudio.cc

endif
