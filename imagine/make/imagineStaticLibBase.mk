CPPFLAGS += -I$(genPath) -I$(projectPath)/include
VPATH += $(projectPath)/src

ifdef configFilename
 makeConfigH := 1
 genConfigH = $(genPath)/$(configFilename)
endif

.SUFFIXES: 
.PHONY: all main config
all : $(genConfigH) config main

ifdef makeConfigH
# config file is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)"
endif

config : $(genConfigH)
