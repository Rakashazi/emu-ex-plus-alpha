include $(currPath)/imagineCommonTarget.mk

targetFile := $(target).a

$(targetDir)/$(targetFile) : $(OBJ)
	@echo "Archiving $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD)ar rcs $@ $^

# ld -r -o $@ $^ $(LDFLAGS)

main: $(targetDir)/$(targetFile)
