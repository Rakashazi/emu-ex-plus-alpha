ifndef inc_base
inc_base := 1

configDefs += CONFIG_BASE_PS3

LDLIBS += -lusbd_stub -lfs_stub -lio_stub -lsysmodule_stub

SRC += base/ps3/main.cc base/ps3/main2.cc util/string/generic.cc

$(objDir)/base/ps3/main2.o : $(imagineSrcDir)/base/ps3/main2.cc
	@echo "Compiling with GCC 4.1 $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(SONY_CC) -c $< -I$(imagineSrcDir) -fno-rtti -fno-exceptions -fno-threadsafe-statics \
	-pipe -fvisibility=hidden -O2 -ffast-math -fmerge-all-constants -fomit-frame-pointer -Wall -Wextra \
	-Wno-comment -Wno-missing-field-initializers -Wno-unused -Wno-non-virtual-dtor -Wno-attributes \
	-include $(IMAGINE_PATH)/src/base/ps3/macros.h -o $@

endif
