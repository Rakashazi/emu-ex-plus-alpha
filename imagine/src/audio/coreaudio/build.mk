ifndef inc_audio_ca
inc_audio_ca := 1

SRC +=  audio/OutputStream.cc audio/coreaudio/coreaudio.cc

LDLIBS += -framework AudioToolbox

endif
