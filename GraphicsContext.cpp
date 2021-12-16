#include "GraphicsContext.h"
#include "pch.h"
#include <g3log/g3log.hpp>

#if ML_DEVICE
#include <GLES/gl.h>
#include <GLES2/gl2ext.h>
#endif

#if NDTECH_ML

static void on_gl_error(GLenum source, GLenum type, GLuint id, GLenum severity,
  GLsizei length, const GLchar* message, const GLvoid *userParam)
{
  //GL_DEBUG_SEVERITY_LOW, GL_DEBUG_SEVERITY_MEDIUM, GL_DEBUG_SEVERITY_HIGH, or GL_DEBUG_SEVERITY_NOTIFICATION

#if ML_DEVICE

  if (severity == GL_DEBUG_SEVERITY_HIGH_KHR) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - HIGH: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_MEDIUM_KHR) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - MEDIUM: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_LOW_KHR) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - LOW: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION_KHR) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - NOTIFICATION: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - WTF: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
#else

  if (severity == GL_DEBUG_SEVERITY_HIGH) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - HIGH: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - MEDIUM: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_LOW) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - LOW: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - NOTIFICATION: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
  else {
    LOGF(INFO, "\nsource: %x\ntype: %x\nid: %d\nseverity - WTF: %x\nlength: %d\nmessage: %s\n", source, type, id, severity, length, message);
  }
#endif

}
#endif

#if USE_GLFW
ndtech::GraphicsContext::GraphicsContext() {
  if (!glfwInit()) {
    //ND_LOG(Error, "glfwInit() failed%s", ":");
    exit(1);
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
#if ML_OSX
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
#else
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

  // We open a 1x1 window here -- this is not where the action happens, however;
  // this program renders to a texture.  This is done merely to make GL happy.
  window = glfwCreateWindow(1, 1, "rendering test", NULL, NULL);
  if (!window) {
    //ND_LOG(Error, "glfwCreateWindow() failed%s", ":");
    exit(1);
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    //ND_LOG(Error, "gladLoadGLLoader() failed%s", ":");
    exit(1);
  }

  GLint v;
  glGetIntegerv(GL_CONTEXT_FLAGS, &v);
  if (v & GL_CONTEXT_FLAG_DEBUG_BIT) {
    LOG(INFO) << "OpenGL Debug Context Active";

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(on_gl_error, NULL);
  }

}

void ndtech::GraphicsContext::makeCurrent() {
  glfwMakeContextCurrent(window);
}

void ndtech::GraphicsContext::unmakeCurrent() {
}

void ndtech::GraphicsContext::swapBuffers() {
  glfwSwapBuffers(window);
}

ndtech::GraphicsContext::~GraphicsContext() {
  glfwDestroyWindow(window);
  window = nullptr;
}
#endif

#if ML_DEVICE

ndtech::GraphicsContext::GraphicsContext() {
  egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  EGLint major = 4;
  EGLint minor = 0;
  eglInitialize(egl_display, &major, &minor);
  eglBindAPI(EGL_OPENGL_API);

  EGLint config_attribs[] = {
    EGL_RED_SIZE, 5,
    EGL_GREEN_SIZE, 6,
    EGL_BLUE_SIZE, 5,
    EGL_ALPHA_SIZE, 0,
    EGL_DEPTH_SIZE, 24,
    EGL_STENCIL_SIZE, 8,
    EGL_NONE
  };
  EGLConfig egl_config = nullptr;
  EGLint config_size = 0;
  eglChooseConfig(egl_display, config_attribs, &egl_config, 1, &config_size);

  EGLint context_attribs[] = {
    EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
    EGL_CONTEXT_MINOR_VERSION_KHR, 0,
    EGL_CONTEXT_FLAGS_KHR, EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR,
    EGL_NONE
  };
  egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);



  GLint v;
  glGetIntegerv(EGL_CONTEXT_FLAGS_KHR, &v);
  if (v & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR) {
    LOG(INFO) << "OpenGL Debug Context Active";

    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_KHR);
    glDebugMessageCallbackKHR(on_gl_error, NULL);
  }
}

void ndtech::GraphicsContext::makeCurrent() {
  eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
}

void ndtech::GraphicsContext::unmakeCurrent() {
  eglMakeCurrent(NULL, EGL_NO_SURFACE, EGL_NO_SURFACE, NULL);
}

void ndtech::GraphicsContext::swapBuffers() {
  // buffer swapping is implicit on device (MLGraphicsEndFrame)
}

ndtech::GraphicsContext::~GraphicsContext() {
  eglDestroyContext(egl_display, egl_context);
  eglTerminate(egl_display);
}

#endif

#if NDTECH_HOLO

ndtech::GraphicsContext::GraphicsContext() {

}

void ndtech::GraphicsContext::makeCurrent() {

}

void ndtech::GraphicsContext::unmakeCurrent() {
}

void ndtech::GraphicsContext::swapBuffers() {

}

ndtech::GraphicsContext::~GraphicsContext() {

}
#endif 
