ifndef inc_audio_opensl
inc_audio_opensl := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_OPENSL_ES

SRC += audio/OutputStream.cc audio/opensl/opensl.cc

LDLIBS += -lOpenSLES

endif
