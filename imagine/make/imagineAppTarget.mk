include $(buildSysPath)/imagineCommonTarget.mk
include $(buildSysPath)/package/stdc++.mk
include $(buildSysPath)/evalPkgConfigCFlags.mk
include $(buildSysPath)/evalPkgConfigLibs.mk

ifeq ($(ENV), android)
 ifndef android_metadata_soName
  $(error android_metadata_soName not defined)
 endif
 target := lib$(android_metadata_soName).so
endif

LDFLAGS += $(STDCXXLIB) $(LDFLAGS_SYSTEM)

allConfigDefs := $(configEnable) $(configDisable) $(configInc)

ifneq ($(strip $(allConfigDefs)),)
 ifdef configFilename
  makeConfigH := 1
 endif
endif

ifdef makeConfigH
genConfigH = $(genPath)/config.h

# config.h is only built if not present, needs manual deletion to update
$(genConfigH) :
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configEnable)" "$(configDisable)" "$(configInc)"

config : $(genConfigH)
endif

$(OBJ) : $(genConfigH) $(genMetaH)

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
