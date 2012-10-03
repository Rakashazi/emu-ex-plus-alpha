ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc

include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_STDINT_H -DGAMBATTE_CONST_FB_PITCH=160 -DGAMBATTE_NO_OSD -DSysDDec=float -Isrc/libgambatte/include -Isrc/common -iquote src/libgambatte/src

GAMBATTE_SRC +=  libgambatte/src/cpu.cpp libgambatte/src/gambatte.cpp \
libgambatte/src/initstate.cpp libgambatte/src/interrupter.cpp libgambatte/src/tima.cpp \
libgambatte/src/memory.cpp libgambatte/src/mem/rtc.cpp libgambatte/src/sound.cpp \
libgambatte/src/statesaver.cpp libgambatte/src/video.cpp libgambatte/src/sound/channel1.cpp \
libgambatte/src/sound/channel2.cpp libgambatte/src/sound/channel3.cpp libgambatte/src/sound/channel4.cpp \
libgambatte/src/sound/duty_unit.cpp libgambatte/src/sound/envelope_unit.cpp libgambatte/src/sound/length_counter.cpp \
libgambatte/src/video/ly_counter.cpp libgambatte/src/video/lyc_irq.cpp \
libgambatte/src/video/next_m0_time.cpp libgambatte/src/video/ppu.cpp libgambatte/src/video/sprite_mapper.cpp \
libgambatte/src/mem/cartridge.cpp libgambatte/src/mem/memptrs.cpp libgambatte/src/interruptrequester.cpp \
libgambatte/src/file/file_zip.cpp libgambatte/src/mem/pakinfo.cpp libgambatte/src/loadres.cpp \
common/resample/src/resamplerinfo.cpp common/resample/src/makesinckernel.cpp common/resample/src/chainresampler.cpp \
common/resample/src/u48div.cpp common/resample/src/i0.cpp \
common/resample/src/kaiser50sinc.cpp common/resample/src/kaiser70sinc.cpp

SRC += $(GAMBATTE_SRC)

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

ifndef target
 target := gbcemu
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
