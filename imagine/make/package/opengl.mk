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
endif

endif