#pragma once



#if ML_DEVICE

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <GLES3/gl31.h>
#include <GLES3/gl3ext.h>
#endif

#if USE_GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#if NDTECH_ML

#ifndef EGL_EGLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES
#endif

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <ml_graphics.h>
#include <ml_head_tracking.h>
#include <ml_perception.h>
#include <ml_lifecycle.h>
#include <ml_logging.h>

using byte = unsigned char;
#if NDTECH_WIN
#define _ENABLE_EXTENDED_ALIGNED_STORAGE = 1
#endif
#endif

#if NDTECH_WIN
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING = 1
#endif

#if NDTECH_HOLO
#include <array>
#include <d2d1_2.h>
#include <d3d11_4.h>
#include <DirectXColors.h>
#include <dwrite_2.h>
#include <future>
#include <map>
#include <mutex>
#include <vector>
#include <wincodec.h>
#include <WindowsNumerics.h>

#include <Windows.Graphics.Directx.Direct3D11.Interop.h>
#include <wrl\client.h>

#include <winrt\base.h>
#include <winrt\Windows.ApplicationModel.Activation.h>
#include <winrt\Windows.ApplicationModel.Core.h>
#include <winrt\Windows.Foundation.h>
#include <winrt\Windows.Foundation.Collections.h>
#include <winrt\Windows.Foundation.Metadata.h>
#include <winrt\Windows.Gaming.Input.h>
#include <winrt\Windows.Graphics.Display.h>
#include <winrt\Windows.Graphics.Holographic.h>
#include <winrt\Windows.Perception.People.h>
#include <winrt\Windows.Perception.Spatial.h>
#include <winrt\Windows.Storage.h>
#include <winrt\Windows.Storage.Streams.h>
#include <winrt\Windows.UI.Core.h>
#include <winrt\Windows.UI.Input.Spatial.h>

#include <exception>


#include <TraceLoggingProvider.h>

TRACELOGGING_DECLARE_PROVIDER(traceProvider);

#define TraceLogWrite(eventName, ...) \
    _TlgWrite_imp(_TlgWrite, \
    traceProvider, eventName, \
    (NULL, NULL), \
    __VA_ARGS__)

#endif 

#include <stdio.h>
#include <stdlib.h>

#include <chrono>
#include <cmath>

#include <g3log/g3log.hpp>

#include <runtime/external/glm/glm/glm.hpp>
#include <runtime/external/glm/glm/gtx/quaternion.hpp>
#include <runtime/external/glm/glm/gtx/transform.hpp>
#include <runtime/external/glm/glm/gtc/type_ptr.hpp>

#define NDTECH_CORE_FWD(...) ::std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)
#define NDTECH_FWD(...) NDTECH_CORE_FWD(__VA_ARGS__)