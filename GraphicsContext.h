#pragma once

#if ML_DEVICE
#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#endif

#if USE_GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

namespace ndtech {


  struct GraphicsContext {
#if USE_GLFW
    GLFWwindow* window;
#endif 

#if ML_DEVICE
    EGLDisplay egl_display;
    EGLContext egl_context;
#endif

#if NDTECH_ML
    GLuint framebuffer_id;
    GLuint vertex_shader_id;
    GLuint fragment_shader_id;
    GLuint program_id;
#endif

    GraphicsContext();
    ~GraphicsContext();

    void makeCurrent();
    void swapBuffers();
    void unmakeCurrent();
  };

}
