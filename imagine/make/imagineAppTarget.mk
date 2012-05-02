include $(currPath)/imagineCommonTarget.mk

ifeq ($(ENV), android)
 target := lib$(android_soName).so
endif

targetFile := $(target)$(targetSuffix)$(targetExtension)

# standard target
$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)$(LD) -o $@ $^ $(LDFLAGS)
ifeq ($(ENV), iOS)
ifndef iOSNoCodesign
	@echo "Signing $@"
	$(PRINT_CMD)ldid -S $@
endif
endif

ifeq ($(ENV), ps3)

#$(targetDir)/$(target).self: $(targetDir)/$(targetFile)
#	fself.py $< $@

$(targetDir)/pkg/USRDIR/EBOOT.BIN: $(targetDir)/$(targetFile)
	make_self_npdrm $< $@ EM0000-PCEE00000_00-EXPLUSALPHATURBO

#$(targetDir)/pkg/USRDIR/EBOOT.BIN: $(targetDir)/$(targetFile)
#	wine /usr/local/cell/host-win32/bin/make_fself_npdrm $< $@

main: $(targetDir)/pkg/USRDIR/EBOOT.BIN

.PHONY: ps3-pkg

SFOXML ?= $(targetDir)/sfo.xml

$(targetDir)/pkg/PARAM.SFO : $(SFOXML)
	sfo.py -f $< $@

$(targetDir)/$(target).pkg : $(targetDir)/pkg/PARAM.SFO $(targetDir)/pkg/USRDIR/EBOOT.BIN
	pkg.py --contentid EM0000-PCEE00000_00-EXPLUSALPHATURBO $(targetDir)/pkg/ $(targetDir)/$(target).pkg
	package_finalize $(targetDir)/$(target).pkg

ps3-pkg : $(targetDir)/$(target).pkg

else

main: $(targetDir)/$(targetFile)

endif
