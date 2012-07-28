ifndef inc_audio
inc_audio := 1

configDefs += CONFIG_AUDIO CONFIG_AUDIO_OPENSL_ES

SRC +=  audio/opensl/opensl.cc

LDLIBS += -lOpenSLES

endif
