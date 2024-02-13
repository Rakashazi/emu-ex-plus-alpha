include $(buildSysPath)/imagineCommonTarget.mk
include $(buildSysPath)/evalPkgConfigCFlags.mk

LDFLAGS += $(LDFLAGS_SYSTEM)

allConfigDefs := $(configEnable) $(configDisable) $(configInc)

ifneq ($(strip $(allConfigDefs)),)
 ifdef configFilename
  makeConfigH := 1
 endif
endif

ifdef makeConfigH
genConfigH = $(genPath)/$(configFilename)

# config file is only built if not present, needs manual deletion to update
$(genConfigH) :
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configEnable)" "$(configDisable)" "$(configInc)"

config : $(genConfigH)
endif

$(OBJ) : $(genConfigH)

targetFile := $(target).a

$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Archiving $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)rm -f $@ && $(AR) rcs $@ $^

genPkgConf = $(targetDir)/$(libName).pc

$(genPkgConf) : $(imaginePkgconfigTemplate)
	@echo "Generating pkg-config file $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)sed -e 's/NAME/$(pkgName)/' \
	-e 's:DESCRIPTION:$(pkgDescription):' \
	-e 's/VERSION/$(pkgVersion)/' \
	-e 's/REQUIRES/$(pkgConfigDeps) $(pkgConfigStaticDeps)/' \
	-e 's:CFLAGS:$(pkgCFlags):' \
	-e 's:LIBS:$(LDLIBS):' < $(imaginePkgconfigTemplate) > $@

.PHONY: pkgconfig
pkgconfig : $(genPkgConf)

main: $(targetDir)/$(targetFile) $(genPkgConf)

.PHONY: clean
clean :
	rm -f $(targetDir)/$(targetFile)
	rm -rf $(genPath)
	rm -rf $(objDir)
