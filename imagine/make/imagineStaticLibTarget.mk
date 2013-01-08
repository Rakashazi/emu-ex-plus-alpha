include $(currPath)/imagineCommonTarget.mk

targetFile := $(target).a

$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Archiving $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)rm -f $@ && ar rcs $@ $^

# ld -r -o $@ $^ $(LDFLAGS)

genPkgConf = $(targetDir)/imagine.pc

$(genPkgConf) :
	@echo "Generating pkg-config file $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)cp $(imaginePkgconfigTemplate) $@

config : $(genPkgConf)

main: $(targetDir)/$(targetFile)
