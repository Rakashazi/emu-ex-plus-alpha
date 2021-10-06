ifndef inc_audio_opensl
inc_audio_opensl := 1

SRC += audio/OutputStream.cc audio/opensl/opensl.cc

LDLIBS += -lOpenSLES

endif
