CPPFLAGS += -I$(genPath) -I$(projectPath)/include
VPATH += $(projectPath)/src

ifndef configFilename
 $(error configFilename not defined)
endif

genConfigH = $(genPath)/$(configFilename)

.SUFFIXES: 
.PHONY: all main config
all : $(genConfigH) config main

# config file is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)"

config : $(genConfigH)
