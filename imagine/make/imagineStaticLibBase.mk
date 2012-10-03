CPPFLAGS += -I$(genPath) -Isrc
VPATH += src

genConfigH = $(genPath)/config.h
genLibsMake = $(genPath)/flags.mk

.SUFFIXES: 
.PHONY: all main config
all : $(genConfigH) $(genLibsMake) main

# config.h is only built if not present, needs manual deletion to update
$(genConfigH) :
	@echo "Generating Config $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)bash $(IMAGINE_PATH)/make/writeConfig.sh $@ "$(configDefs)" "$(configInc)"

$(genLibsMake) :
	@echo "Generating Flags Makefile $@"
	@mkdir -p $(@D)
	$(PRINT_CMD)echo -e "LDLIBS += $(LDLIBS)\nCPPFLAGS += $(CPPFLAGS)" > $@

config : $(genConfigH) $(genLibsMake)

ifndef NO_LOGGER

include $(imagineSrcDir)/logger/system.mk

endif
