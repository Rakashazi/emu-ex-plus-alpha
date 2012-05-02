/*
** The OpenGL Extension Wrangler Library
** Copyright (C) 2002-2008, Milan Ikits <milan ikits[]ieee org>
** Copyright (C) 2002-2008, Marcelo E. Magallon <mmagallo[]debian org>
** Copyright (C) 2002, Lev Povalahev
** All rights reserved.
** 
** Redistribution and use in source and binary forms, with or without 
** modification, are permitted provided that the following conditions are met:
** 
** * Redistributions of source code must retain the above copyright notice, 
**   this list of conditions and the following disclaimer.
** * Redistributions in binary form must reproduce the above copyright notice, 
**   this list of conditions and the following disclaimer in the documentation 
**   and/or other materials provided with the distribution.
** * The name of the author may be used to endorse or promote products 
**   derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
** THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <GL/glew.h>
#if defined(_WIN32)
#  include <GL/wglew.h>
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)
#  include <GL/glxew.h>
#endif

/*
 * Define glewGetContext and related helper macros.
 */
#ifdef GLEW_MX
#  define glewGetContext() ctx
#  ifdef _WIN32
#    define GLEW_CONTEXT_ARG_DEF_INIT GLEWContext* ctx
#    define GLEW_CONTEXT_ARG_VAR_INIT ctx
#    define wglewGetContext() ctx
#    define WGLEW_CONTEXT_ARG_DEF_INIT WGLEWContext* ctx
#    define WGLEW_CONTEXT_ARG_DEF_LIST WGLEWContext* ctx
#  else /* _WIN32 */
#    define GLEW_CONTEXT_ARG_DEF_INIT void
#    define GLEW_CONTEXT_ARG_VAR_INIT
#    define glxewGetContext() ctx
#    define GLXEW_CONTEXT_ARG_DEF_INIT void
#    define GLXEW_CONTEXT_ARG_DEF_LIST GLXEWContext* ctx
#  endif /* _WIN32 */
#  define GLEW_CONTEXT_ARG_DEF_LIST GLEWContext* ctx
#else /* GLEW_MX */
#  define GLEW_CONTEXT_ARG_DEF_INIT void
#  define GLEW_CONTEXT_ARG_VAR_INIT
#  define GLEW_CONTEXT_ARG_DEF_LIST void
#  define WGLEW_CONTEXT_ARG_DEF_INIT void
#  define WGLEW_CONTEXT_ARG_DEF_LIST void
#  define GLXEW_CONTEXT_ARG_DEF_INIT void
#  define GLXEW_CONTEXT_ARG_DEF_LIST void
#endif /* GLEW_MX */

#if defined(__sgi) || defined (__sun) || defined(GLEW_APPLE_GLX)
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void* dlGetProcAddress (const GLubyte* name)
{
  static void* h = NULL;
  static void* gpa;

  if (h == NULL)
  {
    if ((h = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL)) == NULL) return NULL;
    gpa = dlsym(h, "glXGetProcAddress");
  }

  if (gpa != NULL)
    return ((void*(*)(const GLubyte*))gpa)(name);
  else
    return dlsym(h, (const char*)name);
}
#endif /* __sgi || __sun || GLEW_APPLE_GLX */

#if defined(__APPLE__)
#include <stdlib.h>
#include <string.h>
#include <AvailabilityMacros.h>

#ifdef MAC_OS_X_VERSION_10_3

#include <dlfcn.h>

void* NSGLGetProcAddress (const GLubyte *name)
{
  static void* image = NULL;
  if (NULL == image) 
  {
    image = dlopen("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", RTLD_LAZY);
  }
  if( !image ) return NULL;
  void* addr = dlsym(image, (const char*)name);
  if( addr ) return addr;
#ifdef GLEW_APPLE_GLX
  return dlGetProcAddress( name ); // try next for glx symbols
#else
  return NULL;
#endif
}
#else

#include <mach-o/dyld.h>

void* NSGLGetProcAddress (const GLubyte *name)
{
  static const struct mach_header* image = NULL;
  NSSymbol symbol;
  char* symbolName;
  if (NULL == image)
  {
    image = NSAddImage("/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL", NSADDIMAGE_OPTION_RETURN_ON_ERROR);
  }
  /* prepend a '_' for the Unix C symbol mangling convention */
  symbolName = malloc(strlen((const char*)name) + 2);
  strcpy(symbolName+1, (const char*)name);
  symbolName[0] = '_';
  symbol = NULL;
  /* if (NSIsSymbolNameDefined(symbolName))
	 symbol = NSLookupAndBindSymbol(symbolName); */
  symbol = image ? NSLookupSymbolInImage(image, symbolName, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND | NSLOOKUPSYMBOLINIMAGE_OPTION_RETURN_ON_ERROR) : NULL;
  free(symbolName);
  if( symbol ) return NSAddressOfSymbol(symbol);
#ifdef GLEW_APPLE_GLX
  return dlGetProcAddress( name ); // try next for glx symbols
#else
  return NULL;
#endif
}
#endif /* MAC_OS_X_VERSION_10_3 */
#endif /* __APPLE__ */

/*
 * Define glewGetProcAddress.
 */
#if defined(_WIN32)
#  define glewGetProcAddress(name) wglGetProcAddress((LPCSTR)name)
#else
#  if defined(__APPLE__)
#    define glewGetProcAddress(name) NSGLGetProcAddress(name)
#  else
#    if defined(__sgi) || defined(__sun)
#      define glewGetProcAddress(name) dlGetProcAddress(name)
#    else /* __linux */
#      define glewGetProcAddress(name) (*glXGetProcAddressARB)(name)
#    endif
#  endif
#endif

/*
 * Define GLboolean const cast.
 */
#define CONST_CAST(x) (*(GLboolean*)&x)

/*
 * GLEW, just like OpenGL or GLU, does not rely on the standard C library.
 * These functions implement the functionality required in this file.
 */
static GLuint _glewStrLen (const GLubyte* s)
{
  GLuint i=0;
  if (s == NULL) return 0;
  while (s[i] != '\0') i++;
  return i;
}

static GLuint _glewStrCLen (const GLubyte* s, GLubyte c)
{
  GLuint i=0;
  if (s == NULL) return 0;
  while (s[i] != '\0' && s[i] != c) i++;
  return (s[i] == '\0' || s[i] == c) ? i : 0;
}

static GLboolean _glewStrSame (const GLubyte* a, const GLubyte* b, GLuint n)
{
  GLuint i=0;
  if(a == NULL || b == NULL)
    return (a == NULL && b == NULL && n == 0) ? GL_TRUE : GL_FALSE;
  while (i < n && a[i] != '\0' && b[i] != '\0' && a[i] == b[i]) i++;
  return i == n ? GL_TRUE : GL_FALSE;
}

static GLboolean _glewStrSame1 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  while (*na > 0 && (**a == ' ' || **a == '\n' || **a == '\r' || **a == '\t'))
  {
    (*a)++;
    (*na)--;
  }
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if(i == nb)
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

static GLboolean _glewStrSame2 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if(i == nb)
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

static GLboolean _glewStrSame3 (GLubyte** a, GLuint* na, const GLubyte* b, GLuint nb)
{
  if(*na >= nb)
  {
    GLuint i=0;
    while (i < nb && (*a)+i != NULL && b+i != NULL && (*a)[i] == b[i]) i++;
    if (i == nb && (*na == nb || (*a)[i] == ' ' || (*a)[i] == '\n' || (*a)[i] == '\r' || (*a)[i] == '\t'))
    {
      *a = *a + nb;
      *na = *na - nb;
      return GL_TRUE;
    }
  }
  return GL_FALSE;
}

/*
 * Search for name in the extensions string. Use of strstr()
 * is not sufficient because extension names can be prefixes of
 * other extension names. Could use strtok() but the constant
 * string returned by glGetString might be in read-only memory.
 */
static GLboolean _glewSearchExtension (const char* name, const GLubyte *start, const GLubyte *end)
{
  const GLubyte* p;
  GLuint len = _glewStrLen((const GLubyte*)name);
  p = start;
  while (p < end)
  {
    GLuint n = _glewStrCLen(p, ' ');
    if (len == n && _glewStrSame((const GLubyte*)name, p, n)) return GL_TRUE;
    p += n+1;
  }
  return GL_FALSE;
}

#if !defined(_WIN32) || !defined(GLEW_MX)

PFNGLCOPYTEXSUBIMAGE3DPROC __glewCopyTexSubImage3D = NULL;
PFNGLDRAWRANGEELEMENTSPROC __glewDrawRangeElements = NULL;
PFNGLTEXIMAGE3DPROC __glewTexImage3D = NULL;
PFNGLTEXSUBIMAGE3DPROC __glewTexSubImage3D = NULL;

PFNGLACTIVETEXTUREPROC __glewActiveTexture = NULL;
PFNGLCLIENTACTIVETEXTUREPROC __glewClientActiveTexture = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC __glewCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC __glewCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC __glewCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC __glewCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC __glewCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC __glewCompressedTexSubImage3D = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC __glewGetCompressedTexImage = NULL;
PFNGLLOADTRANSPOSEMATRIXDPROC __glewLoadTransposeMatrixd = NULL;
PFNGLLOADTRANSPOSEMATRIXFPROC __glewLoadTransposeMatrixf = NULL;
PFNGLMULTTRANSPOSEMATRIXDPROC __glewMultTransposeMatrixd = NULL;
PFNGLMULTTRANSPOSEMATRIXFPROC __glewMultTransposeMatrixf = NULL;
PFNGLMULTITEXCOORD1DPROC __glewMultiTexCoord1d = NULL;
PFNGLMULTITEXCOORD1DVPROC __glewMultiTexCoord1dv = NULL;
PFNGLMULTITEXCOORD1FPROC __glewMultiTexCoord1f = NULL;
PFNGLMULTITEXCOORD1FVPROC __glewMultiTexCoord1fv = NULL;
PFNGLMULTITEXCOORD1IPROC __glewMultiTexCoord1i = NULL;
PFNGLMULTITEXCOORD1IVPROC __glewMultiTexCoord1iv = NULL;
PFNGLMULTITEXCOORD1SPROC __glewMultiTexCoord1s = NULL;
PFNGLMULTITEXCOORD1SVPROC __glewMultiTexCoord1sv = NULL;
PFNGLMULTITEXCOORD2DPROC __glewMultiTexCoord2d = NULL;
PFNGLMULTITEXCOORD2DVPROC __glewMultiTexCoord2dv = NULL;
PFNGLMULTITEXCOORD2FPROC __glewMultiTexCoord2f = NULL;
PFNGLMULTITEXCOORD2FVPROC __glewMultiTexCoord2fv = NULL;
PFNGLMULTITEXCOORD2IPROC __glewMultiTexCoord2i = NULL;
PFNGLMULTITEXCOORD2IVPROC __glewMultiTexCoord2iv = NULL;
PFNGLMULTITEXCOORD2SPROC __glewMultiTexCoord2s = NULL;
PFNGLMULTITEXCOORD2SVPROC __glewMultiTexCoord2sv = NULL;
PFNGLMULTITEXCOORD3DPROC __glewMultiTexCoord3d = NULL;
PFNGLMULTITEXCOORD3DVPROC __glewMultiTexCoord3dv = NULL;
PFNGLMULTITEXCOORD3FPROC __glewMultiTexCoord3f = NULL;
PFNGLMULTITEXCOORD3FVPROC __glewMultiTexCoord3fv = NULL;
PFNGLMULTITEXCOORD3IPROC __glewMultiTexCoord3i = NULL;
PFNGLMULTITEXCOORD3IVPROC __glewMultiTexCoord3iv = NULL;
PFNGLMULTITEXCOORD3SPROC __glewMultiTexCoord3s = NULL;
PFNGLMULTITEXCOORD3SVPROC __glewMultiTexCoord3sv = NULL;
PFNGLMULTITEXCOORD4DPROC __glewMultiTexCoord4d = NULL;
PFNGLMULTITEXCOORD4DVPROC __glewMultiTexCoord4dv = NULL;
PFNGLMULTITEXCOORD4FPROC __glewMultiTexCoord4f = NULL;
PFNGLMULTITEXCOORD4FVPROC __glewMultiTexCoord4fv = NULL;
PFNGLMULTITEXCOORD4IPROC __glewMultiTexCoord4i = NULL;
PFNGLMULTITEXCOORD4IVPROC __glewMultiTexCoord4iv = NULL;
PFNGLMULTITEXCOORD4SPROC __glewMultiTexCoord4s = NULL;
PFNGLMULTITEXCOORD4SVPROC __glewMultiTexCoord4sv = NULL;
PFNGLSAMPLECOVERAGEPROC __glewSampleCoverage = NULL;

PFNGLBLENDCOLORPROC __glewBlendColor = NULL;
PFNGLBLENDEQUATIONPROC __glewBlendEquation = NULL;
PFNGLBLENDFUNCSEPARATEPROC __glewBlendFuncSeparate = NULL;
PFNGLFOGCOORDPOINTERPROC __glewFogCoordPointer = NULL;
PFNGLFOGCOORDDPROC __glewFogCoordd = NULL;
PFNGLFOGCOORDDVPROC __glewFogCoorddv = NULL;
PFNGLFOGCOORDFPROC __glewFogCoordf = NULL;
PFNGLFOGCOORDFVPROC __glewFogCoordfv = NULL;
PFNGLMULTIDRAWARRAYSPROC __glewMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC __glewMultiDrawElements = NULL;
PFNGLPOINTPARAMETERFPROC __glewPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC __glewPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC __glewPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC __glewPointParameteriv = NULL;
PFNGLSECONDARYCOLOR3BPROC __glewSecondaryColor3b = NULL;
PFNGLSECONDARYCOLOR3BVPROC __glewSecondaryColor3bv = NULL;
PFNGLSECONDARYCOLOR3DPROC __glewSecondaryColor3d = NULL;
PFNGLSECONDARYCOLOR3DVPROC __glewSecondaryColor3dv = NULL;
PFNGLSECONDARYCOLOR3FPROC __glewSecondaryColor3f = NULL;
PFNGLSECONDARYCOLOR3FVPROC __glewSecondaryColor3fv = NULL;
PFNGLSECONDARYCOLOR3IPROC __glewSecondaryColor3i = NULL;
PFNGLSECONDARYCOLOR3IVPROC __glewSecondaryColor3iv = NULL;
PFNGLSECONDARYCOLOR3SPROC __glewSecondaryColor3s = NULL;
PFNGLSECONDARYCOLOR3SVPROC __glewSecondaryColor3sv = NULL;
PFNGLSECONDARYCOLOR3UBPROC __glewSecondaryColor3ub = NULL;
PFNGLSECONDARYCOLOR3UBVPROC __glewSecondaryColor3ubv = NULL;
PFNGLSECONDARYCOLOR3UIPROC __glewSecondaryColor3ui = NULL;
PFNGLSECONDARYCOLOR3UIVPROC __glewSecondaryColor3uiv = NULL;
PFNGLSECONDARYCOLOR3USPROC __glewSecondaryColor3us = NULL;
PFNGLSECONDARYCOLOR3USVPROC __glewSecondaryColor3usv = NULL;
PFNGLSECONDARYCOLORPOINTERPROC __glewSecondaryColorPointer = NULL;
PFNGLWINDOWPOS2DPROC __glewWindowPos2d = NULL;
PFNGLWINDOWPOS2DVPROC __glewWindowPos2dv = NULL;
PFNGLWINDOWPOS2FPROC __glewWindowPos2f = NULL;
PFNGLWINDOWPOS2FVPROC __glewWindowPos2fv = NULL;
PFNGLWINDOWPOS2IPROC __glewWindowPos2i = NULL;
PFNGLWINDOWPOS2IVPROC __glewWindowPos2iv = NULL;
PFNGLWINDOWPOS2SPROC __glewWindowPos2s = NULL;
PFNGLWINDOWPOS2SVPROC __glewWindowPos2sv = NULL;
PFNGLWINDOWPOS3DPROC __glewWindowPos3d = NULL;
PFNGLWINDOWPOS3DVPROC __glewWindowPos3dv = NULL;
PFNGLWINDOWPOS3FPROC __glewWindowPos3f = NULL;
PFNGLWINDOWPOS3FVPROC __glewWindowPos3fv = NULL;
PFNGLWINDOWPOS3IPROC __glewWindowPos3i = NULL;
PFNGLWINDOWPOS3IVPROC __glewWindowPos3iv = NULL;
PFNGLWINDOWPOS3SPROC __glewWindowPos3s = NULL;
PFNGLWINDOWPOS3SVPROC __glewWindowPos3sv = NULL;

PFNGLBEGINQUERYPROC __glewBeginQuery = NULL;
PFNGLBINDBUFFERPROC __glewBindBuffer = NULL;
PFNGLBUFFERDATAPROC __glewBufferData = NULL;
PFNGLBUFFERSUBDATAPROC __glewBufferSubData = NULL;
PFNGLDELETEBUFFERSPROC __glewDeleteBuffers = NULL;
PFNGLDELETEQUERIESPROC __glewDeleteQueries = NULL;
PFNGLENDQUERYPROC __glewEndQuery = NULL;
PFNGLGENBUFFERSPROC __glewGenBuffers = NULL;
PFNGLGENQUERIESPROC __glewGenQueries = NULL;
PFNGLGETBUFFERPARAMETERIVPROC __glewGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC __glewGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC __glewGetBufferSubData = NULL;
PFNGLGETQUERYOBJECTIVPROC __glewGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUIVPROC __glewGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC __glewGetQueryiv = NULL;
PFNGLISBUFFERPROC __glewIsBuffer = NULL;
PFNGLISQUERYPROC __glewIsQuery = NULL;
PFNGLMAPBUFFERPROC __glewMapBuffer = NULL;
PFNGLUNMAPBUFFERPROC __glewUnmapBuffer = NULL;

PFNGLATTACHSHADERPROC __glewAttachShader = NULL;
PFNGLBINDATTRIBLOCATIONPROC __glewBindAttribLocation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC __glewBlendEquationSeparate = NULL;
PFNGLCOMPILESHADERPROC __glewCompileShader = NULL;
PFNGLCREATEPROGRAMPROC __glewCreateProgram = NULL;
PFNGLCREATESHADERPROC __glewCreateShader = NULL;
PFNGLDELETEPROGRAMPROC __glewDeleteProgram = NULL;
PFNGLDELETESHADERPROC __glewDeleteShader = NULL;
PFNGLDETACHSHADERPROC __glewDetachShader = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC __glewDisableVertexAttribArray = NULL;
PFNGLDRAWBUFFERSPROC __glewDrawBuffers = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC __glewEnableVertexAttribArray = NULL;
PFNGLGETACTIVEATTRIBPROC __glewGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC __glewGetActiveUniform = NULL;
PFNGLGETATTACHEDSHADERSPROC __glewGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC __glewGetAttribLocation = NULL;
PFNGLGETPROGRAMINFOLOGPROC __glewGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC __glewGetProgramiv = NULL;
PFNGLGETSHADERINFOLOGPROC __glewGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC __glewGetShaderSource = NULL;
PFNGLGETSHADERIVPROC __glewGetShaderiv = NULL;
PFNGLGETUNIFORMLOCATIONPROC __glewGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC __glewGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC __glewGetUniformiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC __glewGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC __glewGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC __glewGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC __glewGetVertexAttribiv = NULL;
PFNGLISPROGRAMPROC __glewIsProgram = NULL;
PFNGLISSHADERPROC __glewIsShader = NULL;
PFNGLLINKPROGRAMPROC __glewLinkProgram = NULL;
PFNGLSHADERSOURCEPROC __glewShaderSource = NULL;
PFNGLSTENCILFUNCSEPARATEPROC __glewStencilFuncSeparate = NULL;
PFNGLSTENCILMASKSEPARATEPROC __glewStencilMaskSeparate = NULL;
PFNGLSTENCILOPSEPARATEPROC __glewStencilOpSeparate = NULL;
PFNGLUNIFORM1FPROC __glewUniform1f = NULL;
PFNGLUNIFORM1FVPROC __glewUniform1fv = NULL;
PFNGLUNIFORM1IPROC __glewUniform1i = NULL;
PFNGLUNIFORM1IVPROC __glewUniform1iv = NULL;
PFNGLUNIFORM2FPROC __glewUniform2f = NULL;
PFNGLUNIFORM2FVPROC __glewUniform2fv = NULL;
PFNGLUNIFORM2IPROC __glewUniform2i = NULL;
PFNGLUNIFORM2IVPROC __glewUniform2iv = NULL;
PFNGLUNIFORM3FPROC __glewUniform3f = NULL;
PFNGLUNIFORM3FVPROC __glewUniform3fv = NULL;
PFNGLUNIFORM3IPROC __glewUniform3i = NULL;
PFNGLUNIFORM3IVPROC __glewUniform3iv = NULL;
PFNGLUNIFORM4FPROC __glewUniform4f = NULL;
PFNGLUNIFORM4FVPROC __glewUniform4fv = NULL;
PFNGLUNIFORM4IPROC __glewUniform4i = NULL;
PFNGLUNIFORM4IVPROC __glewUniform4iv = NULL;
PFNGLUNIFORMMATRIX2FVPROC __glewUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC __glewUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC __glewUniformMatrix4fv = NULL;
PFNGLUSEPROGRAMPROC __glewUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC __glewValidateProgram = NULL;
PFNGLVERTEXATTRIB1DPROC __glewVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC __glewVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC __glewVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC __glewVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC __glewVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC __glewVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC __glewVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC __glewVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC __glewVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC __glewVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC __glewVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC __glewVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC __glewVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC __glewVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC __glewVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC __glewVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC __glewVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC __glewVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC __glewVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC __glewVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC __glewVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC __glewVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC __glewVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC __glewVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC __glewVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC __glewVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC __glewVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC __glewVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC __glewVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC __glewVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC __glewVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC __glewVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC __glewVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC __glewVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC __glewVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC __glewVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC __glewVertexAttribPointer = NULL;

PFNGLUNIFORMMATRIX2X3FVPROC __glewUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC __glewUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC __glewUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC __glewUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC __glewUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC __glewUniformMatrix4x3fv = NULL;

PFNGLBEGINCONDITIONALRENDERPROC __glewBeginConditionalRender = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC __glewBeginTransformFeedback = NULL;
PFNGLBINDFRAGDATALOCATIONPROC __glewBindFragDataLocation = NULL;
PFNGLCLAMPCOLORPROC __glewClampColor = NULL;
PFNGLCLEARBUFFERFIPROC __glewClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC __glewClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC __glewClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC __glewClearBufferuiv = NULL;
PFNGLCOLORMASKIPROC __glewColorMaski = NULL;
PFNGLDISABLEIPROC __glewDisablei = NULL;
PFNGLENABLEIPROC __glewEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC __glewEndConditionalRender = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC __glewEndTransformFeedback = NULL;
PFNGLGETBOOLEANI_VPROC __glewGetBooleani_v = NULL;
PFNGLGETFRAGDATALOCATIONPROC __glewGetFragDataLocation = NULL;
PFNGLGETSTRINGIPROC __glewGetStringi = NULL;
PFNGLGETTEXPARAMETERIIVPROC __glewGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC __glewGetTexParameterIuiv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC __glewGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMUIVPROC __glewGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC __glewGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC __glewGetVertexAttribIuiv = NULL;
PFNGLISENABLEDIPROC __glewIsEnabledi = NULL;
PFNGLTEXPARAMETERIIVPROC __glewTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC __glewTexParameterIuiv = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC __glewTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1UIPROC __glewUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC __glewUniform1uiv = NULL;
PFNGLUNIFORM2UIPROC __glewUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC __glewUniform2uiv = NULL;
PFNGLUNIFORM3UIPROC __glewUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC __glewUniform3uiv = NULL;
PFNGLUNIFORM4UIPROC __glewUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC __glewUniform4uiv = NULL;
PFNGLVERTEXATTRIBI1IPROC __glewVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC __glewVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC __glewVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC __glewVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC __glewVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC __glewVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC __glewVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC __glewVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC __glewVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC __glewVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC __glewVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC __glewVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC __glewVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC __glewVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC __glewVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC __glewVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC __glewVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC __glewVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC __glewVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC __glewVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC __glewVertexAttribIPointer = NULL;

PFNGLDRAWARRAYSINSTANCEDPROC __glewDrawArraysInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC __glewDrawElementsInstanced = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC __glewPrimitiveRestartIndex = NULL;
PFNGLTEXBUFFERPROC __glewTexBuffer = NULL;

PFNGLFRAMEBUFFERTEXTUREPROC __glewFramebufferTexture = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC __glewGetBufferParameteri64v = NULL;
PFNGLGETINTEGER64I_VPROC __glewGetInteger64i_v = NULL;

PFNGLVERTEXATTRIBDIVISORPROC __glewVertexAttribDivisor = NULL;

PFNGLBLENDEQUATIONSEPARATEIPROC __glewBlendEquationSeparatei = NULL;
PFNGLBLENDEQUATIONIPROC __glewBlendEquationi = NULL;
PFNGLBLENDFUNCSEPARATEIPROC __glewBlendFuncSeparatei = NULL;
PFNGLBLENDFUNCIPROC __glewBlendFunci = NULL;
PFNGLMINSAMPLESHADINGPROC __glewMinSampleShading = NULL;

PFNGLCOLORSUBTABLEPROC __glewColorSubTable = NULL;
PFNGLCOLORTABLEPROC __glewColorTable = NULL;
PFNGLCOLORTABLEPARAMETERFVPROC __glewColorTableParameterfv = NULL;
PFNGLCOLORTABLEPARAMETERIVPROC __glewColorTableParameteriv = NULL;
PFNGLCONVOLUTIONFILTER1DPROC __glewConvolutionFilter1D = NULL;
PFNGLCONVOLUTIONFILTER2DPROC __glewConvolutionFilter2D = NULL;
PFNGLCONVOLUTIONPARAMETERFPROC __glewConvolutionParameterf = NULL;
PFNGLCONVOLUTIONPARAMETERFVPROC __glewConvolutionParameterfv = NULL;
PFNGLCONVOLUTIONPARAMETERIPROC __glewConvolutionParameteri = NULL;
PFNGLCONVOLUTIONPARAMETERIVPROC __glewConvolutionParameteriv = NULL;
PFNGLCOPYCOLORSUBTABLEPROC __glewCopyColorSubTable = NULL;
PFNGLCOPYCOLORTABLEPROC __glewCopyColorTable = NULL;
PFNGLCOPYCONVOLUTIONFILTER1DPROC __glewCopyConvolutionFilter1D = NULL;
PFNGLCOPYCONVOLUTIONFILTER2DPROC __glewCopyConvolutionFilter2D = NULL;
PFNGLGETCOLORTABLEPROC __glewGetColorTable = NULL;
PFNGLGETCOLORTABLEPARAMETERFVPROC __glewGetColorTableParameterfv = NULL;
PFNGLGETCOLORTABLEPARAMETERIVPROC __glewGetColorTableParameteriv = NULL;
PFNGLGETCONVOLUTIONFILTERPROC __glewGetConvolutionFilter = NULL;
PFNGLGETCONVOLUTIONPARAMETERFVPROC __glewGetConvolutionParameterfv = NULL;
PFNGLGETCONVOLUTIONPARAMETERIVPROC __glewGetConvolutionParameteriv = NULL;
PFNGLGETHISTOGRAMPROC __glewGetHistogram = NULL;
PFNGLGETHISTOGRAMPARAMETERFVPROC __glewGetHistogramParameterfv = NULL;
PFNGLGETHISTOGRAMPARAMETERIVPROC __glewGetHistogramParameteriv = NULL;
PFNGLGETMINMAXPROC __glewGetMinmax = NULL;
PFNGLGETMINMAXPARAMETERFVPROC __glewGetMinmaxParameterfv = NULL;
PFNGLGETMINMAXPARAMETERIVPROC __glewGetMinmaxParameteriv = NULL;
PFNGLGETSEPARABLEFILTERPROC __glewGetSeparableFilter = NULL;
PFNGLHISTOGRAMPROC __glewHistogram = NULL;
PFNGLMINMAXPROC __glewMinmax = NULL;
PFNGLRESETHISTOGRAMPROC __glewResetHistogram = NULL;
PFNGLRESETMINMAXPROC __glewResetMinmax = NULL;
PFNGLSEPARABLEFILTER2DPROC __glewSeparableFilter2D = NULL;

PFNGLSAMPLECOVERAGEARBPROC __glewSampleCoverageARB = NULL;

PFNGLBINDFRAMEBUFFEREXTPROC __glewBindFramebufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC __glewBindRenderbufferEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC __glewCheckFramebufferStatusEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC __glewDeleteFramebuffersEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC __glewDeleteRenderbuffersEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC __glewFramebufferRenderbufferEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC __glewFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC __glewFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC __glewFramebufferTexture3DEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC __glewGenFramebuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC __glewGenRenderbuffersEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC __glewGenerateMipmapEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC __glewGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC __glewGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC __glewIsFramebufferEXT = NULL;
PFNGLISRENDERBUFFEREXTPROC __glewIsRenderbufferEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC __glewRenderbufferStorageEXT = NULL;

#endif /* !WIN32 || !GLEW_MX */

#if !defined(GLEW_MX)

GLboolean __GLEW_VERSION_1_1 = GL_FALSE;
GLboolean __GLEW_VERSION_1_2 = GL_FALSE;
GLboolean __GLEW_VERSION_1_2_1 = GL_FALSE;
GLboolean __GLEW_VERSION_1_3 = GL_FALSE;
GLboolean __GLEW_VERSION_1_4 = GL_FALSE;
GLboolean __GLEW_VERSION_1_5 = GL_FALSE;
GLboolean __GLEW_VERSION_2_0 = GL_FALSE;
GLboolean __GLEW_VERSION_2_1 = GL_FALSE;
GLboolean __GLEW_VERSION_3_0 = GL_FALSE;
GLboolean __GLEW_VERSION_3_1 = GL_FALSE;
GLboolean __GLEW_VERSION_3_2 = GL_FALSE;
GLboolean __GLEW_VERSION_3_3 = GL_FALSE;
GLboolean __GLEW_VERSION_4_0 = GL_FALSE;
GLboolean __GLEW_VERSION_4_1 = GL_FALSE;
GLboolean __GLEW_VERSION_4_2 = GL_FALSE;
GLboolean __GLEW_ARB_imaging = GL_FALSE;
GLboolean __GLEW_ARB_multisample = GL_FALSE;
GLboolean __GLEW_ARB_pixel_buffer_object = GL_FALSE;
GLboolean __GLEW_ARB_texture_non_power_of_two = GL_FALSE;
GLboolean __GLEW_EXT_framebuffer_object = GL_FALSE;
GLboolean __GLEW_EXT_texture_filter_anisotropic = GL_FALSE;
GLboolean __GLEW_NV_multisample_filter_hint = GL_FALSE;
GLboolean __GLEW_SGIS_generate_mipmap = GL_FALSE;

#endif /* !GLEW_MX */

#ifdef GL_VERSION_1_2

static GLboolean _glewInit_GL_VERSION_1_2 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glCopyTexSubImage3D = (PFNGLCOPYTEXSUBIMAGE3DPROC)glewGetProcAddress((const GLubyte*)"glCopyTexSubImage3D")) == NULL) || r;
  r = ((glDrawRangeElements = (PFNGLDRAWRANGEELEMENTSPROC)glewGetProcAddress((const GLubyte*)"glDrawRangeElements")) == NULL) || r;
  r = ((glTexImage3D = (PFNGLTEXIMAGE3DPROC)glewGetProcAddress((const GLubyte*)"glTexImage3D")) == NULL) || r;
  r = ((glTexSubImage3D = (PFNGLTEXSUBIMAGE3DPROC)glewGetProcAddress((const GLubyte*)"glTexSubImage3D")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_1_2 */

#ifdef GL_VERSION_1_2_1

#endif /* GL_VERSION_1_2_1 */

#ifdef GL_VERSION_1_3

static GLboolean _glewInit_GL_VERSION_1_3 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glActiveTexture = (PFNGLACTIVETEXTUREPROC)glewGetProcAddress((const GLubyte*)"glActiveTexture")) == NULL) || r;
  r = ((glClientActiveTexture = (PFNGLCLIENTACTIVETEXTUREPROC)glewGetProcAddress((const GLubyte*)"glClientActiveTexture")) == NULL) || r;
  r = ((glCompressedTexImage1D = (PFNGLCOMPRESSEDTEXIMAGE1DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexImage1D")) == NULL) || r;
  r = ((glCompressedTexImage2D = (PFNGLCOMPRESSEDTEXIMAGE2DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexImage2D")) == NULL) || r;
  r = ((glCompressedTexImage3D = (PFNGLCOMPRESSEDTEXIMAGE3DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexImage3D")) == NULL) || r;
  r = ((glCompressedTexSubImage1D = (PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexSubImage1D")) == NULL) || r;
  r = ((glCompressedTexSubImage2D = (PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexSubImage2D")) == NULL) || r;
  r = ((glCompressedTexSubImage3D = (PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC)glewGetProcAddress((const GLubyte*)"glCompressedTexSubImage3D")) == NULL) || r;
  r = ((glGetCompressedTexImage = (PFNGLGETCOMPRESSEDTEXIMAGEPROC)glewGetProcAddress((const GLubyte*)"glGetCompressedTexImage")) == NULL) || r;
  r = ((glLoadTransposeMatrixd = (PFNGLLOADTRANSPOSEMATRIXDPROC)glewGetProcAddress((const GLubyte*)"glLoadTransposeMatrixd")) == NULL) || r;
  r = ((glLoadTransposeMatrixf = (PFNGLLOADTRANSPOSEMATRIXFPROC)glewGetProcAddress((const GLubyte*)"glLoadTransposeMatrixf")) == NULL) || r;
  r = ((glMultTransposeMatrixd = (PFNGLMULTTRANSPOSEMATRIXDPROC)glewGetProcAddress((const GLubyte*)"glMultTransposeMatrixd")) == NULL) || r;
  r = ((glMultTransposeMatrixf = (PFNGLMULTTRANSPOSEMATRIXFPROC)glewGetProcAddress((const GLubyte*)"glMultTransposeMatrixf")) == NULL) || r;
  r = ((glMultiTexCoord1d = (PFNGLMULTITEXCOORD1DPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1d")) == NULL) || r;
  r = ((glMultiTexCoord1dv = (PFNGLMULTITEXCOORD1DVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1dv")) == NULL) || r;
  r = ((glMultiTexCoord1f = (PFNGLMULTITEXCOORD1FPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1f")) == NULL) || r;
  r = ((glMultiTexCoord1fv = (PFNGLMULTITEXCOORD1FVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1fv")) == NULL) || r;
  r = ((glMultiTexCoord1i = (PFNGLMULTITEXCOORD1IPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1i")) == NULL) || r;
  r = ((glMultiTexCoord1iv = (PFNGLMULTITEXCOORD1IVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1iv")) == NULL) || r;
  r = ((glMultiTexCoord1s = (PFNGLMULTITEXCOORD1SPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1s")) == NULL) || r;
  r = ((glMultiTexCoord1sv = (PFNGLMULTITEXCOORD1SVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord1sv")) == NULL) || r;
  r = ((glMultiTexCoord2d = (PFNGLMULTITEXCOORD2DPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2d")) == NULL) || r;
  r = ((glMultiTexCoord2dv = (PFNGLMULTITEXCOORD2DVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2dv")) == NULL) || r;
  r = ((glMultiTexCoord2f = (PFNGLMULTITEXCOORD2FPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2f")) == NULL) || r;
  r = ((glMultiTexCoord2fv = (PFNGLMULTITEXCOORD2FVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2fv")) == NULL) || r;
  r = ((glMultiTexCoord2i = (PFNGLMULTITEXCOORD2IPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2i")) == NULL) || r;
  r = ((glMultiTexCoord2iv = (PFNGLMULTITEXCOORD2IVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2iv")) == NULL) || r;
  r = ((glMultiTexCoord2s = (PFNGLMULTITEXCOORD2SPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2s")) == NULL) || r;
  r = ((glMultiTexCoord2sv = (PFNGLMULTITEXCOORD2SVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord2sv")) == NULL) || r;
  r = ((glMultiTexCoord3d = (PFNGLMULTITEXCOORD3DPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3d")) == NULL) || r;
  r = ((glMultiTexCoord3dv = (PFNGLMULTITEXCOORD3DVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3dv")) == NULL) || r;
  r = ((glMultiTexCoord3f = (PFNGLMULTITEXCOORD3FPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3f")) == NULL) || r;
  r = ((glMultiTexCoord3fv = (PFNGLMULTITEXCOORD3FVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3fv")) == NULL) || r;
  r = ((glMultiTexCoord3i = (PFNGLMULTITEXCOORD3IPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3i")) == NULL) || r;
  r = ((glMultiTexCoord3iv = (PFNGLMULTITEXCOORD3IVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3iv")) == NULL) || r;
  r = ((glMultiTexCoord3s = (PFNGLMULTITEXCOORD3SPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3s")) == NULL) || r;
  r = ((glMultiTexCoord3sv = (PFNGLMULTITEXCOORD3SVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord3sv")) == NULL) || r;
  r = ((glMultiTexCoord4d = (PFNGLMULTITEXCOORD4DPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4d")) == NULL) || r;
  r = ((glMultiTexCoord4dv = (PFNGLMULTITEXCOORD4DVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4dv")) == NULL) || r;
  r = ((glMultiTexCoord4f = (PFNGLMULTITEXCOORD4FPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4f")) == NULL) || r;
  r = ((glMultiTexCoord4fv = (PFNGLMULTITEXCOORD4FVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4fv")) == NULL) || r;
  r = ((glMultiTexCoord4i = (PFNGLMULTITEXCOORD4IPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4i")) == NULL) || r;
  r = ((glMultiTexCoord4iv = (PFNGLMULTITEXCOORD4IVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4iv")) == NULL) || r;
  r = ((glMultiTexCoord4s = (PFNGLMULTITEXCOORD4SPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4s")) == NULL) || r;
  r = ((glMultiTexCoord4sv = (PFNGLMULTITEXCOORD4SVPROC)glewGetProcAddress((const GLubyte*)"glMultiTexCoord4sv")) == NULL) || r;
  r = ((glSampleCoverage = (PFNGLSAMPLECOVERAGEPROC)glewGetProcAddress((const GLubyte*)"glSampleCoverage")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_1_3 */

#ifdef GL_VERSION_1_4

static GLboolean _glewInit_GL_VERSION_1_4 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBlendColor = (PFNGLBLENDCOLORPROC)glewGetProcAddress((const GLubyte*)"glBlendColor")) == NULL) || r;
  r = ((glBlendEquation = (PFNGLBLENDEQUATIONPROC)glewGetProcAddress((const GLubyte*)"glBlendEquation")) == NULL) || r;
  r = ((glBlendFuncSeparate = (PFNGLBLENDFUNCSEPARATEPROC)glewGetProcAddress((const GLubyte*)"glBlendFuncSeparate")) == NULL) || r;
  r = ((glFogCoordPointer = (PFNGLFOGCOORDPOINTERPROC)glewGetProcAddress((const GLubyte*)"glFogCoordPointer")) == NULL) || r;
  r = ((glFogCoordd = (PFNGLFOGCOORDDPROC)glewGetProcAddress((const GLubyte*)"glFogCoordd")) == NULL) || r;
  r = ((glFogCoorddv = (PFNGLFOGCOORDDVPROC)glewGetProcAddress((const GLubyte*)"glFogCoorddv")) == NULL) || r;
  r = ((glFogCoordf = (PFNGLFOGCOORDFPROC)glewGetProcAddress((const GLubyte*)"glFogCoordf")) == NULL) || r;
  r = ((glFogCoordfv = (PFNGLFOGCOORDFVPROC)glewGetProcAddress((const GLubyte*)"glFogCoordfv")) == NULL) || r;
  r = ((glMultiDrawArrays = (PFNGLMULTIDRAWARRAYSPROC)glewGetProcAddress((const GLubyte*)"glMultiDrawArrays")) == NULL) || r;
  r = ((glMultiDrawElements = (PFNGLMULTIDRAWELEMENTSPROC)glewGetProcAddress((const GLubyte*)"glMultiDrawElements")) == NULL) || r;
  r = ((glPointParameterf = (PFNGLPOINTPARAMETERFPROC)glewGetProcAddress((const GLubyte*)"glPointParameterf")) == NULL) || r;
  r = ((glPointParameterfv = (PFNGLPOINTPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glPointParameterfv")) == NULL) || r;
  r = ((glPointParameteri = (PFNGLPOINTPARAMETERIPROC)glewGetProcAddress((const GLubyte*)"glPointParameteri")) == NULL) || r;
  r = ((glPointParameteriv = (PFNGLPOINTPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glPointParameteriv")) == NULL) || r;
  r = ((glSecondaryColor3b = (PFNGLSECONDARYCOLOR3BPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3b")) == NULL) || r;
  r = ((glSecondaryColor3bv = (PFNGLSECONDARYCOLOR3BVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3bv")) == NULL) || r;
  r = ((glSecondaryColor3d = (PFNGLSECONDARYCOLOR3DPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3d")) == NULL) || r;
  r = ((glSecondaryColor3dv = (PFNGLSECONDARYCOLOR3DVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3dv")) == NULL) || r;
  r = ((glSecondaryColor3f = (PFNGLSECONDARYCOLOR3FPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3f")) == NULL) || r;
  r = ((glSecondaryColor3fv = (PFNGLSECONDARYCOLOR3FVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3fv")) == NULL) || r;
  r = ((glSecondaryColor3i = (PFNGLSECONDARYCOLOR3IPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3i")) == NULL) || r;
  r = ((glSecondaryColor3iv = (PFNGLSECONDARYCOLOR3IVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3iv")) == NULL) || r;
  r = ((glSecondaryColor3s = (PFNGLSECONDARYCOLOR3SPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3s")) == NULL) || r;
  r = ((glSecondaryColor3sv = (PFNGLSECONDARYCOLOR3SVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3sv")) == NULL) || r;
  r = ((glSecondaryColor3ub = (PFNGLSECONDARYCOLOR3UBPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3ub")) == NULL) || r;
  r = ((glSecondaryColor3ubv = (PFNGLSECONDARYCOLOR3UBVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3ubv")) == NULL) || r;
  r = ((glSecondaryColor3ui = (PFNGLSECONDARYCOLOR3UIPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3ui")) == NULL) || r;
  r = ((glSecondaryColor3uiv = (PFNGLSECONDARYCOLOR3UIVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3uiv")) == NULL) || r;
  r = ((glSecondaryColor3us = (PFNGLSECONDARYCOLOR3USPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3us")) == NULL) || r;
  r = ((glSecondaryColor3usv = (PFNGLSECONDARYCOLOR3USVPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColor3usv")) == NULL) || r;
  r = ((glSecondaryColorPointer = (PFNGLSECONDARYCOLORPOINTERPROC)glewGetProcAddress((const GLubyte*)"glSecondaryColorPointer")) == NULL) || r;
  r = ((glWindowPos2d = (PFNGLWINDOWPOS2DPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2d")) == NULL) || r;
  r = ((glWindowPos2dv = (PFNGLWINDOWPOS2DVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2dv")) == NULL) || r;
  r = ((glWindowPos2f = (PFNGLWINDOWPOS2FPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2f")) == NULL) || r;
  r = ((glWindowPos2fv = (PFNGLWINDOWPOS2FVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2fv")) == NULL) || r;
  r = ((glWindowPos2i = (PFNGLWINDOWPOS2IPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2i")) == NULL) || r;
  r = ((glWindowPos2iv = (PFNGLWINDOWPOS2IVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2iv")) == NULL) || r;
  r = ((glWindowPos2s = (PFNGLWINDOWPOS2SPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2s")) == NULL) || r;
  r = ((glWindowPos2sv = (PFNGLWINDOWPOS2SVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos2sv")) == NULL) || r;
  r = ((glWindowPos3d = (PFNGLWINDOWPOS3DPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3d")) == NULL) || r;
  r = ((glWindowPos3dv = (PFNGLWINDOWPOS3DVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3dv")) == NULL) || r;
  r = ((glWindowPos3f = (PFNGLWINDOWPOS3FPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3f")) == NULL) || r;
  r = ((glWindowPos3fv = (PFNGLWINDOWPOS3FVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3fv")) == NULL) || r;
  r = ((glWindowPos3i = (PFNGLWINDOWPOS3IPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3i")) == NULL) || r;
  r = ((glWindowPos3iv = (PFNGLWINDOWPOS3IVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3iv")) == NULL) || r;
  r = ((glWindowPos3s = (PFNGLWINDOWPOS3SPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3s")) == NULL) || r;
  r = ((glWindowPos3sv = (PFNGLWINDOWPOS3SVPROC)glewGetProcAddress((const GLubyte*)"glWindowPos3sv")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_1_4 */

#ifdef GL_VERSION_1_5

static GLboolean _glewInit_GL_VERSION_1_5 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBeginQuery = (PFNGLBEGINQUERYPROC)glewGetProcAddress((const GLubyte*)"glBeginQuery")) == NULL) || r;
  r = ((glBindBuffer = (PFNGLBINDBUFFERPROC)glewGetProcAddress((const GLubyte*)"glBindBuffer")) == NULL) || r;
  r = ((glBufferData = (PFNGLBUFFERDATAPROC)glewGetProcAddress((const GLubyte*)"glBufferData")) == NULL) || r;
  r = ((glBufferSubData = (PFNGLBUFFERSUBDATAPROC)glewGetProcAddress((const GLubyte*)"glBufferSubData")) == NULL) || r;
  r = ((glDeleteBuffers = (PFNGLDELETEBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glDeleteBuffers")) == NULL) || r;
  r = ((glDeleteQueries = (PFNGLDELETEQUERIESPROC)glewGetProcAddress((const GLubyte*)"glDeleteQueries")) == NULL) || r;
  r = ((glEndQuery = (PFNGLENDQUERYPROC)glewGetProcAddress((const GLubyte*)"glEndQuery")) == NULL) || r;
  r = ((glGenBuffers = (PFNGLGENBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glGenBuffers")) == NULL) || r;
  r = ((glGenQueries = (PFNGLGENQUERIESPROC)glewGetProcAddress((const GLubyte*)"glGenQueries")) == NULL) || r;
  r = ((glGetBufferParameteriv = (PFNGLGETBUFFERPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glGetBufferParameteriv")) == NULL) || r;
  r = ((glGetBufferPointerv = (PFNGLGETBUFFERPOINTERVPROC)glewGetProcAddress((const GLubyte*)"glGetBufferPointerv")) == NULL) || r;
  r = ((glGetBufferSubData = (PFNGLGETBUFFERSUBDATAPROC)glewGetProcAddress((const GLubyte*)"glGetBufferSubData")) == NULL) || r;
  r = ((glGetQueryObjectiv = (PFNGLGETQUERYOBJECTIVPROC)glewGetProcAddress((const GLubyte*)"glGetQueryObjectiv")) == NULL) || r;
  r = ((glGetQueryObjectuiv = (PFNGLGETQUERYOBJECTUIVPROC)glewGetProcAddress((const GLubyte*)"glGetQueryObjectuiv")) == NULL) || r;
  r = ((glGetQueryiv = (PFNGLGETQUERYIVPROC)glewGetProcAddress((const GLubyte*)"glGetQueryiv")) == NULL) || r;
  r = ((glIsBuffer = (PFNGLISBUFFERPROC)glewGetProcAddress((const GLubyte*)"glIsBuffer")) == NULL) || r;
  r = ((glIsQuery = (PFNGLISQUERYPROC)glewGetProcAddress((const GLubyte*)"glIsQuery")) == NULL) || r;
  r = ((glMapBuffer = (PFNGLMAPBUFFERPROC)glewGetProcAddress((const GLubyte*)"glMapBuffer")) == NULL) || r;
  r = ((glUnmapBuffer = (PFNGLUNMAPBUFFERPROC)glewGetProcAddress((const GLubyte*)"glUnmapBuffer")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_1_5 */

#ifdef GL_VERSION_2_0

static GLboolean _glewInit_GL_VERSION_2_0 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glAttachShader = (PFNGLATTACHSHADERPROC)glewGetProcAddress((const GLubyte*)"glAttachShader")) == NULL) || r;
  r = ((glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)glewGetProcAddress((const GLubyte*)"glBindAttribLocation")) == NULL) || r;
  r = ((glBlendEquationSeparate = (PFNGLBLENDEQUATIONSEPARATEPROC)glewGetProcAddress((const GLubyte*)"glBlendEquationSeparate")) == NULL) || r;
  r = ((glCompileShader = (PFNGLCOMPILESHADERPROC)glewGetProcAddress((const GLubyte*)"glCompileShader")) == NULL) || r;
  r = ((glCreateProgram = (PFNGLCREATEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glCreateProgram")) == NULL) || r;
  r = ((glCreateShader = (PFNGLCREATESHADERPROC)glewGetProcAddress((const GLubyte*)"glCreateShader")) == NULL) || r;
  r = ((glDeleteProgram = (PFNGLDELETEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glDeleteProgram")) == NULL) || r;
  r = ((glDeleteShader = (PFNGLDELETESHADERPROC)glewGetProcAddress((const GLubyte*)"glDeleteShader")) == NULL) || r;
  r = ((glDetachShader = (PFNGLDETACHSHADERPROC)glewGetProcAddress((const GLubyte*)"glDetachShader")) == NULL) || r;
  r = ((glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)glewGetProcAddress((const GLubyte*)"glDisableVertexAttribArray")) == NULL) || r;
  r = ((glDrawBuffers = (PFNGLDRAWBUFFERSPROC)glewGetProcAddress((const GLubyte*)"glDrawBuffers")) == NULL) || r;
  r = ((glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)glewGetProcAddress((const GLubyte*)"glEnableVertexAttribArray")) == NULL) || r;
  r = ((glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)glewGetProcAddress((const GLubyte*)"glGetActiveAttrib")) == NULL) || r;
  r = ((glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)glewGetProcAddress((const GLubyte*)"glGetActiveUniform")) == NULL) || r;
  r = ((glGetAttachedShaders = (PFNGLGETATTACHEDSHADERSPROC)glewGetProcAddress((const GLubyte*)"glGetAttachedShaders")) == NULL) || r;
  r = ((glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)glewGetProcAddress((const GLubyte*)"glGetAttribLocation")) == NULL) || r;
  r = ((glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)glewGetProcAddress((const GLubyte*)"glGetProgramInfoLog")) == NULL) || r;
  r = ((glGetProgramiv = (PFNGLGETPROGRAMIVPROC)glewGetProcAddress((const GLubyte*)"glGetProgramiv")) == NULL) || r;
  r = ((glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)glewGetProcAddress((const GLubyte*)"glGetShaderInfoLog")) == NULL) || r;
  r = ((glGetShaderSource = (PFNGLGETSHADERSOURCEPROC)glewGetProcAddress((const GLubyte*)"glGetShaderSource")) == NULL) || r;
  r = ((glGetShaderiv = (PFNGLGETSHADERIVPROC)glewGetProcAddress((const GLubyte*)"glGetShaderiv")) == NULL) || r;
  r = ((glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)glewGetProcAddress((const GLubyte*)"glGetUniformLocation")) == NULL) || r;
  r = ((glGetUniformfv = (PFNGLGETUNIFORMFVPROC)glewGetProcAddress((const GLubyte*)"glGetUniformfv")) == NULL) || r;
  r = ((glGetUniformiv = (PFNGLGETUNIFORMIVPROC)glewGetProcAddress((const GLubyte*)"glGetUniformiv")) == NULL) || r;
  r = ((glGetVertexAttribPointerv = (PFNGLGETVERTEXATTRIBPOINTERVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribPointerv")) == NULL) || r;
  r = ((glGetVertexAttribdv = (PFNGLGETVERTEXATTRIBDVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribdv")) == NULL) || r;
  r = ((glGetVertexAttribfv = (PFNGLGETVERTEXATTRIBFVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribfv")) == NULL) || r;
  r = ((glGetVertexAttribiv = (PFNGLGETVERTEXATTRIBIVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribiv")) == NULL) || r;
  r = ((glIsProgram = (PFNGLISPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glIsProgram")) == NULL) || r;
  r = ((glIsShader = (PFNGLISSHADERPROC)glewGetProcAddress((const GLubyte*)"glIsShader")) == NULL) || r;
  r = ((glLinkProgram = (PFNGLLINKPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glLinkProgram")) == NULL) || r;
  r = ((glShaderSource = (PFNGLSHADERSOURCEPROC)glewGetProcAddress((const GLubyte*)"glShaderSource")) == NULL) || r;
  r = ((glStencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC)glewGetProcAddress((const GLubyte*)"glStencilFuncSeparate")) == NULL) || r;
  r = ((glStencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC)glewGetProcAddress((const GLubyte*)"glStencilMaskSeparate")) == NULL) || r;
  r = ((glStencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC)glewGetProcAddress((const GLubyte*)"glStencilOpSeparate")) == NULL) || r;
  r = ((glUniform1f = (PFNGLUNIFORM1FPROC)glewGetProcAddress((const GLubyte*)"glUniform1f")) == NULL) || r;
  r = ((glUniform1fv = (PFNGLUNIFORM1FVPROC)glewGetProcAddress((const GLubyte*)"glUniform1fv")) == NULL) || r;
  r = ((glUniform1i = (PFNGLUNIFORM1IPROC)glewGetProcAddress((const GLubyte*)"glUniform1i")) == NULL) || r;
  r = ((glUniform1iv = (PFNGLUNIFORM1IVPROC)glewGetProcAddress((const GLubyte*)"glUniform1iv")) == NULL) || r;
  r = ((glUniform2f = (PFNGLUNIFORM2FPROC)glewGetProcAddress((const GLubyte*)"glUniform2f")) == NULL) || r;
  r = ((glUniform2fv = (PFNGLUNIFORM2FVPROC)glewGetProcAddress((const GLubyte*)"glUniform2fv")) == NULL) || r;
  r = ((glUniform2i = (PFNGLUNIFORM2IPROC)glewGetProcAddress((const GLubyte*)"glUniform2i")) == NULL) || r;
  r = ((glUniform2iv = (PFNGLUNIFORM2IVPROC)glewGetProcAddress((const GLubyte*)"glUniform2iv")) == NULL) || r;
  r = ((glUniform3f = (PFNGLUNIFORM3FPROC)glewGetProcAddress((const GLubyte*)"glUniform3f")) == NULL) || r;
  r = ((glUniform3fv = (PFNGLUNIFORM3FVPROC)glewGetProcAddress((const GLubyte*)"glUniform3fv")) == NULL) || r;
  r = ((glUniform3i = (PFNGLUNIFORM3IPROC)glewGetProcAddress((const GLubyte*)"glUniform3i")) == NULL) || r;
  r = ((glUniform3iv = (PFNGLUNIFORM3IVPROC)glewGetProcAddress((const GLubyte*)"glUniform3iv")) == NULL) || r;
  r = ((glUniform4f = (PFNGLUNIFORM4FPROC)glewGetProcAddress((const GLubyte*)"glUniform4f")) == NULL) || r;
  r = ((glUniform4fv = (PFNGLUNIFORM4FVPROC)glewGetProcAddress((const GLubyte*)"glUniform4fv")) == NULL) || r;
  r = ((glUniform4i = (PFNGLUNIFORM4IPROC)glewGetProcAddress((const GLubyte*)"glUniform4i")) == NULL) || r;
  r = ((glUniform4iv = (PFNGLUNIFORM4IVPROC)glewGetProcAddress((const GLubyte*)"glUniform4iv")) == NULL) || r;
  r = ((glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix2fv")) == NULL) || r;
  r = ((glUniformMatrix3fv = (PFNGLUNIFORMMATRIX3FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix3fv")) == NULL) || r;
  r = ((glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix4fv")) == NULL) || r;
  r = ((glUseProgram = (PFNGLUSEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glUseProgram")) == NULL) || r;
  r = ((glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)glewGetProcAddress((const GLubyte*)"glValidateProgram")) == NULL) || r;
  r = ((glVertexAttrib1d = (PFNGLVERTEXATTRIB1DPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1d")) == NULL) || r;
  r = ((glVertexAttrib1dv = (PFNGLVERTEXATTRIB1DVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1dv")) == NULL) || r;
  r = ((glVertexAttrib1f = (PFNGLVERTEXATTRIB1FPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1f")) == NULL) || r;
  r = ((glVertexAttrib1fv = (PFNGLVERTEXATTRIB1FVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1fv")) == NULL) || r;
  r = ((glVertexAttrib1s = (PFNGLVERTEXATTRIB1SPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1s")) == NULL) || r;
  r = ((glVertexAttrib1sv = (PFNGLVERTEXATTRIB1SVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib1sv")) == NULL) || r;
  r = ((glVertexAttrib2d = (PFNGLVERTEXATTRIB2DPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2d")) == NULL) || r;
  r = ((glVertexAttrib2dv = (PFNGLVERTEXATTRIB2DVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2dv")) == NULL) || r;
  r = ((glVertexAttrib2f = (PFNGLVERTEXATTRIB2FPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2f")) == NULL) || r;
  r = ((glVertexAttrib2fv = (PFNGLVERTEXATTRIB2FVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2fv")) == NULL) || r;
  r = ((glVertexAttrib2s = (PFNGLVERTEXATTRIB2SPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2s")) == NULL) || r;
  r = ((glVertexAttrib2sv = (PFNGLVERTEXATTRIB2SVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib2sv")) == NULL) || r;
  r = ((glVertexAttrib3d = (PFNGLVERTEXATTRIB3DPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3d")) == NULL) || r;
  r = ((glVertexAttrib3dv = (PFNGLVERTEXATTRIB3DVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3dv")) == NULL) || r;
  r = ((glVertexAttrib3f = (PFNGLVERTEXATTRIB3FPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3f")) == NULL) || r;
  r = ((glVertexAttrib3fv = (PFNGLVERTEXATTRIB3FVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3fv")) == NULL) || r;
  r = ((glVertexAttrib3s = (PFNGLVERTEXATTRIB3SPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3s")) == NULL) || r;
  r = ((glVertexAttrib3sv = (PFNGLVERTEXATTRIB3SVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib3sv")) == NULL) || r;
  r = ((glVertexAttrib4Nbv = (PFNGLVERTEXATTRIB4NBVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nbv")) == NULL) || r;
  r = ((glVertexAttrib4Niv = (PFNGLVERTEXATTRIB4NIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Niv")) == NULL) || r;
  r = ((glVertexAttrib4Nsv = (PFNGLVERTEXATTRIB4NSVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nsv")) == NULL) || r;
  r = ((glVertexAttrib4Nub = (PFNGLVERTEXATTRIB4NUBPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nub")) == NULL) || r;
  r = ((glVertexAttrib4Nubv = (PFNGLVERTEXATTRIB4NUBVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nubv")) == NULL) || r;
  r = ((glVertexAttrib4Nuiv = (PFNGLVERTEXATTRIB4NUIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nuiv")) == NULL) || r;
  r = ((glVertexAttrib4Nusv = (PFNGLVERTEXATTRIB4NUSVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4Nusv")) == NULL) || r;
  r = ((glVertexAttrib4bv = (PFNGLVERTEXATTRIB4BVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4bv")) == NULL) || r;
  r = ((glVertexAttrib4d = (PFNGLVERTEXATTRIB4DPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4d")) == NULL) || r;
  r = ((glVertexAttrib4dv = (PFNGLVERTEXATTRIB4DVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4dv")) == NULL) || r;
  r = ((glVertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4f")) == NULL) || r;
  r = ((glVertexAttrib4fv = (PFNGLVERTEXATTRIB4FVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4fv")) == NULL) || r;
  r = ((glVertexAttrib4iv = (PFNGLVERTEXATTRIB4IVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4iv")) == NULL) || r;
  r = ((glVertexAttrib4s = (PFNGLVERTEXATTRIB4SPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4s")) == NULL) || r;
  r = ((glVertexAttrib4sv = (PFNGLVERTEXATTRIB4SVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4sv")) == NULL) || r;
  r = ((glVertexAttrib4ubv = (PFNGLVERTEXATTRIB4UBVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4ubv")) == NULL) || r;
  r = ((glVertexAttrib4uiv = (PFNGLVERTEXATTRIB4UIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4uiv")) == NULL) || r;
  r = ((glVertexAttrib4usv = (PFNGLVERTEXATTRIB4USVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttrib4usv")) == NULL) || r;
  r = ((glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribPointer")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_2_0 */

#ifdef GL_VERSION_2_1

static GLboolean _glewInit_GL_VERSION_2_1 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glUniformMatrix2x3fv = (PFNGLUNIFORMMATRIX2X3FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix2x3fv")) == NULL) || r;
  r = ((glUniformMatrix2x4fv = (PFNGLUNIFORMMATRIX2X4FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix2x4fv")) == NULL) || r;
  r = ((glUniformMatrix3x2fv = (PFNGLUNIFORMMATRIX3X2FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix3x2fv")) == NULL) || r;
  r = ((glUniformMatrix3x4fv = (PFNGLUNIFORMMATRIX3X4FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix3x4fv")) == NULL) || r;
  r = ((glUniformMatrix4x2fv = (PFNGLUNIFORMMATRIX4X2FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix4x2fv")) == NULL) || r;
  r = ((glUniformMatrix4x3fv = (PFNGLUNIFORMMATRIX4X3FVPROC)glewGetProcAddress((const GLubyte*)"glUniformMatrix4x3fv")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_2_1 */

#ifdef GL_VERSION_3_0

static GLboolean _glewInit_GL_VERSION_3_0 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBeginConditionalRender = (PFNGLBEGINCONDITIONALRENDERPROC)glewGetProcAddress((const GLubyte*)"glBeginConditionalRender")) == NULL) || r;
  r = ((glBeginTransformFeedback = (PFNGLBEGINTRANSFORMFEEDBACKPROC)glewGetProcAddress((const GLubyte*)"glBeginTransformFeedback")) == NULL) || r;
  r = ((glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)glewGetProcAddress((const GLubyte*)"glBindFragDataLocation")) == NULL) || r;
  r = ((glClampColor = (PFNGLCLAMPCOLORPROC)glewGetProcAddress((const GLubyte*)"glClampColor")) == NULL) || r;
  r = ((glClearBufferfi = (PFNGLCLEARBUFFERFIPROC)glewGetProcAddress((const GLubyte*)"glClearBufferfi")) == NULL) || r;
  r = ((glClearBufferfv = (PFNGLCLEARBUFFERFVPROC)glewGetProcAddress((const GLubyte*)"glClearBufferfv")) == NULL) || r;
  r = ((glClearBufferiv = (PFNGLCLEARBUFFERIVPROC)glewGetProcAddress((const GLubyte*)"glClearBufferiv")) == NULL) || r;
  r = ((glClearBufferuiv = (PFNGLCLEARBUFFERUIVPROC)glewGetProcAddress((const GLubyte*)"glClearBufferuiv")) == NULL) || r;
  r = ((glColorMaski = (PFNGLCOLORMASKIPROC)glewGetProcAddress((const GLubyte*)"glColorMaski")) == NULL) || r;
  r = ((glDisablei = (PFNGLDISABLEIPROC)glewGetProcAddress((const GLubyte*)"glDisablei")) == NULL) || r;
  r = ((glEnablei = (PFNGLENABLEIPROC)glewGetProcAddress((const GLubyte*)"glEnablei")) == NULL) || r;
  r = ((glEndConditionalRender = (PFNGLENDCONDITIONALRENDERPROC)glewGetProcAddress((const GLubyte*)"glEndConditionalRender")) == NULL) || r;
  r = ((glEndTransformFeedback = (PFNGLENDTRANSFORMFEEDBACKPROC)glewGetProcAddress((const GLubyte*)"glEndTransformFeedback")) == NULL) || r;
  r = ((glGetBooleani_v = (PFNGLGETBOOLEANI_VPROC)glewGetProcAddress((const GLubyte*)"glGetBooleani_v")) == NULL) || r;
  r = ((glGetFragDataLocation = (PFNGLGETFRAGDATALOCATIONPROC)glewGetProcAddress((const GLubyte*)"glGetFragDataLocation")) == NULL) || r;
  r = ((glGetStringi = (PFNGLGETSTRINGIPROC)glewGetProcAddress((const GLubyte*)"glGetStringi")) == NULL) || r;
  r = ((glGetTexParameterIiv = (PFNGLGETTEXPARAMETERIIVPROC)glewGetProcAddress((const GLubyte*)"glGetTexParameterIiv")) == NULL) || r;
  r = ((glGetTexParameterIuiv = (PFNGLGETTEXPARAMETERIUIVPROC)glewGetProcAddress((const GLubyte*)"glGetTexParameterIuiv")) == NULL) || r;
  r = ((glGetTransformFeedbackVarying = (PFNGLGETTRANSFORMFEEDBACKVARYINGPROC)glewGetProcAddress((const GLubyte*)"glGetTransformFeedbackVarying")) == NULL) || r;
  r = ((glGetUniformuiv = (PFNGLGETUNIFORMUIVPROC)glewGetProcAddress((const GLubyte*)"glGetUniformuiv")) == NULL) || r;
  r = ((glGetVertexAttribIiv = (PFNGLGETVERTEXATTRIBIIVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribIiv")) == NULL) || r;
  r = ((glGetVertexAttribIuiv = (PFNGLGETVERTEXATTRIBIUIVPROC)glewGetProcAddress((const GLubyte*)"glGetVertexAttribIuiv")) == NULL) || r;
  r = ((glIsEnabledi = (PFNGLISENABLEDIPROC)glewGetProcAddress((const GLubyte*)"glIsEnabledi")) == NULL) || r;
  r = ((glTexParameterIiv = (PFNGLTEXPARAMETERIIVPROC)glewGetProcAddress((const GLubyte*)"glTexParameterIiv")) == NULL) || r;
  r = ((glTexParameterIuiv = (PFNGLTEXPARAMETERIUIVPROC)glewGetProcAddress((const GLubyte*)"glTexParameterIuiv")) == NULL) || r;
  r = ((glTransformFeedbackVaryings = (PFNGLTRANSFORMFEEDBACKVARYINGSPROC)glewGetProcAddress((const GLubyte*)"glTransformFeedbackVaryings")) == NULL) || r;
  r = ((glUniform1ui = (PFNGLUNIFORM1UIPROC)glewGetProcAddress((const GLubyte*)"glUniform1ui")) == NULL) || r;
  r = ((glUniform1uiv = (PFNGLUNIFORM1UIVPROC)glewGetProcAddress((const GLubyte*)"glUniform1uiv")) == NULL) || r;
  r = ((glUniform2ui = (PFNGLUNIFORM2UIPROC)glewGetProcAddress((const GLubyte*)"glUniform2ui")) == NULL) || r;
  r = ((glUniform2uiv = (PFNGLUNIFORM2UIVPROC)glewGetProcAddress((const GLubyte*)"glUniform2uiv")) == NULL) || r;
  r = ((glUniform3ui = (PFNGLUNIFORM3UIPROC)glewGetProcAddress((const GLubyte*)"glUniform3ui")) == NULL) || r;
  r = ((glUniform3uiv = (PFNGLUNIFORM3UIVPROC)glewGetProcAddress((const GLubyte*)"glUniform3uiv")) == NULL) || r;
  r = ((glUniform4ui = (PFNGLUNIFORM4UIPROC)glewGetProcAddress((const GLubyte*)"glUniform4ui")) == NULL) || r;
  r = ((glUniform4uiv = (PFNGLUNIFORM4UIVPROC)glewGetProcAddress((const GLubyte*)"glUniform4uiv")) == NULL) || r;
  r = ((glVertexAttribI1i = (PFNGLVERTEXATTRIBI1IPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI1i")) == NULL) || r;
  r = ((glVertexAttribI1iv = (PFNGLVERTEXATTRIBI1IVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI1iv")) == NULL) || r;
  r = ((glVertexAttribI1ui = (PFNGLVERTEXATTRIBI1UIPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI1ui")) == NULL) || r;
  r = ((glVertexAttribI1uiv = (PFNGLVERTEXATTRIBI1UIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI1uiv")) == NULL) || r;
  r = ((glVertexAttribI2i = (PFNGLVERTEXATTRIBI2IPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI2i")) == NULL) || r;
  r = ((glVertexAttribI2iv = (PFNGLVERTEXATTRIBI2IVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI2iv")) == NULL) || r;
  r = ((glVertexAttribI2ui = (PFNGLVERTEXATTRIBI2UIPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI2ui")) == NULL) || r;
  r = ((glVertexAttribI2uiv = (PFNGLVERTEXATTRIBI2UIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI2uiv")) == NULL) || r;
  r = ((glVertexAttribI3i = (PFNGLVERTEXATTRIBI3IPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI3i")) == NULL) || r;
  r = ((glVertexAttribI3iv = (PFNGLVERTEXATTRIBI3IVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI3iv")) == NULL) || r;
  r = ((glVertexAttribI3ui = (PFNGLVERTEXATTRIBI3UIPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI3ui")) == NULL) || r;
  r = ((glVertexAttribI3uiv = (PFNGLVERTEXATTRIBI3UIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI3uiv")) == NULL) || r;
  r = ((glVertexAttribI4bv = (PFNGLVERTEXATTRIBI4BVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4bv")) == NULL) || r;
  r = ((glVertexAttribI4i = (PFNGLVERTEXATTRIBI4IPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4i")) == NULL) || r;
  r = ((glVertexAttribI4iv = (PFNGLVERTEXATTRIBI4IVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4iv")) == NULL) || r;
  r = ((glVertexAttribI4sv = (PFNGLVERTEXATTRIBI4SVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4sv")) == NULL) || r;
  r = ((glVertexAttribI4ubv = (PFNGLVERTEXATTRIBI4UBVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4ubv")) == NULL) || r;
  r = ((glVertexAttribI4ui = (PFNGLVERTEXATTRIBI4UIPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4ui")) == NULL) || r;
  r = ((glVertexAttribI4uiv = (PFNGLVERTEXATTRIBI4UIVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4uiv")) == NULL) || r;
  r = ((glVertexAttribI4usv = (PFNGLVERTEXATTRIBI4USVPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribI4usv")) == NULL) || r;
  r = ((glVertexAttribIPointer = (PFNGLVERTEXATTRIBIPOINTERPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribIPointer")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_3_0 */

#ifdef GL_VERSION_3_1

static GLboolean _glewInit_GL_VERSION_3_1 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glDrawArraysInstanced = (PFNGLDRAWARRAYSINSTANCEDPROC)glewGetProcAddress((const GLubyte*)"glDrawArraysInstanced")) == NULL) || r;
  r = ((glDrawElementsInstanced = (PFNGLDRAWELEMENTSINSTANCEDPROC)glewGetProcAddress((const GLubyte*)"glDrawElementsInstanced")) == NULL) || r;
  r = ((glPrimitiveRestartIndex = (PFNGLPRIMITIVERESTARTINDEXPROC)glewGetProcAddress((const GLubyte*)"glPrimitiveRestartIndex")) == NULL) || r;
  r = ((glTexBuffer = (PFNGLTEXBUFFERPROC)glewGetProcAddress((const GLubyte*)"glTexBuffer")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_3_1 */

#ifdef GL_VERSION_3_2

static GLboolean _glewInit_GL_VERSION_3_2 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)glewGetProcAddress((const GLubyte*)"glFramebufferTexture")) == NULL) || r;
  r = ((glGetBufferParameteri64v = (PFNGLGETBUFFERPARAMETERI64VPROC)glewGetProcAddress((const GLubyte*)"glGetBufferParameteri64v")) == NULL) || r;
  r = ((glGetInteger64i_v = (PFNGLGETINTEGER64I_VPROC)glewGetProcAddress((const GLubyte*)"glGetInteger64i_v")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_3_2 */

#ifdef GL_VERSION_3_3

static GLboolean _glewInit_GL_VERSION_3_3 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glVertexAttribDivisor = (PFNGLVERTEXATTRIBDIVISORPROC)glewGetProcAddress((const GLubyte*)"glVertexAttribDivisor")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_3_3 */

#ifdef GL_VERSION_4_0

static GLboolean _glewInit_GL_VERSION_4_0 (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBlendEquationSeparatei = (PFNGLBLENDEQUATIONSEPARATEIPROC)glewGetProcAddress((const GLubyte*)"glBlendEquationSeparatei")) == NULL) || r;
  r = ((glBlendEquationi = (PFNGLBLENDEQUATIONIPROC)glewGetProcAddress((const GLubyte*)"glBlendEquationi")) == NULL) || r;
  r = ((glBlendFuncSeparatei = (PFNGLBLENDFUNCSEPARATEIPROC)glewGetProcAddress((const GLubyte*)"glBlendFuncSeparatei")) == NULL) || r;
  r = ((glBlendFunci = (PFNGLBLENDFUNCIPROC)glewGetProcAddress((const GLubyte*)"glBlendFunci")) == NULL) || r;
  r = ((glMinSampleShading = (PFNGLMINSAMPLESHADINGPROC)glewGetProcAddress((const GLubyte*)"glMinSampleShading")) == NULL) || r;

  return r;
}

#endif /* GL_VERSION_4_0 */

#ifdef GL_VERSION_4_1

#endif /* GL_VERSION_4_1 */

#ifdef GL_VERSION_4_2

#endif /* GL_VERSION_4_2 */

#ifdef GL_ARB_imaging

static GLboolean _glewInit_GL_ARB_imaging (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBlendEquation = (PFNGLBLENDEQUATIONPROC)glewGetProcAddress((const GLubyte*)"glBlendEquation")) == NULL) || r;
  r = ((glColorSubTable = (PFNGLCOLORSUBTABLEPROC)glewGetProcAddress((const GLubyte*)"glColorSubTable")) == NULL) || r;
  r = ((glColorTable = (PFNGLCOLORTABLEPROC)glewGetProcAddress((const GLubyte*)"glColorTable")) == NULL) || r;
  r = ((glColorTableParameterfv = (PFNGLCOLORTABLEPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glColorTableParameterfv")) == NULL) || r;
  r = ((glColorTableParameteriv = (PFNGLCOLORTABLEPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glColorTableParameteriv")) == NULL) || r;
  r = ((glConvolutionFilter1D = (PFNGLCONVOLUTIONFILTER1DPROC)glewGetProcAddress((const GLubyte*)"glConvolutionFilter1D")) == NULL) || r;
  r = ((glConvolutionFilter2D = (PFNGLCONVOLUTIONFILTER2DPROC)glewGetProcAddress((const GLubyte*)"glConvolutionFilter2D")) == NULL) || r;
  r = ((glConvolutionParameterf = (PFNGLCONVOLUTIONPARAMETERFPROC)glewGetProcAddress((const GLubyte*)"glConvolutionParameterf")) == NULL) || r;
  r = ((glConvolutionParameterfv = (PFNGLCONVOLUTIONPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glConvolutionParameterfv")) == NULL) || r;
  r = ((glConvolutionParameteri = (PFNGLCONVOLUTIONPARAMETERIPROC)glewGetProcAddress((const GLubyte*)"glConvolutionParameteri")) == NULL) || r;
  r = ((glConvolutionParameteriv = (PFNGLCONVOLUTIONPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glConvolutionParameteriv")) == NULL) || r;
  r = ((glCopyColorSubTable = (PFNGLCOPYCOLORSUBTABLEPROC)glewGetProcAddress((const GLubyte*)"glCopyColorSubTable")) == NULL) || r;
  r = ((glCopyColorTable = (PFNGLCOPYCOLORTABLEPROC)glewGetProcAddress((const GLubyte*)"glCopyColorTable")) == NULL) || r;
  r = ((glCopyConvolutionFilter1D = (PFNGLCOPYCONVOLUTIONFILTER1DPROC)glewGetProcAddress((const GLubyte*)"glCopyConvolutionFilter1D")) == NULL) || r;
  r = ((glCopyConvolutionFilter2D = (PFNGLCOPYCONVOLUTIONFILTER2DPROC)glewGetProcAddress((const GLubyte*)"glCopyConvolutionFilter2D")) == NULL) || r;
  r = ((glGetColorTable = (PFNGLGETCOLORTABLEPROC)glewGetProcAddress((const GLubyte*)"glGetColorTable")) == NULL) || r;
  r = ((glGetColorTableParameterfv = (PFNGLGETCOLORTABLEPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glGetColorTableParameterfv")) == NULL) || r;
  r = ((glGetColorTableParameteriv = (PFNGLGETCOLORTABLEPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glGetColorTableParameteriv")) == NULL) || r;
  r = ((glGetConvolutionFilter = (PFNGLGETCONVOLUTIONFILTERPROC)glewGetProcAddress((const GLubyte*)"glGetConvolutionFilter")) == NULL) || r;
  r = ((glGetConvolutionParameterfv = (PFNGLGETCONVOLUTIONPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glGetConvolutionParameterfv")) == NULL) || r;
  r = ((glGetConvolutionParameteriv = (PFNGLGETCONVOLUTIONPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glGetConvolutionParameteriv")) == NULL) || r;
  r = ((glGetHistogram = (PFNGLGETHISTOGRAMPROC)glewGetProcAddress((const GLubyte*)"glGetHistogram")) == NULL) || r;
  r = ((glGetHistogramParameterfv = (PFNGLGETHISTOGRAMPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glGetHistogramParameterfv")) == NULL) || r;
  r = ((glGetHistogramParameteriv = (PFNGLGETHISTOGRAMPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glGetHistogramParameteriv")) == NULL) || r;
  r = ((glGetMinmax = (PFNGLGETMINMAXPROC)glewGetProcAddress((const GLubyte*)"glGetMinmax")) == NULL) || r;
  r = ((glGetMinmaxParameterfv = (PFNGLGETMINMAXPARAMETERFVPROC)glewGetProcAddress((const GLubyte*)"glGetMinmaxParameterfv")) == NULL) || r;
  r = ((glGetMinmaxParameteriv = (PFNGLGETMINMAXPARAMETERIVPROC)glewGetProcAddress((const GLubyte*)"glGetMinmaxParameteriv")) == NULL) || r;
  r = ((glGetSeparableFilter = (PFNGLGETSEPARABLEFILTERPROC)glewGetProcAddress((const GLubyte*)"glGetSeparableFilter")) == NULL) || r;
  r = ((glHistogram = (PFNGLHISTOGRAMPROC)glewGetProcAddress((const GLubyte*)"glHistogram")) == NULL) || r;
  r = ((glMinmax = (PFNGLMINMAXPROC)glewGetProcAddress((const GLubyte*)"glMinmax")) == NULL) || r;
  r = ((glResetHistogram = (PFNGLRESETHISTOGRAMPROC)glewGetProcAddress((const GLubyte*)"glResetHistogram")) == NULL) || r;
  r = ((glResetMinmax = (PFNGLRESETMINMAXPROC)glewGetProcAddress((const GLubyte*)"glResetMinmax")) == NULL) || r;
  r = ((glSeparableFilter2D = (PFNGLSEPARABLEFILTER2DPROC)glewGetProcAddress((const GLubyte*)"glSeparableFilter2D")) == NULL) || r;

  return r;
}

#endif /* GL_ARB_imaging */

#ifdef GL_ARB_multisample

static GLboolean _glewInit_GL_ARB_multisample (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glSampleCoverageARB = (PFNGLSAMPLECOVERAGEARBPROC)glewGetProcAddress((const GLubyte*)"glSampleCoverageARB")) == NULL) || r;

  return r;
}

#endif /* GL_ARB_multisample */

#ifdef GL_ARB_pixel_buffer_object

#endif /* GL_ARB_pixel_buffer_object */

#ifdef GL_ARB_texture_non_power_of_two

#endif /* GL_ARB_texture_non_power_of_two */

#ifdef GL_EXT_framebuffer_object

static GLboolean _glewInit_GL_EXT_framebuffer_object (GLEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)glewGetProcAddress((const GLubyte*)"glBindFramebufferEXT")) == NULL) || r;
  r = ((glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)glewGetProcAddress((const GLubyte*)"glBindRenderbufferEXT")) == NULL) || r;
  r = ((glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)glewGetProcAddress((const GLubyte*)"glCheckFramebufferStatusEXT")) == NULL) || r;
  r = ((glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)glewGetProcAddress((const GLubyte*)"glDeleteFramebuffersEXT")) == NULL) || r;
  r = ((glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)glewGetProcAddress((const GLubyte*)"glDeleteRenderbuffersEXT")) == NULL) || r;
  r = ((glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)glewGetProcAddress((const GLubyte*)"glFramebufferRenderbufferEXT")) == NULL) || r;
  r = ((glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)glewGetProcAddress((const GLubyte*)"glFramebufferTexture1DEXT")) == NULL) || r;
  r = ((glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)glewGetProcAddress((const GLubyte*)"glFramebufferTexture2DEXT")) == NULL) || r;
  r = ((glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)glewGetProcAddress((const GLubyte*)"glFramebufferTexture3DEXT")) == NULL) || r;
  r = ((glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)glewGetProcAddress((const GLubyte*)"glGenFramebuffersEXT")) == NULL) || r;
  r = ((glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)glewGetProcAddress((const GLubyte*)"glGenRenderbuffersEXT")) == NULL) || r;
  r = ((glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)glewGetProcAddress((const GLubyte*)"glGenerateMipmapEXT")) == NULL) || r;
  r = ((glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)glewGetProcAddress((const GLubyte*)"glGetFramebufferAttachmentParameterivEXT")) == NULL) || r;
  r = ((glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)glewGetProcAddress((const GLubyte*)"glGetRenderbufferParameterivEXT")) == NULL) || r;
  r = ((glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)glewGetProcAddress((const GLubyte*)"glIsFramebufferEXT")) == NULL) || r;
  r = ((glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)glewGetProcAddress((const GLubyte*)"glIsRenderbufferEXT")) == NULL) || r;
  r = ((glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)glewGetProcAddress((const GLubyte*)"glRenderbufferStorageEXT")) == NULL) || r;

  return r;
}

#endif /* GL_EXT_framebuffer_object */

#ifdef GL_EXT_texture_filter_anisotropic

#endif /* GL_EXT_texture_filter_anisotropic */

#ifdef GL_NV_multisample_filter_hint

#endif /* GL_NV_multisample_filter_hint */

#ifdef GL_SGIS_generate_mipmap

#endif /* GL_SGIS_generate_mipmap */

/* ------------------------------------------------------------------------- */

GLboolean glewGetExtension (const char* name)
{    
  const GLubyte* start;
  const GLubyte* end;
  start = (const GLubyte*)glGetString(GL_EXTENSIONS);
  if (start == 0)
    return GL_FALSE;
  end = start + _glewStrLen(start);
  return _glewSearchExtension(name, start, end);
}

/* ------------------------------------------------------------------------- */

#ifndef GLEW_MX
static
#endif
GLenum glewContextInit (GLEW_CONTEXT_ARG_DEF_LIST)
{
  const GLubyte* s;
  GLuint dot;
  GLint major, minor;
  const GLubyte* extStart;
  const GLubyte* extEnd;
  /* query opengl version */
  s = glGetString(GL_VERSION);
  dot = _glewStrCLen(s, '.');
  if (dot == 0)
    return GLEW_ERROR_NO_GL_VERSION;
  
  major = s[dot-1]-'0';
  minor = s[dot+1]-'0';

  if (minor < 0 || minor > 9)
    minor = 0;
  if (major<0 || major>9)
    return GLEW_ERROR_NO_GL_VERSION;
  

  if (major == 1 && minor == 0)
  {
    return GLEW_ERROR_GL_VERSION_10_ONLY;
  }
  else
  {
    CONST_CAST(GLEW_VERSION_4_2)   = ( major > 4 )                 || ( major == 4 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_1)   = GLEW_VERSION_4_2   == GL_TRUE || ( major == 4 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_4_0)   = GLEW_VERSION_4_1   == GL_TRUE || ( major == 4               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_3)   = GLEW_VERSION_4_0   == GL_TRUE || ( major == 3 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_2)   = GLEW_VERSION_3_3   == GL_TRUE || ( major == 3 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_1)   = GLEW_VERSION_3_2   == GL_TRUE || ( major == 3 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_3_0)   = GLEW_VERSION_3_1   == GL_TRUE || ( major == 3               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_2_1)   = GLEW_VERSION_3_0   == GL_TRUE || ( major == 2 && minor >= 1 ) ? GL_TRUE : GL_FALSE;    
    CONST_CAST(GLEW_VERSION_2_0)   = GLEW_VERSION_2_1   == GL_TRUE || ( major == 2               ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_5)   = GLEW_VERSION_2_0   == GL_TRUE || ( major == 1 && minor >= 5 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_4)   = GLEW_VERSION_1_5   == GL_TRUE || ( major == 1 && minor >= 4 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_3)   = GLEW_VERSION_1_4   == GL_TRUE || ( major == 1 && minor >= 3 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_2_1) = GLEW_VERSION_1_3   == GL_TRUE                                 ? GL_TRUE : GL_FALSE; 
    CONST_CAST(GLEW_VERSION_1_2)   = GLEW_VERSION_1_2_1 == GL_TRUE || ( major == 1 && minor >= 2 ) ? GL_TRUE : GL_FALSE;
    CONST_CAST(GLEW_VERSION_1_1)   = GLEW_VERSION_1_2   == GL_TRUE || ( major == 1 && minor >= 1 ) ? GL_TRUE : GL_FALSE;
  }

  /* query opengl extensions string */
  extStart = glGetString(GL_EXTENSIONS);
  if (extStart == 0)
    extStart = (const GLubyte*)"";
  extEnd = extStart + _glewStrLen(extStart);

  /* initialize extensions */
#ifdef GL_VERSION_1_2
  if (glewExperimental || GLEW_VERSION_1_2) CONST_CAST(GLEW_VERSION_1_2) = !_glewInit_GL_VERSION_1_2(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_1_2 */
#ifdef GL_VERSION_1_2_1
#endif /* GL_VERSION_1_2_1 */
#ifdef GL_VERSION_1_3
  if (glewExperimental || GLEW_VERSION_1_3) CONST_CAST(GLEW_VERSION_1_3) = !_glewInit_GL_VERSION_1_3(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_1_3 */
#ifdef GL_VERSION_1_4
  if (glewExperimental || GLEW_VERSION_1_4) CONST_CAST(GLEW_VERSION_1_4) = !_glewInit_GL_VERSION_1_4(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_1_4 */
#ifdef GL_VERSION_1_5
  if (glewExperimental || GLEW_VERSION_1_5) CONST_CAST(GLEW_VERSION_1_5) = !_glewInit_GL_VERSION_1_5(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_1_5 */
#ifdef GL_VERSION_2_0
  if (glewExperimental || GLEW_VERSION_2_0) CONST_CAST(GLEW_VERSION_2_0) = !_glewInit_GL_VERSION_2_0(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_2_0 */
#ifdef GL_VERSION_2_1
  if (glewExperimental || GLEW_VERSION_2_1) CONST_CAST(GLEW_VERSION_2_1) = !_glewInit_GL_VERSION_2_1(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_2_1 */
#ifdef GL_VERSION_3_0
  if (glewExperimental || GLEW_VERSION_3_0) CONST_CAST(GLEW_VERSION_3_0) = !_glewInit_GL_VERSION_3_0(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_3_0 */
#ifdef GL_VERSION_3_1
  if (glewExperimental || GLEW_VERSION_3_1) CONST_CAST(GLEW_VERSION_3_1) = !_glewInit_GL_VERSION_3_1(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_3_1 */
#ifdef GL_VERSION_3_2
  if (glewExperimental || GLEW_VERSION_3_2) CONST_CAST(GLEW_VERSION_3_2) = !_glewInit_GL_VERSION_3_2(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_3_2 */
#ifdef GL_VERSION_3_3
  if (glewExperimental || GLEW_VERSION_3_3) CONST_CAST(GLEW_VERSION_3_3) = !_glewInit_GL_VERSION_3_3(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_3_3 */
#ifdef GL_VERSION_4_0
  if (glewExperimental || GLEW_VERSION_4_0) CONST_CAST(GLEW_VERSION_4_0) = !_glewInit_GL_VERSION_4_0(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_VERSION_4_0 */
#ifdef GL_VERSION_4_1
#endif /* GL_VERSION_4_1 */
#ifdef GL_VERSION_4_2
#endif /* GL_VERSION_4_2 */
#ifdef GL_ARB_imaging
  CONST_CAST(GLEW_ARB_imaging) = _glewSearchExtension("GL_ARB_imaging", extStart, extEnd);
  if (glewExperimental || GLEW_ARB_imaging) CONST_CAST(GLEW_ARB_imaging) = !_glewInit_GL_ARB_imaging(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_ARB_imaging */
#ifdef GL_ARB_multisample
  CONST_CAST(GLEW_ARB_multisample) = _glewSearchExtension("GL_ARB_multisample", extStart, extEnd);
  if (glewExperimental || GLEW_ARB_multisample) CONST_CAST(GLEW_ARB_multisample) = !_glewInit_GL_ARB_multisample(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_ARB_multisample */
#ifdef GL_ARB_pixel_buffer_object
  CONST_CAST(GLEW_ARB_pixel_buffer_object) = _glewSearchExtension("GL_ARB_pixel_buffer_object", extStart, extEnd);
#endif /* GL_ARB_pixel_buffer_object */
#ifdef GL_ARB_texture_non_power_of_two
  CONST_CAST(GLEW_ARB_texture_non_power_of_two) = _glewSearchExtension("GL_ARB_texture_non_power_of_two", extStart, extEnd);
#endif /* GL_ARB_texture_non_power_of_two */
#ifdef GL_EXT_framebuffer_object
  CONST_CAST(GLEW_EXT_framebuffer_object) = _glewSearchExtension("GL_EXT_framebuffer_object", extStart, extEnd);
  if (glewExperimental || GLEW_EXT_framebuffer_object) CONST_CAST(GLEW_EXT_framebuffer_object) = !_glewInit_GL_EXT_framebuffer_object(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GL_EXT_framebuffer_object */
#ifdef GL_EXT_texture_filter_anisotropic
  CONST_CAST(GLEW_EXT_texture_filter_anisotropic) = _glewSearchExtension("GL_EXT_texture_filter_anisotropic", extStart, extEnd);
#endif /* GL_EXT_texture_filter_anisotropic */
#ifdef GL_NV_multisample_filter_hint
  CONST_CAST(GLEW_NV_multisample_filter_hint) = _glewSearchExtension("GL_NV_multisample_filter_hint", extStart, extEnd);
#endif /* GL_NV_multisample_filter_hint */
#ifdef GL_SGIS_generate_mipmap
  CONST_CAST(GLEW_SGIS_generate_mipmap) = _glewSearchExtension("GL_SGIS_generate_mipmap", extStart, extEnd);
#endif /* GL_SGIS_generate_mipmap */

  return GLEW_OK;
}


#if defined(_WIN32)

#if !defined(GLEW_MX)

#endif /* !GLEW_MX */

/* ------------------------------------------------------------------------- */

static PFNWGLGETEXTENSIONSSTRINGARBPROC _wglewGetExtensionsStringARB = NULL;
static PFNWGLGETEXTENSIONSSTRINGEXTPROC _wglewGetExtensionsStringEXT = NULL;

GLboolean wglewGetExtension (const char* name)
{    
  const GLubyte* start;
  const GLubyte* end;
  if (_wglewGetExtensionsStringARB == NULL)
    if (_wglewGetExtensionsStringEXT == NULL)
      return GL_FALSE;
    else
      start = (const GLubyte*)_wglewGetExtensionsStringEXT();
  else
    start = (const GLubyte*)_wglewGetExtensionsStringARB(wglGetCurrentDC());
  if (start == 0)
    return GL_FALSE;
  end = start + _glewStrLen(start);
  return _glewSearchExtension(name, start, end);
}

GLenum wglewContextInit (WGLEW_CONTEXT_ARG_DEF_LIST)
{
  GLboolean crippled;
  const GLubyte* extStart;
  const GLubyte* extEnd;
  /* find wgl extension string query functions */
  _wglewGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)glewGetProcAddress((const GLubyte*)"wglGetExtensionsStringARB");
  _wglewGetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)glewGetProcAddress((const GLubyte*)"wglGetExtensionsStringEXT");
  /* query wgl extension string */
  if (_wglewGetExtensionsStringARB == NULL)
    if (_wglewGetExtensionsStringEXT == NULL)
      extStart = (const GLubyte*)"";
    else
      extStart = (const GLubyte*)_wglewGetExtensionsStringEXT();
  else
    extStart = (const GLubyte*)_wglewGetExtensionsStringARB(wglGetCurrentDC());
  extEnd = extStart + _glewStrLen(extStart);
  /* initialize extensions */
  crippled = _wglewGetExtensionsStringARB == NULL && _wglewGetExtensionsStringEXT == NULL;

  return GLEW_OK;
}

#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)

PFNGLXGETCURRENTDISPLAYPROC __glewXGetCurrentDisplay = NULL;

PFNGLXCHOOSEFBCONFIGPROC __glewXChooseFBConfig = NULL;
PFNGLXCREATENEWCONTEXTPROC __glewXCreateNewContext = NULL;
PFNGLXCREATEPBUFFERPROC __glewXCreatePbuffer = NULL;
PFNGLXCREATEPIXMAPPROC __glewXCreatePixmap = NULL;
PFNGLXCREATEWINDOWPROC __glewXCreateWindow = NULL;
PFNGLXDESTROYPBUFFERPROC __glewXDestroyPbuffer = NULL;
PFNGLXDESTROYPIXMAPPROC __glewXDestroyPixmap = NULL;
PFNGLXDESTROYWINDOWPROC __glewXDestroyWindow = NULL;
PFNGLXGETCURRENTREADDRAWABLEPROC __glewXGetCurrentReadDrawable = NULL;
PFNGLXGETFBCONFIGATTRIBPROC __glewXGetFBConfigAttrib = NULL;
PFNGLXGETFBCONFIGSPROC __glewXGetFBConfigs = NULL;
PFNGLXGETSELECTEDEVENTPROC __glewXGetSelectedEvent = NULL;
PFNGLXGETVISUALFROMFBCONFIGPROC __glewXGetVisualFromFBConfig = NULL;
PFNGLXMAKECONTEXTCURRENTPROC __glewXMakeContextCurrent = NULL;
PFNGLXQUERYCONTEXTPROC __glewXQueryContext = NULL;
PFNGLXQUERYDRAWABLEPROC __glewXQueryDrawable = NULL;
PFNGLXSELECTEVENTPROC __glewXSelectEvent = NULL;

#if !defined(GLEW_MX)

GLboolean __GLXEW_VERSION_1_0 = GL_FALSE;
GLboolean __GLXEW_VERSION_1_1 = GL_FALSE;
GLboolean __GLXEW_VERSION_1_2 = GL_FALSE;
GLboolean __GLXEW_VERSION_1_3 = GL_FALSE;
GLboolean __GLXEW_VERSION_1_4 = GL_FALSE;

#endif /* !GLEW_MX */

#ifdef GLX_VERSION_1_2

static GLboolean _glewInit_GLX_VERSION_1_2 (GLXEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glXGetCurrentDisplay = (PFNGLXGETCURRENTDISPLAYPROC)glewGetProcAddress((const GLubyte*)"glXGetCurrentDisplay")) == NULL) || r;

  return r;
}

#endif /* GLX_VERSION_1_2 */

#ifdef GLX_VERSION_1_3

static GLboolean _glewInit_GLX_VERSION_1_3 (GLXEW_CONTEXT_ARG_DEF_INIT)
{
  GLboolean r = GL_FALSE;

  r = ((glXChooseFBConfig = (PFNGLXCHOOSEFBCONFIGPROC)glewGetProcAddress((const GLubyte*)"glXChooseFBConfig")) == NULL) || r;
  r = ((glXCreateNewContext = (PFNGLXCREATENEWCONTEXTPROC)glewGetProcAddress((const GLubyte*)"glXCreateNewContext")) == NULL) || r;
  r = ((glXCreatePbuffer = (PFNGLXCREATEPBUFFERPROC)glewGetProcAddress((const GLubyte*)"glXCreatePbuffer")) == NULL) || r;
  r = ((glXCreatePixmap = (PFNGLXCREATEPIXMAPPROC)glewGetProcAddress((const GLubyte*)"glXCreatePixmap")) == NULL) || r;
  r = ((glXCreateWindow = (PFNGLXCREATEWINDOWPROC)glewGetProcAddress((const GLubyte*)"glXCreateWindow")) == NULL) || r;
  r = ((glXDestroyPbuffer = (PFNGLXDESTROYPBUFFERPROC)glewGetProcAddress((const GLubyte*)"glXDestroyPbuffer")) == NULL) || r;
  r = ((glXDestroyPixmap = (PFNGLXDESTROYPIXMAPPROC)glewGetProcAddress((const GLubyte*)"glXDestroyPixmap")) == NULL) || r;
  r = ((glXDestroyWindow = (PFNGLXDESTROYWINDOWPROC)glewGetProcAddress((const GLubyte*)"glXDestroyWindow")) == NULL) || r;
  r = ((glXGetCurrentReadDrawable = (PFNGLXGETCURRENTREADDRAWABLEPROC)glewGetProcAddress((const GLubyte*)"glXGetCurrentReadDrawable")) == NULL) || r;
  r = ((glXGetFBConfigAttrib = (PFNGLXGETFBCONFIGATTRIBPROC)glewGetProcAddress((const GLubyte*)"glXGetFBConfigAttrib")) == NULL) || r;
  r = ((glXGetFBConfigs = (PFNGLXGETFBCONFIGSPROC)glewGetProcAddress((const GLubyte*)"glXGetFBConfigs")) == NULL) || r;
  r = ((glXGetSelectedEvent = (PFNGLXGETSELECTEDEVENTPROC)glewGetProcAddress((const GLubyte*)"glXGetSelectedEvent")) == NULL) || r;
  r = ((glXGetVisualFromFBConfig = (PFNGLXGETVISUALFROMFBCONFIGPROC)glewGetProcAddress((const GLubyte*)"glXGetVisualFromFBConfig")) == NULL) || r;
  r = ((glXMakeContextCurrent = (PFNGLXMAKECONTEXTCURRENTPROC)glewGetProcAddress((const GLubyte*)"glXMakeContextCurrent")) == NULL) || r;
  r = ((glXQueryContext = (PFNGLXQUERYCONTEXTPROC)glewGetProcAddress((const GLubyte*)"glXQueryContext")) == NULL) || r;
  r = ((glXQueryDrawable = (PFNGLXQUERYDRAWABLEPROC)glewGetProcAddress((const GLubyte*)"glXQueryDrawable")) == NULL) || r;
  r = ((glXSelectEvent = (PFNGLXSELECTEVENTPROC)glewGetProcAddress((const GLubyte*)"glXSelectEvent")) == NULL) || r;

  return r;
}

#endif /* GLX_VERSION_1_3 */

#ifdef GLX_VERSION_1_4

#endif /* GLX_VERSION_1_4 */

/* ------------------------------------------------------------------------ */

GLboolean glxewGetExtension (const char* name)
{    
  const GLubyte* start;
  const GLubyte* end;

  if (glXGetCurrentDisplay == NULL) return GL_FALSE;
  start = (const GLubyte*)glXGetClientString(glXGetCurrentDisplay(), GLX_EXTENSIONS);
  if (0 == start) return GL_FALSE;
  end = start + _glewStrLen(start);
  return _glewSearchExtension(name, start, end);
}

GLenum glxewContextInit (GLXEW_CONTEXT_ARG_DEF_LIST)
{
  int major, minor;
  const GLubyte* extStart;
  const GLubyte* extEnd;
  /* initialize core GLX 1.2 */
  if (_glewInit_GLX_VERSION_1_2(GLEW_CONTEXT_ARG_VAR_INIT)) return GLEW_ERROR_GLX_VERSION_11_ONLY;
  /* initialize flags */
  CONST_CAST(GLXEW_VERSION_1_0) = GL_TRUE;
  CONST_CAST(GLXEW_VERSION_1_1) = GL_TRUE;
  CONST_CAST(GLXEW_VERSION_1_2) = GL_TRUE;
  CONST_CAST(GLXEW_VERSION_1_3) = GL_TRUE;
  CONST_CAST(GLXEW_VERSION_1_4) = GL_TRUE;
  /* query GLX version */
  glXQueryVersion(glXGetCurrentDisplay(), &major, &minor);
  if (major == 1 && minor <= 3)
  {
    switch (minor)
    {
      case 3:
      CONST_CAST(GLXEW_VERSION_1_4) = GL_FALSE;
      break;
      case 2:
      CONST_CAST(GLXEW_VERSION_1_4) = GL_FALSE;
      CONST_CAST(GLXEW_VERSION_1_3) = GL_FALSE;
      break;
      default:
      return GLEW_ERROR_GLX_VERSION_11_ONLY;
      break;
    }
  }
  /* query GLX extension string */
  extStart = 0;
  if (glXGetCurrentDisplay != NULL)
    extStart = (const GLubyte*)glXGetClientString(glXGetCurrentDisplay(), GLX_EXTENSIONS);
  if (extStart == 0)
    extStart = (const GLubyte *)"";
  extEnd = extStart + _glewStrLen(extStart);
  /* initialize extensions */
#ifdef GLX_VERSION_1_3
  if (glewExperimental || GLXEW_VERSION_1_3) CONST_CAST(GLXEW_VERSION_1_3) = !_glewInit_GLX_VERSION_1_3(GLEW_CONTEXT_ARG_VAR_INIT);
#endif /* GLX_VERSION_1_3 */

  return GLEW_OK;
}

#endif /* !__APPLE__ || GLEW_APPLE_GLX */

/* ------------------------------------------------------------------------ */

const GLubyte* glewGetErrorString (GLenum error)
{
  static const GLubyte* _glewErrorString[] =
  {
    (const GLubyte*)"No error",
    (const GLubyte*)"Missing GL version",
    (const GLubyte*)"GL 1.1 and up are not supported",
    (const GLubyte*)"GLX 1.2 and up are not supported",
    (const GLubyte*)"Unknown error"
  };
  const int max_error = sizeof(_glewErrorString)/sizeof(*_glewErrorString) - 1;
  return _glewErrorString[(int)error > max_error ? max_error : (int)error];
}

const GLubyte* glewGetString (GLenum name)
{
  static const GLubyte* _glewString[] =
  {
    (const GLubyte*)NULL,
    (const GLubyte*)"1.7.0",
    (const GLubyte*)"1",
    (const GLubyte*)"7",
    (const GLubyte*)"0"
  };
  const int max_string = sizeof(_glewString)/sizeof(*_glewString) - 1;
  return _glewString[(int)name > max_string ? 0 : (int)name];
}

/* ------------------------------------------------------------------------ */

GLboolean glewExperimental = GL_FALSE;

#if !defined(GLEW_MX)

#if defined(_WIN32)
extern GLenum wglewContextInit (void);
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX) /* _UNIX */
extern GLenum glxewContextInit (void);
#endif /* _WIN32 */

GLenum glewInit ()
{
  GLenum r;
  if ( (r = glewContextInit()) ) return r;
#if defined(_WIN32)
  return wglewContextInit();
#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX) /* _UNIX */
  return glxewContextInit();
#else
  return r;
#endif /* _WIN32 */
}

#endif /* !GLEW_MX */
#ifdef GLEW_MX
GLboolean glewContextIsSupported (const GLEWContext* ctx, const char* name)
#else
GLboolean glewIsSupported (const char* name)
#endif
{
  GLubyte* pos = (GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if (_glewStrSame1(&pos, &len, (const GLubyte*)"GL_", 3))
    {
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"VERSION_", 8))
      {
#ifdef GL_VERSION_1_2
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_2", 3))
        {
          ret = GLEW_VERSION_1_2;
          continue;
        }
#endif
#ifdef GL_VERSION_1_2_1
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_2_1", 5))
        {
          ret = GLEW_VERSION_1_2_1;
          continue;
        }
#endif
#ifdef GL_VERSION_1_3
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_3", 3))
        {
          ret = GLEW_VERSION_1_3;
          continue;
        }
#endif
#ifdef GL_VERSION_1_4
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_4", 3))
        {
          ret = GLEW_VERSION_1_4;
          continue;
        }
#endif
#ifdef GL_VERSION_1_5
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_5", 3))
        {
          ret = GLEW_VERSION_1_5;
          continue;
        }
#endif
#ifdef GL_VERSION_2_0
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"2_0", 3))
        {
          ret = GLEW_VERSION_2_0;
          continue;
        }
#endif
#ifdef GL_VERSION_2_1
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"2_1", 3))
        {
          ret = GLEW_VERSION_2_1;
          continue;
        }
#endif
#ifdef GL_VERSION_3_0
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"3_0", 3))
        {
          ret = GLEW_VERSION_3_0;
          continue;
        }
#endif
#ifdef GL_VERSION_3_1
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"3_1", 3))
        {
          ret = GLEW_VERSION_3_1;
          continue;
        }
#endif
#ifdef GL_VERSION_3_2
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"3_2", 3))
        {
          ret = GLEW_VERSION_3_2;
          continue;
        }
#endif
#ifdef GL_VERSION_3_3
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"3_3", 3))
        {
          ret = GLEW_VERSION_3_3;
          continue;
        }
#endif
#ifdef GL_VERSION_4_0
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"4_0", 3))
        {
          ret = GLEW_VERSION_4_0;
          continue;
        }
#endif
#ifdef GL_VERSION_4_1
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"4_1", 3))
        {
          ret = GLEW_VERSION_4_1;
          continue;
        }
#endif
#ifdef GL_VERSION_4_2
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"4_2", 3))
        {
          ret = GLEW_VERSION_4_2;
          continue;
        }
#endif
      }
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"ARB_", 4))
      {
#ifdef GL_ARB_imaging
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"imaging", 7))
        {
          ret = GLEW_ARB_imaging;
          continue;
        }
#endif
#ifdef GL_ARB_multisample
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"multisample", 11))
        {
          ret = GLEW_ARB_multisample;
          continue;
        }
#endif
#ifdef GL_ARB_pixel_buffer_object
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"pixel_buffer_object", 19))
        {
          ret = GLEW_ARB_pixel_buffer_object;
          continue;
        }
#endif
#ifdef GL_ARB_texture_non_power_of_two
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"texture_non_power_of_two", 24))
        {
          ret = GLEW_ARB_texture_non_power_of_two;
          continue;
        }
#endif
      }
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"EXT_", 4))
      {
#ifdef GL_EXT_framebuffer_object
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"framebuffer_object", 18))
        {
          ret = GLEW_EXT_framebuffer_object;
          continue;
        }
#endif
#ifdef GL_EXT_texture_filter_anisotropic
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"texture_filter_anisotropic", 26))
        {
          ret = GLEW_EXT_texture_filter_anisotropic;
          continue;
        }
#endif
      }
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"NV_", 3))
      {
#ifdef GL_NV_multisample_filter_hint
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"multisample_filter_hint", 23))
        {
          ret = GLEW_NV_multisample_filter_hint;
          continue;
        }
#endif
      }
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"SGIS_", 5))
      {
#ifdef GL_SGIS_generate_mipmap
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"generate_mipmap", 15))
        {
          ret = GLEW_SGIS_generate_mipmap;
          continue;
        }
#endif
      }
    }
    ret = (len == 0);
  }
  return ret;
}

#if defined(_WIN32)

#if defined(GLEW_MX)
GLboolean wglewContextIsSupported (const WGLEWContext* ctx, const char* name)
#else
GLboolean wglewIsSupported (const char* name)
#endif
{
  GLubyte* pos = (GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if (_glewStrSame1(&pos, &len, (const GLubyte*)"WGL_", 4))
    {
    }
    ret = (len == 0);
  }
  return ret;
}

#elif !defined(__APPLE__) || defined(GLEW_APPLE_GLX)

#if defined(GLEW_MX)
GLboolean glxewContextIsSupported (const GLXEWContext* ctx, const char* name)
#else
GLboolean glxewIsSupported (const char* name)
#endif
{
  GLubyte* pos = (GLubyte*)name;
  GLuint len = _glewStrLen(pos);
  GLboolean ret = GL_TRUE;
  while (ret && len > 0)
  {
    if(_glewStrSame1(&pos, &len, (const GLubyte*)"GLX_", 4))
    {
      if (_glewStrSame2(&pos, &len, (const GLubyte*)"VERSION_", 8))
      {
#ifdef GLX_VERSION_1_2
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_2", 3))
        {
          ret = GLXEW_VERSION_1_2;
          continue;
        }
#endif
#ifdef GLX_VERSION_1_3
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_3", 3))
        {
          ret = GLXEW_VERSION_1_3;
          continue;
        }
#endif
#ifdef GLX_VERSION_1_4
        if (_glewStrSame3(&pos, &len, (const GLubyte*)"1_4", 3))
        {
          ret = GLXEW_VERSION_1_4;
          continue;
        }
#endif
      }
    }
    ret = (len == 0);
  }
  return ret;
}

#endif /* _WIN32 */
