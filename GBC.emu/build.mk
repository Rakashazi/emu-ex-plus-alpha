ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

emuFramework_cheats := 1
include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_STDINT_H -DGAMBATTE_CONST_FB_PITCH=160 -DGAMBATTE_NO_OSD -DSysDDec=float \
-Isrc/libgambatte/include -Isrc/common -iquote src/libgambatte/src

libgambatteSrc :=  src/cpu.cpp src/gambatte.cpp src/initstate.cpp \
src/interrupter.cpp src/tima.cpp src/memory.cpp src/mem/rtc.cpp \
src/sound.cpp src/statesaver.cpp src/video.cpp src/sound/channel1.cpp \
src/sound/channel2.cpp src/sound/channel3.cpp src/sound/channel4.cpp \
src/sound/duty_unit.cpp src/sound/envelope_unit.cpp src/sound/length_counter.cpp \
src/video/ly_counter.cpp src/video/lyc_irq.cpp src/video/next_m0_time.cpp \
src/video/ppu.cpp src/video/sprite_mapper.cpp src/mem/cartridge.cpp \
src/mem/memptrs.cpp src/interruptrequester.cpp src/file/file_zip.cpp \
src/mem/pakinfo.cpp src/loadres.cpp

libgambattePath := libgambatte
SRC += main/Main.cc main/EmuControls.cc main/Cheats.cc $(addprefix $(libgambattePath)/,$(libgambatteSrc))

gambatteCommonSrc := resample/src/resamplerinfo.cpp resample/src/makesinckernel.cpp \
resample/src/chainresampler.cpp resample/src/u48div.cpp resample/src/i0.cpp \
resample/src/kaiser50sinc.cpp resample/src/kaiser70sinc.cpp

gambatteCommonPath := common
SRC +=  $(addprefix $(gambatteCommonPath)/,$(gambatteCommonSrc))

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
