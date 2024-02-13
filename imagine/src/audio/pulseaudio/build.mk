ifndef inc_audio_pa
inc_audio_pa := 1

ifeq ($(pulseAudioMainLoop), glib)
 include $(IMAGINE_PATH)/make/package/pulseaudio-glib.mk
else
 include $(IMAGINE_PATH)/make/package/pulseaudio.mk
endif

SRC += audio/OutputStream.cc audio/pulseaudio/pulseaudio.cc

endif
