RELEASE := 1

include $(IMAGINE_PATH)/make/iOS-armv6-gcc.mk

all : fixMobilePermission

fixMobilePermission : fixMobilePermission.c
	@rm -f $@
	@echo "Compiling/Linking $<"
	$(PRINT_CMD)$(LD) $< $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
	@echo "Signing $@"
	$(PRINT_CMD)ldid -S $@
	chmod gu+s $@
