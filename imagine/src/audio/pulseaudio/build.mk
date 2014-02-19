ifndef inc_audio
inc_audio := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_PULSEAUDIO

ifeq ($(pulseAudioMainLoop), glib)
 configDefs += CONFIG_AUDIO_PULSEAUDIO_GLIB
 include $(IMAGINE_PATH)/make/package/pulseaudio-glib.mk
else
 include $(IMAGINE_PATH)/make/package/pulseaudio.mk
endif

SRC += audio/pulseaudio/pulseaudio.cc

endif
