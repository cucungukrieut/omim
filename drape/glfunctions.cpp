#include "drape/glfunctions.hpp"
#include "drape/glIncludes.hpp"
#include "drape/glextensions_list.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/macros.hpp"
#include "base/mutex.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <utility>

#if defined(OMIM_OS_WINDOWS)
#define DP_APIENTRY __stdcall
#else
#define DP_APIENTRY
#endif

// static
dp::ApiVersion GLFunctions::CurrentApiVersion = dp::ApiVersion::OpenGLES2;

namespace
{
#ifdef DEBUG
using NodeKey = std::pair<std::thread::id, glConst>;
using Node = std::pair<NodeKey, uint32_t>;
using BoundMap = std::map<NodeKey, uint32_t>;
BoundMap g_boundBuffers;
std::mutex g_boundBuffersMutex;
#endif

inline GLboolean convert(bool v) { return static_cast<GLboolean>(v ? GL_TRUE : GL_FALSE); }

typedef void(DP_APIENTRY * TglClearColorFn)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
typedef void(DP_APIENTRY * TglClearFn)(GLbitfield mask);
typedef void(DP_APIENTRY * TglViewportFn)(GLint x, GLint y, GLsizei w, GLsizei h);
typedef void(DP_APIENTRY * TglScissorFn)(GLint x, GLint y, GLsizei w, GLsizei h);
typedef void(DP_APIENTRY * TglFlushFn)();

typedef void(DP_APIENTRY * TglActiveTextureFn)(GLenum texture);
typedef void(DP_APIENTRY * TglBlendEquationFn)(GLenum mode);

typedef void(DP_APIENTRY * TglGenVertexArraysFn)(GLsizei n, GLuint * ids);
typedef void(DP_APIENTRY * TglBindVertexArrayFn)(GLuint id);
typedef void(DP_APIENTRY * TglDeleteVertexArrayFn)(GLsizei n, GLuint const * ids);

typedef void(DP_APIENTRY * TglGetBufferParameterFn)(GLenum target, GLenum value, GLint * data);
typedef void(DP_APIENTRY * TglGenBuffersFn)(GLsizei n, GLuint * buffers);
typedef void(DP_APIENTRY * TglBindBufferFn)(GLenum target, GLuint buffer);
typedef void(DP_APIENTRY * TglDeleteBuffersFn)(GLsizei n, GLuint const * buffers);
typedef void(DP_APIENTRY * TglBufferDataFn)(GLenum target, GLsizeiptr size, GLvoid const * data,
                                            GLenum usage);
typedef void(DP_APIENTRY * TglBufferSubDataFn)(GLenum target, GLintptr offset, GLsizeiptr size,
                                               GLvoid const * data);
typedef void *(DP_APIENTRY * TglMapBufferFn)(GLenum target, GLenum access);
typedef GLboolean(DP_APIENTRY * TglUnmapBufferFn)(GLenum target);
typedef void *(DP_APIENTRY * TglMapBufferRangeFn)(GLenum target, GLintptr offset, GLsizeiptr length,
                                                  GLbitfield access);
typedef void(DP_APIENTRY * TglFlushMappedBufferRangeFn)(GLenum target, GLintptr offset,
                                                        GLsizeiptr length);

typedef GLuint(DP_APIENTRY * TglCreateShaderFn)(GLenum type);
typedef void(DP_APIENTRY * TglShaderSourceFn)(GLuint shaderID, GLsizei count,
                                              GLchar const ** string, GLint const * length);
typedef void(DP_APIENTRY * TglCompileShaderFn)(GLuint shaderID);
typedef void(DP_APIENTRY * TglDeleteShaderFn)(GLuint shaderID);
typedef void(DP_APIENTRY * TglGetShaderivFn)(GLuint shaderID, GLenum name, GLint * p);
typedef void(DP_APIENTRY * TglGetShaderInfoLogFn)(GLuint shaderID, GLsizei maxLength,
                                                  GLsizei * length, GLchar * infoLog);

typedef GLuint(DP_APIENTRY * TglCreateProgramFn)();
typedef void(DP_APIENTRY * TglAttachShaderFn)(GLuint programID, GLuint shaderID);
typedef void(DP_APIENTRY * TglDetachShaderFn)(GLuint programID, GLuint shaderID);
typedef void(DP_APIENTRY * TglLinkProgramFn)(GLuint programID);
typedef void(DP_APIENTRY * TglDeleteProgramFn)(GLuint programID);
typedef void(DP_APIENTRY * TglGetProgramivFn)(GLuint programID, GLenum name, GLint * p);
typedef void(DP_APIENTRY * TglGetProgramInfoLogFn)(GLuint programID, GLsizei maxLength,
                                                   GLsizei * length, GLchar * infoLog);

typedef void(DP_APIENTRY * TglUseProgramFn)(GLuint programID);
typedef GLint(DP_APIENTRY * TglGetAttribLocationFn)(GLuint program, GLchar const * name);
typedef void(DP_APIENTRY * TglBindAttribLocationFn)(GLuint program, GLuint index,
                                                    GLchar const * name);

typedef void(DP_APIENTRY * TglEnableVertexAttributeFn)(GLuint location);
typedef void(DP_APIENTRY * TglVertexAttributePointerFn)(GLuint index, GLint count, GLenum type,
                                                        GLboolean normalize, GLsizei stride,
                                                        GLvoid const * p);
typedef GLint(DP_APIENTRY * TglGetUniformLocationFn)(GLuint programID, GLchar const * name);
typedef void(DP_APIENTRY * TglGetActiveUniformFn)(GLuint programID, GLuint uniformIndex,
                                                  GLsizei bufSize, GLsizei * length, GLint * size,
                                                  GLenum * type, GLchar * name);
typedef void(DP_APIENTRY * TglUniform1iFn)(GLint location, GLint value);
typedef void(DP_APIENTRY * TglUniform2iFn)(GLint location, GLint v1, GLint v2);
typedef void(DP_APIENTRY * TglUniform3iFn)(GLint location, GLint v1, GLint v2, GLint v3);
typedef void(DP_APIENTRY * TglUniform4iFn)(GLint location, GLint v1, GLint v2, GLint v3, GLint v4);
typedef void(DP_APIENTRY * TglUniform1ivFn)(GLint location, GLsizei count, GLint const * value);
typedef void(DP_APIENTRY * TglUniform1fFn)(GLint location, GLfloat value);
typedef void(DP_APIENTRY * TglUniform2fFn)(GLint location, GLfloat v1, GLfloat v2);
typedef void(DP_APIENTRY * TglUniform3fFn)(GLint location, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void(DP_APIENTRY * TglUniform4fFn)(GLint location, GLfloat v1, GLfloat v2, GLfloat v3,
                                           GLfloat v4);
typedef void(DP_APIENTRY * TglUniform1fvFn)(GLint location, GLsizei count, GLfloat const * value);
typedef void(DP_APIENTRY * TglUniformMatrix4fvFn)(GLint location, GLsizei count,
                                                  GLboolean transpose, GLfloat const * value);

typedef void(DP_APIENTRY * TglGenFramebuffersFn)(GLsizei n, GLuint * framebuffers);
typedef void(DP_APIENTRY * TglDeleteFramebuffersFn)(GLsizei n, GLuint const * framebuffers);
typedef void(DP_APIENTRY * TglBindFramebufferFn)(GLenum target, GLuint id);
typedef void(DP_APIENTRY * TglFramebufferTexture2DFn)(GLenum target, GLenum attachment,
                                                      GLenum textarget, GLuint texture,
                                                      GLint level);
typedef GLenum(DP_APIENTRY * TglCheckFramebufferStatusFn)(GLenum target);

typedef GLubyte const * (DP_APIENTRY * TglGetStringiFn) (GLenum name, GLuint index);

TglClearColorFn glClearColorFn = nullptr;
TglClearFn glClearFn = nullptr;
TglViewportFn glViewportFn = nullptr;
TglScissorFn glScissorFn = nullptr;
TglFlushFn glFlushFn = nullptr;

TglActiveTextureFn glActiveTextureFn = nullptr;
TglBlendEquationFn glBlendEquationFn = nullptr;

/// VAO
TglGenVertexArraysFn glGenVertexArraysFn = nullptr;
TglBindVertexArrayFn glBindVertexArrayFn = nullptr;
TglDeleteVertexArrayFn glDeleteVertexArrayFn = nullptr;

/// VBO
TglGetBufferParameterFn glGetBufferParameterFn = nullptr;
TglGenBuffersFn glGenBuffersFn = nullptr;
TglBindBufferFn glBindBufferFn = nullptr;
TglDeleteBuffersFn glDeleteBuffersFn = nullptr;
TglBufferDataFn glBufferDataFn = nullptr;
TglBufferSubDataFn glBufferSubDataFn = nullptr;
TglMapBufferFn glMapBufferFn = nullptr;
TglUnmapBufferFn glUnmapBufferFn = nullptr;
TglMapBufferRangeFn glMapBufferRangeFn = nullptr;
TglFlushMappedBufferRangeFn glFlushMappedBufferRangeFn = nullptr;

/// Shaders
TglCreateShaderFn glCreateShaderFn = nullptr;
TglShaderSourceFn glShaderSourceFn = nullptr;
TglCompileShaderFn glCompileShaderFn = nullptr;
TglDeleteShaderFn glDeleteShaderFn = nullptr;
TglGetShaderivFn glGetShaderivFn = nullptr;
TglGetShaderInfoLogFn glGetShaderInfoLogFn = nullptr;

TglCreateProgramFn glCreateProgramFn = nullptr;
TglAttachShaderFn glAttachShaderFn = nullptr;
TglDetachShaderFn glDetachShaderFn = nullptr;
TglLinkProgramFn glLinkProgramFn = nullptr;
TglDeleteProgramFn glDeleteProgramFn = nullptr;
TglGetProgramivFn glGetProgramivFn = nullptr;
TglGetProgramInfoLogFn glGetProgramInfoLogFn = nullptr;

TglUseProgramFn glUseProgramFn = nullptr;
TglGetAttribLocationFn glGetAttribLocationFn = nullptr;
TglBindAttribLocationFn glBindAttribLocationFn = nullptr;

TglEnableVertexAttributeFn glEnableVertexAttributeFn = nullptr;
TglVertexAttributePointerFn glVertexAttributePointerFn = nullptr;
TglGetUniformLocationFn glGetUniformLocationFn = nullptr;
TglGetActiveUniformFn glGetActiveUniformFn = nullptr;
TglUniform1iFn glUniform1iFn = nullptr;
TglUniform2iFn glUniform2iFn = nullptr;
TglUniform3iFn glUniform3iFn = nullptr;
TglUniform4iFn glUniform4iFn = nullptr;
TglUniform1ivFn glUniform1ivFn = nullptr;
TglUniform1fFn glUniform1fFn = nullptr;
TglUniform2fFn glUniform2fFn = nullptr;
TglUniform3fFn glUniform3fFn = nullptr;
TglUniform4fFn glUniform4fFn = nullptr;
TglUniform1fvFn glUniform1fvFn = nullptr;
TglUniformMatrix4fvFn glUniformMatrix4fvFn = nullptr;

/// FBO
TglGenFramebuffersFn glGenFramebuffersFn = nullptr;
TglDeleteFramebuffersFn glDeleteFramebuffersFn = nullptr;
TglBindFramebufferFn glBindFramebufferFn = nullptr;
TglFramebufferTexture2DFn glFramebufferTexture2DFn = nullptr;
TglCheckFramebufferStatusFn glCheckFramebufferStatusFn = nullptr;

TglGetStringiFn glGetStringiFn = nullptr;

#if !defined(GL_NUM_EXTENSIONS)
  #define GL_NUM_EXTENSIONS 0x821D
#endif

std::mutex s_mutex;
bool s_inited = false;
}  // namespace

#ifdef OMIM_OS_WINDOWS
template <typename TFunc>
TFunc LoadExtension(std::string const & ext)
{
  TFunc func = reinterpret_cast<TFunc>(wglGetProcAddress(ext.c_str()));
  if (func == nullptr)
  {
    func = reinterpret_cast<TFunc>(wglGetProcAddress((ext + "EXT").c_str()));
    ASSERT(func, ());
  }

  return func;
}
#define LOAD_GL_FUNC(type, func) LoadExtension<type>(#func);
#else
#define LOAD_GL_FUNC(type, func) static_cast<type>(&::func)
#endif

void GLFunctions::Init(dp::ApiVersion apiVersion)
{
  std::lock_guard<std::mutex> lock(s_mutex);
  if (s_inited)
    return;

  CurrentApiVersion = apiVersion;
  s_inited = true;

/// VAO
#if defined(OMIM_OS_MAC)
  if (CurrentApiVersion == dp::ApiVersion::OpenGLES2)
  {
    glGenVertexArraysFn = &glGenVertexArraysAPPLE;
    glBindVertexArrayFn = &glBindVertexArrayAPPLE;
    glDeleteVertexArrayFn = &glDeleteVertexArraysAPPLE;
    glMapBufferFn = &::glMapBuffer;
    glUnmapBufferFn = &::glUnmapBuffer;
  }
  else if (CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    glGenVertexArraysFn = &::glGenVertexArrays;
    glBindVertexArrayFn = &::glBindVertexArray;
    glDeleteVertexArrayFn = &::glDeleteVertexArrays;
    glUnmapBufferFn = &::glUnmapBuffer;
    glMapBufferRangeFn = &::glMapBufferRange;
    glFlushMappedBufferRangeFn = &::glFlushMappedBufferRange;
    glGetStringiFn = &::glGetStringi;
  }
  else
  {
    ASSERT(false, ("Unknown Graphics API"));
  }
#elif defined(OMIM_OS_LINUX)
  glGenVertexArraysFn = &::glGenVertexArrays;
  glBindVertexArrayFn = &::glBindVertexArray;
  glDeleteVertexArrayFn = &::glDeleteVertexArrays;
  glMapBufferFn = &::glMapBuffer;
  glUnmapBufferFn = &::glUnmapBuffer;
#elif defined(OMIM_OS_ANDROID)
  if (CurrentApiVersion == dp::ApiVersion::OpenGLES2)
  {
    glGenVertexArraysFn = (TglGenVertexArraysFn)eglGetProcAddress("glGenVertexArraysOES");
    glBindVertexArrayFn = (TglBindVertexArrayFn)eglGetProcAddress("glBindVertexArrayOES");
    glDeleteVertexArrayFn = (TglDeleteVertexArrayFn)eglGetProcAddress("glDeleteVertexArraysOES");
    glMapBufferFn = &::glMapBufferOES;
    glUnmapBufferFn = &::glUnmapBufferOES;
    glMapBufferRangeFn = (TglMapBufferRangeFn)eglGetProcAddress("glMapBufferRangeEXT");
    glFlushMappedBufferRangeFn =
        (TglFlushMappedBufferRangeFn)eglGetProcAddress("glFlushMappedBufferRangeEXT");
  }
  else if (CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    glGenVertexArraysFn = ::glGenVertexArrays;
    glBindVertexArrayFn = ::glBindVertexArray;
    glDeleteVertexArrayFn = ::glDeleteVertexArrays;
    glUnmapBufferFn = ::glUnmapBuffer;
    glMapBufferRangeFn = ::glMapBufferRange;
    glFlushMappedBufferRangeFn = ::glFlushMappedBufferRange;
    glGetStringiFn = ::glGetStringi;
  }
  else
  {
    ASSERT(false, ("Unknown Graphics API"));
  }
#elif defined(OMIM_OS_MOBILE)
  if (CurrentApiVersion == dp::ApiVersion::OpenGLES2)
  {
    glGenVertexArraysFn = &glGenVertexArraysOES;
    glBindVertexArrayFn = &glBindVertexArrayOES;
    glDeleteVertexArrayFn = &glDeleteVertexArraysOES;
    glMapBufferFn = &::glMapBufferOES;
    glUnmapBufferFn = &::glUnmapBufferOES;
    glMapBufferRangeFn = &::glMapBufferRangeEXT;
    glFlushMappedBufferRangeFn = &::glFlushMappedBufferRangeEXT;
  }
  else if (CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    glGenVertexArraysFn = &::glGenVertexArrays;
    glBindVertexArrayFn = &::glBindVertexArray;
    glDeleteVertexArrayFn = &::glDeleteVertexArrays;
    glUnmapBufferFn = &::glUnmapBuffer;
    glMapBufferRangeFn = &::glMapBufferRange;
    glFlushMappedBufferRangeFn = &::glFlushMappedBufferRange;
    glGetStringiFn = &::glGetStringi;
  }
  else
  {
    ASSERT(false, ("Unknown Graphics API"));
  }
#elif defined(OMIM_OS_WINDOWS)
  if (dp::GLExtensionsList::Instance().IsSupported(dp::GLExtensionsList::VertexArrayObject))
  {
    glGenVertexArraysFn = LOAD_GL_FUNC(TglGenVertexArraysFn, glGenVertexArrays);
    glBindVertexArrayFn = LOAD_GL_FUNC(TglBindVertexArrayFn, glBindVertexArray);
    glDeleteVertexArrayFn = LOAD_GL_FUNC(TglDeleteVertexArrayFn, glDeleteVertexArrays);
  }
  glMapBufferFn = LOAD_GL_FUNC(TglMapBufferFn, glMapBuffer);
  glUnmapBufferFn = LOAD_GL_FUNC(TglUnmapBufferFn, glUnmapBuffer);
#endif

  glClearColorFn = LOAD_GL_FUNC(TglClearColorFn, glClearColor);
  glClearFn = LOAD_GL_FUNC(TglClearFn, glClear);
  glViewportFn = LOAD_GL_FUNC(TglViewportFn, glViewport);
  glScissorFn = LOAD_GL_FUNC(TglScissorFn, glScissor);
  glFlushFn = LOAD_GL_FUNC(TglFlushFn, glFlush);

  glActiveTextureFn = LOAD_GL_FUNC(TglActiveTextureFn, glActiveTexture);
  glBlendEquationFn = LOAD_GL_FUNC(TglBlendEquationFn, glBlendEquation);

  /// VBO
  glGetBufferParameterFn = LOAD_GL_FUNC(TglGetBufferParameterFn, glGetBufferParameteriv);
  glGenBuffersFn = LOAD_GL_FUNC(TglGenBuffersFn, glGenBuffers);
  glBindBufferFn = LOAD_GL_FUNC(TglBindBufferFn, glBindBuffer);
  glDeleteBuffersFn = LOAD_GL_FUNC(TglDeleteBuffersFn, glDeleteBuffers);
  glBufferDataFn = LOAD_GL_FUNC(TglBufferDataFn, glBufferData);
  glBufferSubDataFn = LOAD_GL_FUNC(TglBufferSubDataFn, glBufferSubData);

  /// Shaders
  glCreateShaderFn = LOAD_GL_FUNC(TglCreateShaderFn, glCreateShader);
#ifdef OMIM_OS_WINDOWS
  glShaderSourceFn = LOAD_GL_FUNC(TglShaderSourceFn, glShaderSource);
#else
  typedef void(DP_APIENTRY * glShaderSource_Type)(GLuint shaderID, GLsizei count,
                                                  GLchar const ** string, GLint const * length);
  glShaderSourceFn = reinterpret_cast<glShaderSource_Type>(&::glShaderSource);
#endif
  glCompileShaderFn = LOAD_GL_FUNC(TglCompileShaderFn, glCompileShader);
  glDeleteShaderFn = LOAD_GL_FUNC(TglDeleteShaderFn, glDeleteShader);
  glGetShaderivFn = LOAD_GL_FUNC(TglGetShaderivFn, glGetShaderiv);
  glGetShaderInfoLogFn = LOAD_GL_FUNC(TglGetShaderInfoLogFn, glGetShaderInfoLog);

  glCreateProgramFn = LOAD_GL_FUNC(TglCreateProgramFn, glCreateProgram);
  glAttachShaderFn = LOAD_GL_FUNC(TglAttachShaderFn, glAttachShader);
  glDetachShaderFn = LOAD_GL_FUNC(TglDetachShaderFn, glDetachShader);
  glLinkProgramFn = LOAD_GL_FUNC(TglLinkProgramFn, glLinkProgram);
  glDeleteProgramFn = LOAD_GL_FUNC(TglDeleteProgramFn, glDeleteProgram);
  glGetProgramivFn = LOAD_GL_FUNC(TglGetProgramivFn, glGetProgramiv);
  glGetProgramInfoLogFn = LOAD_GL_FUNC(TglGetProgramInfoLogFn, glGetProgramInfoLog);

  glUseProgramFn = LOAD_GL_FUNC(TglUseProgramFn, glUseProgram);
  glGetAttribLocationFn = LOAD_GL_FUNC(TglGetAttribLocationFn, glGetAttribLocation);
  glBindAttribLocationFn = LOAD_GL_FUNC(TglBindAttribLocationFn, glBindAttribLocation);

  glEnableVertexAttributeFn = LOAD_GL_FUNC(TglEnableVertexAttributeFn, glEnableVertexAttribArray);
  glVertexAttributePointerFn = LOAD_GL_FUNC(TglVertexAttributePointerFn, glVertexAttribPointer);

  glGetUniformLocationFn = LOAD_GL_FUNC(TglGetUniformLocationFn, glGetUniformLocation);
  glGetActiveUniformFn = LOAD_GL_FUNC(TglGetActiveUniformFn, glGetActiveUniform);
  glUniform1iFn = LOAD_GL_FUNC(TglUniform1iFn, glUniform1i);
  glUniform2iFn = LOAD_GL_FUNC(TglUniform2iFn, glUniform2i);
  glUniform3iFn = LOAD_GL_FUNC(TglUniform3iFn, glUniform3i);
  glUniform4iFn = LOAD_GL_FUNC(TglUniform4iFn, glUniform4i);
  glUniform1ivFn = LOAD_GL_FUNC(TglUniform1ivFn, glUniform1iv);

  glUniform1fFn = LOAD_GL_FUNC(TglUniform1fFn, glUniform1f);
  glUniform2fFn = LOAD_GL_FUNC(TglUniform2fFn, glUniform2f);
  glUniform3fFn = LOAD_GL_FUNC(TglUniform3fFn, glUniform3f);
  glUniform4fFn = LOAD_GL_FUNC(TglUniform4fFn, glUniform4f);
  glUniform1fvFn = LOAD_GL_FUNC(TglUniform1fvFn, glUniform1fv);

  glUniformMatrix4fvFn = LOAD_GL_FUNC(TglUniformMatrix4fvFn, glUniformMatrix4fv);

  /// FBO
  glGenFramebuffersFn = LOAD_GL_FUNC(TglGenFramebuffersFn, glGenFramebuffers);
  glDeleteFramebuffersFn = LOAD_GL_FUNC(TglDeleteFramebuffersFn, glDeleteFramebuffers);
  glBindFramebufferFn = LOAD_GL_FUNC(TglBindFramebufferFn, glBindFramebuffer);
  glFramebufferTexture2DFn = LOAD_GL_FUNC(TglFramebufferTexture2DFn, glFramebufferTexture2D);
  glCheckFramebufferStatusFn = LOAD_GL_FUNC(TglCheckFramebufferStatusFn, glCheckFramebufferStatus);
}

bool GLFunctions::glHasExtension(std::string const & name)
{
  if (CurrentApiVersion == dp::ApiVersion::OpenGLES2)
  {
    char const * extensions = reinterpret_cast<char const *>(::glGetString(GL_EXTENSIONS));
    GLCHECKCALL();
    if (extensions == nullptr)
      return false;

    char const * extName = name.c_str();
    char const * ptr = nullptr;
    while ((ptr = strstr(extensions, extName)) != nullptr)
    {
      char const * end = ptr + strlen(extName);
      if (isspace(*end) || *end == '\0')
        return true;

      extensions = end;
    }
  }
  else if (CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    ASSERT(glGetStringiFn != nullptr, ());
    GLint n = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &n);
    for (GLint i = 0; i < n; i++)
    {
      std::string const extension =
          std::string(reinterpret_cast<char const *>(glGetStringiFn(GL_EXTENSIONS, i)));
      if (extension == name)
        return true;
    }
  }
  return false;
}

void GLFunctions::glClearColor(float r, float g, float b, float a)
{
  ASSERT(glClearColorFn != nullptr, ());
  GLCHECK(glClearColorFn(r, g, b, a));
}

void GLFunctions::glClear()
{
  ASSERT(glClearFn != nullptr, ());
  GLCHECK(glClearFn(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void GLFunctions::glClearDepth()
{
  ASSERT(glClearFn != nullptr, ());
  GLCHECK(glClearFn(GL_DEPTH_BUFFER_BIT));
}

void GLFunctions::glViewport(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  ASSERT(glViewportFn != nullptr, ());
  GLCHECK(glViewportFn(x, y, w, h));
}

void GLFunctions::glScissor(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
  ASSERT(glScissorFn != nullptr, ());
  GLCHECK(glScissorFn(x, y, w, h));
}

void GLFunctions::glFlush()
{
  ASSERT(glFlushFn != nullptr, ());
  GLCHECK(glFlushFn());
}

void GLFunctions::glFinish() { GLCHECK(::glFinish()); }
void GLFunctions::glFrontFace(glConst mode) { GLCHECK(::glFrontFace(mode)); }
void GLFunctions::glCullFace(glConst face) { GLCHECK(::glCullFace(face)); }
void GLFunctions::glPixelStore(glConst name, uint32_t value)
{
  GLCHECK(::glPixelStorei(name, value));
}

int32_t GLFunctions::glGetInteger(glConst pname)
{
  GLint value;
  GLCHECK(::glGetIntegerv(pname, &value));
  return (int32_t)value;
}

std::string GLFunctions::glGetString(glConst pname)
{
  char const * str = reinterpret_cast<char const *>(::glGetString(pname));
  GLCHECKCALL();
  if (str == nullptr)
    return "";

  return std::string(str);
}

int32_t GLFunctions::glGetMaxLineWidth()
{
  GLint range[2];
  GLCHECK(::glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, range));
  return std::max(range[0], range[1]);
}

int32_t GLFunctions::glGetBufferParameter(glConst target, glConst name)
{
  GLint result;
  ASSERT(glGetBufferParameterFn != nullptr, ());
  GLCHECK(glGetBufferParameterFn(target, name, &result));
  return static_cast<int32_t>(result);
}

void GLFunctions::glEnable(glConst mode)
{
  GLCHECK(::glEnable(mode));
}

void GLFunctions::glDisable(glConst mode)
{
  GLCHECK(::glDisable(mode));
}

void GLFunctions::glClearDepthValue(double depth)
{
#if defined(OMIM_OS_IPHONE) || defined(OMIM_OS_ANDROID)
  GLCHECK(::glClearDepthf(static_cast<GLclampf>(depth)));
#else
  GLCHECK(::glClearDepth(depth));
#endif
}

void GLFunctions::glDepthMask(bool needWriteToDepthBuffer)
{
  GLCHECK(::glDepthMask(convert(needWriteToDepthBuffer)));
}

void GLFunctions::glDepthFunc(glConst depthFunc) { GLCHECK(::glDepthFunc(depthFunc)); }
void GLFunctions::glBlendEquation(glConst function)
{
  ASSERT(glBlendEquationFn != nullptr, ());
  GLCHECK(glBlendEquationFn(function));
}

void GLFunctions::glBlendFunc(glConst srcFactor, glConst dstFactor)
{
  GLCHECK(::glBlendFunc(srcFactor, dstFactor));
}

uint32_t GLFunctions::glGenVertexArray()
{
  ASSERT(glGenVertexArraysFn != nullptr, ());
  GLuint result = 0;
  GLCHECK(glGenVertexArraysFn(1, &result));
  return result;
}

void GLFunctions::glBindVertexArray(uint32_t vao)
{
  ASSERT(glBindVertexArrayFn != nullptr, ());
  GLCHECK(glBindVertexArrayFn(vao));
}

void GLFunctions::glDeleteVertexArray(uint32_t vao)
{
  ASSERT(glDeleteVertexArrayFn != nullptr, ());
  GLCHECK(glDeleteVertexArrayFn(1, &vao));
}

uint32_t GLFunctions::glGenBuffer()
{
  ASSERT(glGenBuffersFn != nullptr, ());
  GLuint result = (GLuint)-1;
  GLCHECK(glGenBuffersFn(1, &result));
  return result;
}

void GLFunctions::glBindBuffer(uint32_t vbo, uint32_t target)
{
  ASSERT(glBindBufferFn != nullptr, ());
#ifdef DEBUG
  std::lock_guard<std::mutex> guard(g_boundBuffersMutex);
  g_boundBuffers[std::make_pair(std::this_thread::get_id(), target)] = vbo;
#endif
  GLCHECK(glBindBufferFn(target, vbo));
}

void GLFunctions::glDeleteBuffer(uint32_t vbo)
{
  ASSERT(glDeleteBuffersFn != nullptr, ());
#ifdef DEBUG
  std::lock_guard<std::mutex> guard(g_boundBuffersMutex);
  for (auto const & n : g_boundBuffers)
    ASSERT(n.second != vbo, ());
#endif
  GLCHECK(glDeleteBuffersFn(1, &vbo));
}

void GLFunctions::glBufferData(glConst target, uint32_t size, void const * data, glConst usage)
{
  ASSERT(glBufferDataFn != nullptr, ());
  GLCHECK(glBufferDataFn(target, size, data, usage));
}

void GLFunctions::glBufferSubData(glConst target, uint32_t size, void const * data, uint32_t offset)
{
  ASSERT(glBufferSubDataFn != nullptr, ());
  GLCHECK(glBufferSubDataFn(target, offset, size, data));
}

void * GLFunctions::glMapBuffer(glConst target, glConst access)
{
  ASSERT(glMapBufferFn != nullptr, ());
  void * result = glMapBufferFn(target, access);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glUnmapBuffer(glConst target)
{
  ASSERT(glUnmapBufferFn != nullptr, ());
  VERIFY(glUnmapBufferFn(target) == GL_TRUE, ());
  GLCHECKCALL();
}

void * GLFunctions::glMapBufferRange(glConst target, uint32_t offset, uint32_t length,
                                     glConst access)
{
  ASSERT(glMapBufferRangeFn != nullptr, ());
  void * result = glMapBufferRangeFn(target, offset, length, access);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glFlushMappedBufferRange(glConst target, uint32_t offset, uint32_t length)
{
  ASSERT(glFlushMappedBufferRangeFn != nullptr, ());
  GLCHECK(glFlushMappedBufferRangeFn(target, offset, length));
}

uint32_t GLFunctions::glCreateShader(glConst type)
{
  ASSERT(glCreateShaderFn != nullptr, ());
  GLuint result = glCreateShaderFn(type);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glShaderSource(uint32_t shaderID, std::string const & src, std::string const & defines)
{
  ASSERT(glShaderSourceFn != nullptr, ());
  
  std::string fullSrc;
  if (src.find("#version") != std::string::npos)
  {
    auto pos = src.find('\n');
    ASSERT_NOT_EQUAL(pos, std::string::npos, ());
    fullSrc = src;
    fullSrc.insert(pos + 1, defines);
  }
  else
  {
    fullSrc = defines + src;
  }
  
  GLchar const * source[1] = {fullSrc.c_str()};
  GLint lengths[1] = {static_cast<GLint>(fullSrc.size())};
  GLCHECK(glShaderSourceFn(shaderID, 1, source, lengths));
}

bool GLFunctions::glCompileShader(uint32_t shaderID, std::string & errorLog)
{
  ASSERT(glCompileShaderFn != nullptr, ());
  ASSERT(glGetShaderivFn != nullptr, ());
  ASSERT(glGetShaderInfoLogFn != nullptr, ());
  GLCHECK(glCompileShaderFn(shaderID));

  GLint result = GL_FALSE;
  GLCHECK(glGetShaderivFn(shaderID, GL_COMPILE_STATUS, &result));
  if (result == GL_TRUE)
    return true;

  GLchar buf[1024];
  GLint length = 0;
  GLCHECK(glGetShaderInfoLogFn(shaderID, 1024, &length, buf));
  errorLog = std::string(buf, static_cast<size_t>(length));
  return false;
}

void GLFunctions::glDeleteShader(uint32_t shaderID)
{
  ASSERT(glDeleteShaderFn != nullptr, ());
  GLCHECK(glDeleteBuffersFn(1, &shaderID));
}

uint32_t GLFunctions::glCreateProgram()
{
  ASSERT(glCreateProgramFn != nullptr, ());
  GLuint result = glCreateProgramFn();
  GLCHECKCALL();
  return result;
}

void GLFunctions::glAttachShader(uint32_t programID, uint32_t shaderID)
{
  ASSERT(glAttachShaderFn != nullptr, ());
  GLCHECK(glAttachShaderFn(programID, shaderID));
}

void GLFunctions::glDetachShader(uint32_t programID, uint32_t shaderID)
{
  ASSERT(glDetachShaderFn != nullptr, ());
  GLCHECK(glDetachShaderFn(programID, shaderID));
}

bool GLFunctions::glLinkProgram(uint32_t programID, std::string & errorLog)
{
  ASSERT(glLinkProgramFn != nullptr, ());
  ASSERT(glGetProgramivFn != nullptr, ());
  ASSERT(glGetProgramInfoLogFn != nullptr, ());
  GLCHECK(glLinkProgramFn(programID));

  GLint result = GL_FALSE;
  GLCHECK(glGetProgramivFn(programID, GL_LINK_STATUS, &result));

  if (result == GL_TRUE)
    return true;

  GLchar buf[1024];
  GLint length = 0;
  GLCHECK(glGetProgramInfoLogFn(programID, 1024, &length, buf));
  errorLog = std::string(buf, static_cast<size_t>(length));
  return false;
}

void GLFunctions::glDeleteProgram(uint32_t programID)
{
  ASSERT(glDeleteProgramFn != nullptr, ());
  GLCHECK(glDeleteProgramFn(programID));
}

void GLFunctions::glUseProgram(uint32_t programID)
{
  ASSERT(glUseProgramFn != nullptr, ());
  GLCHECK(glUseProgramFn(programID));
}

int8_t GLFunctions::glGetAttribLocation(uint32_t programID, std::string const & name)
{
  ASSERT(glGetAttribLocationFn != nullptr, ());
  int result = glGetAttribLocationFn(programID, name.c_str());
  GLCHECKCALL();
  ASSERT(result != -1, ());
  return static_cast<int8_t>(result);
}

void GLFunctions::glBindAttribLocation(uint32_t programID, uint8_t index, std::string const & name)
{
  ASSERT(glBindAttribLocationFn != nullptr, ());
  GLCHECK(glBindAttribLocationFn(programID, index, name.c_str()));
}

void GLFunctions::glEnableVertexAttribute(int attributeLocation)
{
  ASSERT(glEnableVertexAttributeFn != nullptr, ());
  GLCHECK(glEnableVertexAttributeFn(attributeLocation));
}

void GLFunctions::glVertexAttributePointer(int attrLocation, uint32_t count, glConst type,
                                           bool needNormalize, uint32_t stride, uint32_t offset)
{
  ASSERT(glVertexAttributePointerFn != nullptr, ());
  GLCHECK(glVertexAttributePointerFn(attrLocation, count, type, convert(needNormalize), stride,
                                     reinterpret_cast<void *>(offset)));
}

void GLFunctions::glGetActiveUniform(uint32_t programID, uint32_t uniformIndex,
                                     int32_t * uniformSize, glConst * type, std::string & name)
{
  ASSERT(glGetActiveUniformFn != nullptr, ());
  GLchar buff[256];
  GLCHECK(glGetActiveUniformFn(programID, uniformIndex, ARRAY_SIZE(buff), nullptr, uniformSize,
                               type, buff));
  name = buff;
}

int8_t GLFunctions::glGetUniformLocation(uint32_t programID, std::string const & name)
{
  ASSERT(glGetUniformLocationFn != nullptr, ());
  int result = glGetUniformLocationFn(programID, name.c_str());
  GLCHECKCALL();
  ASSERT(result != -1, ());
  return static_cast<int8_t>(result);
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v)
{
  ASSERT(glUniform1iFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1iFn(location, v));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2)
{
  ASSERT(glUniform2iFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform2iFn(location, v1, v2));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3)
{
  ASSERT(glUniform3iFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform3iFn(location, v1, v2, v3));
}

void GLFunctions::glUniformValuei(int8_t location, int32_t v1, int32_t v2, int32_t v3, int32_t v4)
{
  ASSERT(glUniform4iFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform4iFn(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformValueiv(int8_t location, int32_t * v, uint32_t size)
{
  ASSERT(glUniform1ivFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1ivFn(location, size, v));
}

void GLFunctions::glUniformValuef(int8_t location, float v)
{
  ASSERT(glUniform1fFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1fFn(location, v));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2)
{
  ASSERT(glUniform2fFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform2fFn(location, v1, v2));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3)
{
  ASSERT(glUniform3fFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform3fFn(location, v1, v2, v3));
}

void GLFunctions::glUniformValuef(int8_t location, float v1, float v2, float v3, float v4)
{
  ASSERT(glUniform4fFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform4fFn(location, v1, v2, v3, v4));
}

void GLFunctions::glUniformValuefv(int8_t location, float * v, uint32_t size)
{
  ASSERT(glUniform1fvFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniform1fvFn(location, size, v));
}

void GLFunctions::glUniformMatrix4x4Value(int8_t location, float const * values)
{
  ASSERT(glUniformMatrix4fvFn != nullptr, ());
  ASSERT(location != -1, ());
  GLCHECK(glUniformMatrix4fvFn(location, 1, GL_FALSE, values));
}

uint32_t GLFunctions::glGetCurrentProgram()
{
  GLint programIndex = 0;
  GLCHECK(glGetIntegerv(GL_CURRENT_PROGRAM, &programIndex));
  ASSERT_GREATER_OR_EQUAL(programIndex, 0, ());
  return static_cast<uint32_t>(programIndex);
}

int32_t GLFunctions::glGetProgramiv(uint32_t program, glConst paramName)
{
  ASSERT(glGetProgramivFn != nullptr, ());
  GLint paramValue = 0;
  GLCHECK(glGetProgramivFn(program, paramName, &paramValue));
  return paramValue;
}

void GLFunctions::glActiveTexture(glConst texBlock)
{
  ASSERT(glActiveTextureFn != nullptr, ());
  GLCHECK(glActiveTextureFn(texBlock));
}

uint32_t GLFunctions::glGenTexture()
{
  GLuint result = 0;
  GLCHECK(::glGenTextures(1, &result));
  return result;
}

void GLFunctions::glDeleteTexture(uint32_t id)
{
  GLCHECK(::glDeleteTextures(1, &id));
}

void GLFunctions::glBindTexture(uint32_t textureID)
{
  GLCHECK(::glBindTexture(GL_TEXTURE_2D, textureID));
}

void GLFunctions::glTexImage2D(int width, int height, glConst layout, glConst pixelType,
                               void const * data)
{
  // In OpenGL ES3:
  // - we can't create unsized GL_RED texture, so we use GL_R8;
  // - we can't create unsized GL_DEPTH_COMPONENT texture, so we use GL_DEPTH_COMPONENT16
  //   or GL_DEPTH_COMPONENT24 or GL_DEPTH_COMPONENT32F.
  glConst internalFormat = layout;
  if (CurrentApiVersion == dp::ApiVersion::OpenGLES3)
  {
    if (layout == gl_const::GLRed)
    {
      internalFormat = GL_R8;
    }
    else if (layout == gl_const::GLDepthComponent)
    {
      internalFormat = GL_DEPTH_COMPONENT16;
      if (pixelType == gl_const::GLUnsignedIntType)
        internalFormat = GL_DEPTH_COMPONENT24;
      else if (pixelType == gl_const::GLFloatType)
        internalFormat = GL_DEPTH_COMPONENT32F;
    }
  }

  GLCHECK(
      ::glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, layout, pixelType, data));
}

void GLFunctions::glTexSubImage2D(int x, int y, int width, int height, glConst layout,
                                  glConst pixelType, void const * data)
{
  GLCHECK(::glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, width, height, layout, pixelType, data));
}

void GLFunctions::glTexParameter(glConst param, glConst value)
{
  GLCHECK(::glTexParameteri(GL_TEXTURE_2D, param, value));
}

void GLFunctions::glDrawElements(glConst primitive, uint32_t sizeOfIndex, uint32_t indexCount,
                                 uint32_t startIndex)
{
  GLCHECK(::glDrawElements(primitive, indexCount,
                           sizeOfIndex == sizeof(uint32_t) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT,
                           reinterpret_cast<GLvoid *>(startIndex * sizeOfIndex)));
}

void GLFunctions::glDrawArrays(glConst mode, int32_t first, uint32_t count)
{
  GLCHECK(::glDrawArrays(mode, first, count));
}

void GLFunctions::glGenFramebuffer(uint32_t * fbo)
{
  ASSERT(glGenFramebuffersFn != nullptr, ());
  GLCHECK(glGenFramebuffersFn(1, fbo));
}

void GLFunctions::glDeleteFramebuffer(uint32_t * fbo)
{
  ASSERT(glDeleteFramebuffersFn != nullptr, ());
  GLCHECK(glDeleteFramebuffersFn(1, fbo));
}

void GLFunctions::glFramebufferTexture2D(glConst attachment, glConst texture)
{
  ASSERT(glFramebufferTexture2DFn != nullptr, ());
  GLCHECK(glFramebufferTexture2DFn(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0));
}

void GLFunctions::glBindFramebuffer(uint32_t fbo)
{
  ASSERT(glBindFramebufferFn != nullptr, ());
  GLCHECK(glBindFramebufferFn(GL_FRAMEBUFFER, fbo));
}

uint32_t GLFunctions::glCheckFramebufferStatus()
{
  ASSERT(glCheckFramebufferStatusFn != nullptr, ());
  uint32_t const result = glCheckFramebufferStatusFn(GL_FRAMEBUFFER);
  GLCHECKCALL();
  return result;
}

void GLFunctions::glLineWidth(uint32_t value)
{
  GLCHECK(::glLineWidth(static_cast<float>(value)));
}

namespace
{
std::string GetGLError(GLenum error)
{
  switch (error)
  {
  case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
  case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
  case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
  case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
  default: return strings::to_string(error);
  }
}
}  // namespace

void CheckGLError(my::SrcPoint const & srcPoint)
{
  GLenum result = glGetError();
  while (result != GL_NO_ERROR)
  {
    LOG(LINFO, ("SrcPoint ", srcPoint, "GLError:", GetGLError(result)));
    result = glGetError();
  }
}
