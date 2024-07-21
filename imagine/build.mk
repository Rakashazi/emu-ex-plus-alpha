ifndef inc_main
inc_main := 1

ifdef RELEASE
 configFilename := imagine$(libNameExt)-config.h
else
 configFilename := imagine$(libNameExt)-debug-config.h
endif

include $(IMAGINE_PATH)/make/imagineStaticLibBase.mk

imagineSrcDir := $(projectPath)/src

include $(imagineSrcDir)/audio/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/fs/system.mk
include $(imagineSrcDir)/fs/ArchiveFS.mk
include $(imagineSrcDir)/io/system.mk
include $(imagineSrcDir)/io/MapIO.mk
include $(imagineSrcDir)/bluetooth/system.mk
include $(imagineSrcDir)/gui/gui.mk
include $(imagineSrcDir)/font/system.mk
include $(imagineSrcDir)/data-type/image/system.mk
include $(imagineSrcDir)/thread/system.mk
include $(imagineSrcDir)/vmem/system.mk
include $(imagineSrcDir)/logger/system.mk
include $(buildSysPath)/package/stdc++.mk

libName := imagine$(libNameExt)
ifndef RELEASE
 libName := $(libName)-debug
endif

target := lib$(libName)

targetDir := lib/$(buildName)

prefix ?= $(IMAGINE_SDK_PLATFORM_PATH)

imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine.pc
pkgName := $(libName)
pkgDescription := Game/Multimedia Engine
pkgVersion := 1.5.82
LDLIBS := -l$(libName) $(LDLIBS)
ifdef libNameExt
 pkgCFlags := -DIMAGINE_CONFIG_H=$(configFilename)
 CPPFLAGS += -DIMAGINE_CONFIG_H=$(configFilename)
endif

CFLAGS_WARN += -Werror
CFLAGS_LANG = -std=gnu23

include $(IMAGINE_PATH)/make/imagineStaticLibTarget.mk

install : main
	@echo "Installing lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)cp lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)cp lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)cp -r $(projectPath)/include/imagine $(prefix)/include/
ifdef makeConfigH
	$(PRINT_CMD)cp build/$(buildName)/gen/$(configFilename) $(prefix)/include/
endif

install-links : main
	@echo "Installing symlink lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)$(LN) -srf $(projectPath)/include/imagine $(prefix)/include/
ifdef makeConfigH
	$(PRINT_CMD)$(LN) -srf build/$(buildName)/gen/$(configFilename) $(prefix)/include/
endif

endif
