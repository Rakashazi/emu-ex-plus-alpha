ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/EmuControls.cc

NP_CORE := Core

CPPFLAGS += -DLSB_FIRST -D__cdecl= -DSysDDec=float \
-I$(projectPath)/src -I$(projectPath)/src/$(NP_CORE)/z80 \
-I$(projectPath)/src/$(NP_CORE)/TLCS-900h -I$(projectPath)/src/$(NP_CORE)

NEOPOP_SRC +=  $(NP_CORE)/z80/Z80.cc \
$(NP_CORE)/flash.cc $(NP_CORE)/gfx_scanline_colour.cc $(NP_CORE)/gfx_scanline_mono.cc \
$(NP_CORE)/gfx.cc  $(NP_CORE)/rom.cc $(NP_CORE)/sound.cc $(NP_CORE)/state.cc \
$(NP_CORE)/chunk.cc $(NP_CORE)/dma.cc \
$(NP_CORE)/bios.cc $(NP_CORE)/biosHLE.cc \
$(NP_CORE)/interrupt.cc $(NP_CORE)/mem.cc $(NP_CORE)/neopop.cc $(NP_CORE)/Z80_interface.cc \
$(NP_CORE)/TLCS-900h/TLCS900h.cc

#CPPFLAGS += -DNEOPOP_DEBUG
#NEOPOP_SRC += $(NP_CORE)/TLCS-900h/TLCS900h_disassemble_dst.cc $(NP_CORE)/TLCS-900h/TLCS900h_disassemble_extra.cc \
$(NP_CORE)/TLCS-900h/TLCS900h_disassemble_reg.cc $(NP_CORE)/TLCS-900h/TLCS900h_disassemble_src.cc \
$(NP_CORE)/TLCS-900h/TLCS900h_disassemble.cc

#$(NP_CORE)/TLCS-900h/TLCS900h_interpret_dst.cc $(NP_CORE)/TLCS-900h/TLCS900h_interpret_reg.cc \
#$(NP_CORE)/TLCS-900h/TLCS900h_interpret_single.cc $(NP_CORE)/TLCS-900h/TLCS900h_interpret_src.cc \
#$(NP_CORE)/TLCS-900h/TLCS900h_interpret.cc $(NP_CORE)/TLCS-900h/TLCS900h_registers.cc
#NEOPOP_OBJ := $(addprefix $(objDir)/,$(NEOPOP_SRC:.cc=.o))

SRC += $(NEOPOP_SRC)

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/unzip.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
