ifndef buildName
 buildName := $(baseMakefileName:.mk=)
endif

ifndef genPath
 genPath := $(basePath)/build/$(buildName)/gen
endif

ifndef objDir
 objDir := $(basePath)/build/$(buildName)/obj
endif

ifndef targetDir
 targetDir := target/$(ENV)
endif

imagineSrcDir := $(IMAGINE_PATH)/src

# C
$(objDir)/%.o : %.c
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CFLAGS) -o $@

# C++
$(objDir)/%.o : %.cc
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

$(objDir)/%.o : %.cpp
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

$(objDir)/%.o : %.cxx
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Objective C
$(objDir)/%.o : %.m
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CFLAGS) -o $@

# Objective C++
$(objDir)/%.o : %.mm
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Assembly
$(objDir)/%.o : %.s
	@echo "Assembling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(AS) $< $(ASMFLAGS) -o $@
