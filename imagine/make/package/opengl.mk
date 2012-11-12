ifndef inc_pkg_opengl
inc_pkg_opengl := 1

ifeq ($(ENV), linux)
 # TODO: use pkg-config
 ifdef config_gfx_openGLES
 LDLIBS += -lGLESv1_CM -lEGL -lm
 configDefs += CONFIG_GFX_OPENGL_ES
 else
 LDLIBS += -lGL
 endif
else ifeq ($(ENV), android)
 LDLIBS += -lGLESv1_CM
 ifneq ($(ARCH), x86)
  LDLIBS += -ldl
 endif
else ifeq ($(ENV), ios)
 LDLIBS += -framework OpenGLES
else ifeq ($(ENV), macosx)
 configDefs += CONFIG_GFX_OPENGL_GLEW_STATIC
 LDLIBS += -framework OpenGL -framework CoreVideo
else ifeq ($(ENV), webos)
 LDLIBS += -lGLES_CM $(webos_libm)
else ifeq ($(ENV), ps3)
 CPPFLAGS += -DPSGL
 LDLIBS += -L/usr/local/cell/target/ppu/lib/PSGL/RSX/ultra-opt -lPSGL -lm -lgcm_cmd -lgcm_sys_stub -lresc_stub -lsysutil_stub
 #\
 $(ps3CellPPULibPath)/libcgc.a $(ps3CellPPULibPath)/libsnc.a \
 $(ps3CellPPULibPath)/liblv2_stub.a
 # -lPSGLFX -lperf
endif

endif