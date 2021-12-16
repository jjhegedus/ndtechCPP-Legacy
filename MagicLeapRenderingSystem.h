#pragma once

#include "pch.h"
#include "BaseApp.h"
#include "GraphicsContext.h"
#include "TypeUtilities.h"
#include "VertexTypes.h"

#include <chrono>
#include <vector>
#include <ml_graphics.h>
#include "Utilities.h"

namespace ndtech
{
  template <typename TSettings>
  class RenderingSystem
  {

    template <typename TestType, typename AppType>
    using TestTypeHasRenderComponentsImpl = decltype(
      std::declval<TestType>().RenderComponents(
        std::declval<RenderingSystem<TSettings>*>(),
        std::declval<AppType*>(),
        0
      )
      );

    template <typename TestType, typename AppType>
    using TestTypeHasRenderComponents = ndtech::TypeUtilities::is_detected<TestTypeHasRenderComponentsImpl, TestType, AppType>;

    using Settings = TSettings;
    using Components = typename Settings::Components;
    using ComponentSystems = typename Settings::ComponentSystems;

    using ComponentSystemsTuple = TypeUtilities::Convert<ComponentSystems, std::tuple>;
    int numberOfComponentSystems = std::tuple_size<ComponentSystemsTuple>::value;

    using ComponentVectors = TypeUtilities::Convert<Components, TypeUtilities::TupleOfVectors>;
    int numberOfComponentVectors = std::tuple_size<ComponentVectors>::value;

    using EntityIndexType = typename Settings::EntityIndexType;
    using EntityType = typename Settings::EntityType;
    using EntityVector = std::vector<EntityType>;

    using VertexType = ndtech::vertexTypes::VertexPositionColor;

    NamedItemStore<std::string, std::wstring> m_shaderFileDataStore;
    boost::fibers::fiber m_shaderFileDataStoreFiber;

    NamedItemStore< int, std::pair<std::wstring, std::wstring> > m_shaderProgramStore;
    boost::fibers::fiber m_shaderProgramStoreFiber;

    NamedItemStore< GLuint, std::wstring > m_vertexShaderStore;
    boost::fibers::fiber m_vertexShaderStoreFiber;

    NamedItemStore< GLuint, std::wstring > m_fragmentShaderStore;
    boost::fibers::fiber m_fragmentShaderStoreFiber;

    template <typename AppType, typename ComponentSystemType>
    void RenderComponentSystem(AppType* app, int cameraIndex) {

      if constexpr (TestTypeHasRenderComponents<ComponentSystemType, AppType>{}) {

        ComponentSystemType& componentSystem = std::get<ComponentSystemType>(*this->m_componentSystems);

        if constexpr (TestTypeHasPreRenderComponentSystem<ComponentSystemType>{}) {
          componentSystem.PreRenderComponentSystem(this->m_app);
        }

        componentSystem.RenderComponents(this, app, cameraIndex);
      }

    }

    template <typename AppType, typename... ComponentSystemTypes>
    void RenderComponentSystems(AppType* app, ndtech::TypeUtilities::Typelist<ComponentSystemTypes...> typelist, int cameraIndex) {
      (RenderComponentSystem<AppType, ComponentSystemTypes>(app, cameraIndex), ...);
    }

  public:
    RenderingSystem() = default;
    RenderingSystem(ComponentSystemsTuple* componentSystems) {
      m_componentSystems = componentSystems;
    };
    ~RenderingSystem() {

      m_vertexShaderStore.Stop();
      m_vertexShaderStoreFiber.join();

      m_fragmentShaderStore.Stop();
      m_fragmentShaderStoreFiber.join();

      m_shaderProgramStore.Stop();
      m_shaderProgramStoreFiber.join();

      m_shaderFileDataStore.Stop();
      m_shaderFileDataStoreFiber.join();

      graphics_context.unmakeCurrent();
      glDeleteFramebuffers(1, &graphics_context.framebuffer_id);
      MLGraphicsDestroyClient(&graphics_client);

    };

    RenderingSystem & operator=(const RenderingSystem&) = default;
    RenderingSystem(const RenderingSystem&) = default;
    RenderingSystem(RenderingSystem &&) = default;

    ComponentSystemsTuple* m_componentSystems;
    std::vector<EntityIndexType>* m_freeComponentIndices;
    ComponentVectors* m_componentVectors;
    BaseApp* m_app;

    MLGraphicsOptions graphics_options = { 0, MLSurfaceFormat_RGBA8UNorm, MLSurfaceFormat_D32Float };
    MLHandle graphics_client = ML_INVALID_HANDLE;
    unsigned int m_shaderProgram;

    GLuint m_vao;
    GLuint m_vbo;

    GraphicsContext graphics_context;

    std::chrono::time_point<std::chrono::steady_clock> start;

    MLGraphicsVirtualCameraInfoArray m_virtual_camera_array;

#if USE_GLFW
    MLHandle opengl_context = reinterpret_cast<MLHandle>(glfwGetCurrentContext());
#endif

#if ML_DEVICE
    MLHandle opengl_context = reinterpret_cast<MLHandle>(graphics_context.egl_context);
#endif

    void Initialize() {

      m_shaderFileDataStore.RunOn(m_shaderFileDataStoreFiber);
      m_shaderProgramStore.RunOn(m_shaderProgramStoreFiber);
      m_fragmentShaderStore.RunOn(m_fragmentShaderStoreFiber);
      m_vertexShaderStore.RunOn(m_vertexShaderStoreFiber);

      // Get ready to connect our GL context to the MLSDK graphics API
      graphics_context.makeCurrent();
      glGenFramebuffers(1, &graphics_context.framebuffer_id);

      MLGraphicsCreateClientGL(&graphics_options, opengl_context, &graphics_client);

    };

    template<typename AppType>
    void Render(AppType* app) {

      MLGraphicsFrameParams frame_params;

      MLResult out_result = MLGraphicsInitFrameParams(&frame_params);
      if (MLResult_Ok != out_result) {
        LOG(FATAL) << "MLGraphicsInitFrameParams complained: " << out_result;
      }
      frame_params.surface_scale = 1.0f;
      frame_params.projection_type = MLGraphicsProjectionType_ReversedInfiniteZ;
      frame_params.near_clip = 1.0f;
      frame_params.focus_distance = 1.0f;

      MLHandle frame_handle;
      out_result = MLGraphicsBeginFrame(graphics_client, &frame_params, &frame_handle, &m_virtual_camera_array);
      if (MLResult_Ok != out_result) {
        LOG(FATAL) << "MLGraphicsBeginFrame complained: " << out_result;
      }

      glFrontFace(GL_CW);
      glEnable(GL_CULL_FACE);

      for (int cameraIndex = 0; cameraIndex < 2; ++cameraIndex) {
        glBindFramebuffer(GL_FRAMEBUFFER, graphics_context.framebuffer_id);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_virtual_camera_array.color_id, 0, cameraIndex);
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_virtual_camera_array.depth_id, 0, cameraIndex);

        const MLRectf& viewport = m_virtual_camera_array.viewport;
        glViewport((GLint)viewport.x, (GLint)viewport.y,
          (GLsizei)viewport.w, (GLsizei)viewport.h);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //RenderComponentSystems(0, app);
        RenderComponentSystems(app, ComponentSystems{}, cameraIndex);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        MLGraphicsSignalSyncObjectGL(graphics_client, m_virtual_camera_array.virtual_cameras[cameraIndex].sync_object);
      }
      out_result = MLGraphicsEndFrame(graphics_client, frame_handle);
      if (MLResult_Ok != out_result) {
        LOG(FATAL) << "MLGraphicsEndFrame complained: " << out_result;
      }

      graphics_context.swapBuffers();

    };
  
    void LinkAndCheckShaderProgram(unsigned int shaderProgramId, GLuint vertexShaderId, GLuint fragmentShaderId) {

      // Link the program
      GLint result = GL_FALSE;
      int infoLogLength = 0;
      glAttachShader(shaderProgramId, vertexShaderId);
      glAttachShader(shaderProgramId, fragmentShaderId);
      glLinkProgram(shaderProgramId);

      // Check the program
      glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &result);
      glGetProgramiv(shaderProgramId, GL_INFO_LOG_LENGTH, &infoLogLength);
      if (infoLogLength > 0) {
        std::vector<char> programErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(shaderProgramId, infoLogLength, NULL, &programErrorMessage[0]);
        LOGF(INFO, "%s\n", &programErrorMessage[0]);
      }

    }

    void CompileAndCheckShader(GLuint shaderId, std::string shaderCode) {
      GLint result = GL_FALSE;
      int infoLogLength = 0;

      // Compile Vertex Shader
      char const * shaderSourcePointer = shaderCode.c_str();
      glShaderSource(shaderId, 1, &shaderSourcePointer, NULL);
      glCompileShader(shaderId);

      // Check Vertex Shader
      glGetShaderiv(shaderId, GL_COMPILE_STATUS, &result);
      glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
      if (infoLogLength > 0) {
        std::vector<char> shaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(shaderId, infoLogLength, NULL, &shaderErrorMessage[0]);
        LOGF(INFO, "Checking Shader: %s", &shaderErrorMessage[0]);
      }
    }

    unsigned int GetShaderProgramId(std::wstring vertexShaderFilePath, std::wstring fragmentShaderFilePath) {
      
      if (!m_shaderProgramStore.ItemExists(std::make_pair(vertexShaderFilePath, fragmentShaderFilePath))) {

        GLuint vertexShaderId = 0;
        std::string vertexShaderCode;
        if (!m_vertexShaderStore.ItemExists(vertexShaderFilePath)) {

          if (!m_shaderFileDataStore.ItemExists(vertexShaderFilePath)) {
            std::string vertexShaderCode = ndtech::utilities::ReadSmallTextFileSync(vertexShaderFilePath);
            m_shaderFileDataStore.AddItem(vertexShaderFilePath, vertexShaderCode);
          }

          vertexShaderCode = m_shaderFileDataStore.GetItem(vertexShaderFilePath);
          GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);

          m_vertexShaderStore.AddItem(vertexShaderFilePath, vertexShaderId);
        }
        vertexShaderId = m_vertexShaderStore.GetItem(vertexShaderFilePath);
        CompileAndCheckShader(vertexShaderId, vertexShaderCode);


        GLuint fragmentShaderId = 0;
        std::string fragmentShaderCode;
        if (!m_fragmentShaderStore.ItemExists(fragmentShaderFilePath)) {

          if (!m_shaderFileDataStore.ItemExists(fragmentShaderFilePath)) {
            std::string fragmentShaderCode = ndtech::utilities::ReadSmallTextFileSync(fragmentShaderFilePath);
            m_shaderFileDataStore.AddItem(fragmentShaderFilePath, fragmentShaderCode);
          }

          fragmentShaderCode = m_shaderFileDataStore.GetItem(fragmentShaderFilePath);
          GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

          m_fragmentShaderStore.AddItem(fragmentShaderFilePath, fragmentShaderId);
        }
        fragmentShaderId = m_fragmentShaderStore.GetItem(fragmentShaderFilePath);
        CompileAndCheckShader(fragmentShaderId, fragmentShaderCode);


        unsigned int shaderProgramId = glCreateProgram();
        LinkAndCheckShaderProgram(shaderProgramId, vertexShaderId, fragmentShaderId);

        // Detach and delete the vertexShader and fragmentShader since they are no longer needed
        glDetachShader(shaderProgramId, vertexShaderId);
        glDetachShader(shaderProgramId, fragmentShaderId);
        glDeleteShader(vertexShaderId);
        glDeleteShader(fragmentShaderId);

        m_shaderProgramStore.AddItem(std::make_pair(vertexShaderFilePath, fragmentShaderFilePath), shaderProgramId);
      }
      
      // TODO:  The line below will hang if there is an error that does not throw.  I need to add a timeout to GetItem that throws to avoid long delays
      return m_shaderProgramStore.GetItem(std::make_pair(vertexShaderFilePath, fragmentShaderFilePath));

    }

};
}