include $(buildSysPath)/imagineCommonTarget.mk
include $(buildSysPath)/package/stdc++.mk
include $(buildSysPath)/evalPkgConfigCFlags.mk
include $(buildSysPath)/evalPkgConfigLibs.mk

ifeq ($(ENV), android)
 target := lib$(android_soName).so
 LDFLAGS += $(LDFLAGS_SO)
endif

ifdef O_LTO
 LDFLAGS += $(COMPILE_FLAGS)
endif

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
	$(PRINT_CMD) $(LD) -o $@ $(OBJ) $(LDFLAGS) $(STDCXXLIB)
ifeq ($(ENV), ios)
 ifndef iOSNoCodesign
	@echo "Signing $@"
	$(PRINT_CMD)ldid -S $@
 endif
endif

ifeq ($(ENV), ps3)

$(targetDir)/pkg/USRDIR/EBOOT.BIN: $(targetDir)/$(targetFile)
	make_self_npdrm $< $@ $(ps3_contentid)

main: $(targetDir)/pkg/USRDIR/EBOOT.BIN

.PHONY: ps3-pkg

SFOXML ?= $(targetDir)/sfo.xml

$(targetDir)/pkg/PARAM.SFO : $(SFOXML)
	sfo.py -f $< $@

$(targetDir)/$(target).pkg : $(targetDir)/pkg/PARAM.SFO $(targetDir)/pkg/USRDIR/EBOOT.BIN
	$(PKG_NPDRM) $(targetDir)/package.conf $(targetDir)/pkg
#$(targetDir)/$(target).pkg : $(targetDir)/pkg/PARAM.SFO $(targetDir)/pkg/USRDIR/EBOOT.BIN
#	pkg.py --contentid $(ps3_contentid) $(targetDir)/pkg/ $(targetDir)/$(target).pkg
#	package_finalize $(targetDir)/$(target).pkg

ps3-pkg : $(targetDir)/$(target).pkg

else

main: $(targetDir)/$(targetFile)

endif

.PHONY: clean
clean :
	rm -f $(targetDir)/$(targetFile)
	rm -rf $(genPath)
	rm -rf $(objDir)
