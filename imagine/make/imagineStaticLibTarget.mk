include $(buildSysPath)/imagineCommonTarget.mk
include $(buildSysPath)/evalPkgConfigCFlags.mk

targetFile := $(target).a

$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Archiving $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)rm -f $@ && $(AR) rcs $@ $^

# ld -r -o $@ $^ $(LDFLAGS)

genPkgConf = $(targetDir)/imagine.pc

$(genPkgConf) : $(imaginePkgconfigTemplate)
	@echo "Generating pkg-config file $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)sed -e 's:PREFIX:$(pkgPrefix):' -e 's/BUILD/$(pkgBuild)/' -e 's/NAME/$(pkgName)/' \
	-e 's:DESCRIPTION:$(pkgDescription):' -e 's/VERSION/$(pkgVersion)/' \
	-e 's/REQUIRES/$(pkgConfigDeps) $(pkgConfigStaticDeps)/' -e 's:LIBS:$(LDLIBS):' < $(imaginePkgconfigTemplate) > $@

config : $(genPkgConf)

main: $(targetDir)/$(targetFile)
