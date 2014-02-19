CPPFLAGS += -I$(genPath) -I$(projectPath)/src
VPATH += $(projectPath)/src

genConfigH = $(genPath)/config.h

.SUFFIXES: 
.PHONY: all main config
all : $(genConfigH) config main

# config.h is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)"

config : $(genConfigH)

ifndef NO_LOGGER

include $(imagineSrcDir)/logger/system.mk

endif
