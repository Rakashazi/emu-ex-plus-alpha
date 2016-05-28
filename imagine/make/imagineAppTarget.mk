include $(buildSysPath)/imagineCommonTarget.mk
include $(buildSysPath)/package/stdc++.mk
include $(buildSysPath)/evalPkgConfigCFlags.mk
include $(buildSysPath)/evalPkgConfigLibs.mk

ifeq ($(ENV), android)
 target := lib$(android_soName).so
endif

ifdef STDCXXINC
 CPPFLAGS += $(STDCXXINC)
endif

ifdef O_LTO
 LDFLAGS_SYSTEM += $(CFLAGS_CODEGEN)
endif

LDFLAGS += $(STDCXXLIB)

targetFile := $(target)$(targetSuffix)$(targetExtension)

linkerLibsDep := $(linkerLibs)
# TODO: add Xcode lib path to VPATH so next lines aren't needed
linkerLibsDep := $(linkerLibsDep:-lz=)
linkerLibsDep := $(linkerLibsDep:-lm=)

linkerLibsDep := $(linkerLibsDep:-lgcc=)

# standard target
$(targetDir)/$(targetFile) : $(OBJ) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkAction) $(OBJ) $(LDFLAGS)
ifeq ($(ENV), ios)
 ifndef iOSNoCodesign
	@echo "Signing $@"
	$(PRINT_CMD)ldid -S $@
 endif
endif

main: $(targetDir)/$(targetFile)

.PHONY: clean
clean :
	rm -f $(targetDir)/$(targetFile)
	rm -rf $(genPath)
	rm -rf $(objDir)
