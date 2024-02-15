ifndef inc_pkg_opengl
inc_pkg_opengl := 1

ifeq ($(SUBENV), pandora)
 LDLIBS += -lGLESv2 -lm
else ifeq ($(ENV), linux)
 ifeq ($(openGLAPI), gles)
  pkgConfigDeps += glesv2
 else
  pkgConfigDeps += gl
 endif
else ifeq ($(ENV), android)
 LDLIBS += -lGLESv2
else ifeq ($(ENV), ios)
 LDLIBS += -framework OpenGLES
else ifeq ($(ENV), macosx)
 LDLIBS += -framework OpenGL -framework CoreVideo
else ifeq ($(ENV), win32)
 LDLIBS += -lglew32 -lopengl32
endif

endif