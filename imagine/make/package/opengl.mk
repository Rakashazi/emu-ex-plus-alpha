ifndef inc_pkg_opengl
inc_pkg_opengl := 1

ifeq ($(SUBENV), pandora)
 openGLAPI := gles
 ifeq ($(openGLESVersion), 1)
  LDLIBS += -lGLES_CM -lm
 else
  LDLIBS += -lGLESv2 -lm
 endif
else ifeq ($(ENV), linux)
 ifeq ($(openGLAPI), gles)
  ifeq ($(openGLESVersion), 1)
   pkgConfigDeps += glesv1_cm
  else
   pkgConfigDeps += glesv2
  endif
 else
  pkgConfigDeps += gl
 endif
else ifeq ($(ENV), android)
 openGLAPI := gles
 ifeq ($(openGLESVersion), 1)
  LDLIBS += -lGLESv1_CM
 else
  LDLIBS += -lGLESv2
 endif
else ifeq ($(ENV), ios)
 openGLAPI := gles
 LDLIBS += -framework OpenGLES
else ifeq ($(ENV), macosx)
 LDLIBS += -framework OpenGL -framework CoreVideo
else ifeq ($(ENV), win32)
 LDLIBS += -lglew32 -lopengl32
else ifeq ($(ENV), webos)
 openGLAPI := gles
 LDLIBS += -lGLES_CM $(webos_libm)
else ifeq ($(ENV), ps3)
 CPPFLAGS += -DPSGL
 LDLIBS += -L/usr/local/cell/target/ppu/lib/PSGL/RSX/ultra-opt -lPSGL -lm -lgcm_cmd -lgcm_sys_stub -lresc_stub -lsysutil_stub
 #\
 $(ps3CellPPULibPath)/libcgc.a $(ps3CellPPULibPath)/libsnc.a \
 $(ps3CellPPULibPath)/liblv2_stub.a
 # -lPSGLFX -lperf
endif

ifeq ($(openGLAPI), gles)
 configDefs += CONFIG_GFX_OPENGL_ES
 ifndef openGLESVersion
  $(error openGLESVersion isn't defined)
 endif
 configDefs += CONFIG_GFX_OPENGL_ES_MAJOR_VERSION=$(openGLESVersion)
endif

endif