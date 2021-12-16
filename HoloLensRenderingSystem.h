#pragma once

#include "BaseApp.h"
#include "DeviceResources.h"
#include "SpatialInputHandler.h"
#include <vector>
#include <map>
#include "VertexTypes.h"
#include "NamedItemStore.h"
#include <boost/fiber/all.hpp>
#include "Utilities.h"

namespace ndtech
{
  template <typename TSettings>
  class RenderingSystem : public IDeviceNotify
  {

    template <typename TestType, typename AppType>
    using TestTypeHasRenderComponentsImpl = decltype(
      std::declval<TestType>().RenderComponents(
        std::declval<RenderingSystem<TSettings>*>(),
        std::declval<AppType*>()
      )
      );

    template <typename TestType, typename AppType>
    using TestTypeHasRenderComponents = ndtech::TypeUtilities::is_detected<TestTypeHasRenderComponentsImpl, TestType, AppType>;

    using Settings = TSettings;
    using Components = typename Settings::Components;
    using ComponentSystems = typename Settings::ComponentSystems;
    using ShaderTypes = typename Settings::ShaderTypes;

    using ComponentSystemsTuple = TypeUtilities::Convert<ComponentSystems, std::tuple>;
    int numberOfComponentSystems = std::tuple_size<ComponentSystemsTuple>::value;

    using ComponentVectors = TypeUtilities::Convert<Components, TypeUtilities::TupleOfVectors>;
    int numberOfComponentVectors = std::tuple_size<ComponentVectors>::value;

    using EntityIndexType = typename Settings::EntityIndexType;
    using EntityType = typename Settings::EntityType;
    using EntityVector = std::vector<EntityType>;

    using ShaderVectorsType = TypeUtilities::ConvertToTupleOfMaps<std::wstring, ShaderTypes>;

    template <typename AppType, typename ComponentSystemType>
    void RenderComponentSystem(AppType* app) {

      if constexpr (TestTypeHasRenderComponents<ComponentSystemType, AppType>{}) {

        ComponentSystemType& componentSystem = std::get<ComponentSystemType>(*this->m_componentSystems);

        if constexpr (TestTypeHasPreRenderComponentSystem<ComponentSystemType>{}) {
          componentSystem.PreRenderComponentSystem(this->m_app);
        }

        componentSystem.RenderComponents(this, app);
      }

    }

    template <typename AppType, typename... ComponentSystemTypes>
    void RenderComponentSystems(AppType* app, ndtech::TypeUtilities::Typelist<ComponentSystemTypes...> typelist) {
      (RenderComponentSystem<AppType, ComponentSystemTypes>(app), ...);
    }

  public:

    // Keep track of gamepads.
    struct GamepadWithButtonState
    {
      winrt::Windows::Gaming::Input::Gamepad gamepad;
      bool buttonAWasPressedLastFrame = false;
    };

    RenderingSystem() = default;
    ~RenderingSystem() {
      m_vertexShadersStore.Stop();
      m_pixelShadersStore.Stop();
      m_geometryShadersStore.Stop();
      m_vertexShadersStoreFiber.join();
      m_pixelShadersStoreFiber.join();
      m_geometryShadersStoreFiber.join();
    };

    RenderingSystem & operator=(const RenderingSystem&) = default;
    RenderingSystem(const RenderingSystem&) = default;
    RenderingSystem(RenderingSystem &&) = default;

    ComponentSystemsTuple* m_componentSystems;
    std::vector<EntityIndexType>* m_freeComponentIndices;
    ComponentVectors* m_componentVectors;
    BaseApp* m_app;

    // Cached pointer to device resources.
    std::shared_ptr<DeviceResources>                        m_deviceResources;

    // The holographic space the app will use for rendering.
    winrt::Windows::Graphics::Holographic::HolographicSpace m_holographicSpace = nullptr;
    winrt::Windows::Graphics::Holographic::HolographicFrame m_holographicFrame = nullptr;

    std::vector<RenderingSystem::GamepadWithButtonState>                         m_gamepads;

    // A stationary reference frame based on m_spatialLocator.
    winrt::Windows::Perception::Spatial::SpatialStationaryFrameOfReference m_stationaryReferenceFrame = nullptr;

    // Keep track of mouse input.
    bool                                                        m_pointerPressed = false;

    // Cache whether or not the HolographicCameraRenderingParameters.CommitDirect3D11DepthBuffer() method can be called.
    bool                                                        m_canCommitDirect3D11DepthBuffer = false;

    winrt::Windows::UI::Input::Spatial::SpatialPointerPose m_pose = nullptr;

    // Listens for the Pressed spatial input event.
    std::shared_ptr<SpatialInputHandler>                        m_spatialInputHandler;

    // Cache whether or not the HolographicCamera.Display property can be accessed.
    bool                                                        m_canGetHolographicDisplayForCamera = false;

    winrt::event_token                                          m_gamepadAddedEventToken;
    winrt::event_token                                          m_gamepadRemovedEventToken;

    // Cache whether or not the HolographicDisplay.GetDefault() method can be called.
    bool                                                        m_canGetDefaultHolographicDisplay = false;

    winrt::event_token                                          m_holographicDisplayIsAvailableChangedEventToken;

    // SpatialLocator that is attached to the default HolographicDisplay.
    winrt::Windows::Perception::Spatial::SpatialLocator         m_spatialLocator = nullptr;
    winrt::event_token                                          m_locatabilityChangedToken;

    // Event registration tokens.
    winrt::event_token                                          m_cameraAddedToken;
    winrt::event_token                                          m_cameraRemovedToken;

    NamedItemStore< winrt::com_ptr<ID3D11VertexShader>, std::wstring > m_vertexShadersStore;
    boost::fibers::fiber m_vertexShadersStoreFiber;

    NamedItemStore< winrt::com_ptr<ID3D11PixelShader>, std::wstring > m_pixelShadersStore;
    boost::fibers::fiber m_pixelShadersStoreFiber;

    NamedItemStore< winrt::com_ptr<ID3D11GeometryShader>, std::wstring > m_geometryShadersStore;
    boost::fibers::fiber m_geometryShadersStoreFiber;


    winrt::Windows::Foundation::Numerics::float3 const& GetFocusPosition() {
      return winrt::Windows::Foundation::Numerics::float3(0, 0, 0);
    };

    template <typename AppType>
    void Render(AppType* app) {
      //
      // TODO: Add code for pre-pass rendering here.
      //
      // Take care of any tasks that are not specific to an individual holographic
      // camera. This includes anything that doesn't need the final view or projection
      // matrix, such as lighting maps.
      //

      // Lock the set of holographic camera resources, then draw to each camera
      // in this frame.
      m_deviceResources->UseHolographicCameraResources<bool>(
        [this, app](std::map<UINT32, std::unique_ptr<CameraResources>>& cameraResourceMap)
        {
          // Up-to-date frame predictions enhance the effectiveness of image stablization and
          // allow more accurate positioning of holograms.
          m_holographicFrame.UpdateCurrentPrediction();
          winrt::Windows::Graphics::Holographic::HolographicFramePrediction prediction = m_holographicFrame.CurrentPrediction();

          bool atLeastOneCameraRendered = false;


          for (winrt::Windows::Graphics::Holographic::HolographicCameraPose const& cameraPose : prediction.CameraPoses())
          {
            TraceLogWrite(
              "ndtech::Render(HolographicSpace)",
              TraceLoggingUInt32(cameraPose.HolographicCamera().Id(), "cameraPose.HolographicCamera().Id()"));

            // This represents the device-based resources for a HolographicCamera.
            CameraResources* pCameraResources = cameraResourceMap[cameraPose.HolographicCamera().Id()].get();

            // Get the device context.
            const auto context = m_deviceResources->GetD3DDeviceContext();
            const auto depthStencilView = pCameraResources->GetDepthStencilView();

            // Set render targets to the current holographic camera.
            ID3D11RenderTargetView *const targets[1] = { pCameraResources->GetBackBufferRenderTargetView() };
            context->OMSetRenderTargets(1, targets, depthStencilView);

            // Clear the back buffer and depth stencil view.
            if (m_canGetHolographicDisplayForCamera &&
              cameraPose.HolographicCamera().Display().IsOpaque())
            {
              context->ClearRenderTargetView(targets[0], DirectX::Colors::CornflowerBlue);
            }
            else
            {
              context->ClearRenderTargetView(targets[0], DirectX::Colors::Transparent);
            }
            context->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

            //
            // TODO: Replace the sample content with your own content.
            //
            // Notes regarding holographic content:
            //    * For drawing, remember that you have the potential to fill twice as many pixels
            //      in a stereoscopic render target as compared to a non-stereoscopic render target
            //      of the same resolution. Avoid unnecessary or repeated writes to the same pixel,
            //      and only draw holograms that the user can see.
            //    * To help occlude hologram geometry, you can create a depth map using geometry
            //      data obtained via the surface mapping APIs. You can use this depth map to avoid
            //      rendering holograms that are intended to be hidden behind tables, walls,
            //      monitors, and so on.
            //    * On HolographicDisplays that are transparent, black pixels will appear transparent 
            //      to the user. On such devices, you should clear the screen to Transparent as shown 
            //      above. You should still use alpha blending to draw semitransparent holograms. 
            //


            // The view and projection matrices for each holographic camera will change
            // every frame. This function refreshes the data in the constant buffer for
            // the holographic camera indicated by cameraPose.
            if (m_stationaryReferenceFrame)
            {
              pCameraResources->UpdateViewProjectionBuffer(m_deviceResources, cameraPose, m_stationaryReferenceFrame.CoordinateSystem());
            }

            // Attach the view/projection constant buffer for this camera to the graphics pipeline.
            bool cameraActive = pCameraResources->AttachViewProjectionBuffer(m_deviceResources);

            // Only render world-locked content when positional tracking is active.
            if (cameraActive)
            {
              //std::string componentSystemName = typeid(ComponentSystems).name();

              RenderComponentSystems(app, ComponentSystems{});

              if (m_canCommitDirect3D11DepthBuffer)
              {
                using namespace winrt::Windows::Graphics::Holographic;
                // On versions of the platform that support the CommitDirect3D11DepthBuffer API, we can 
                // provide the depth buffer to the system, and it will use depth information to stabilize 
                // the image at a per-pixel level.
                HolographicCameraRenderingParameters renderingParameters = m_holographicFrame.GetRenderingParameters(cameraPose);

                //DX::IDirect3DSurface interopSurface = DX::CreateDepthTextureInteropObject(pCameraResources->GetDepthStencilTexture2D());

                //// Calling CommitDirect3D11DepthBuffer causes the system to queue Direct3D commands to 
                //// read the depth buffer. It will then use that information to stabilize the image as
                //// the HolographicFrame is presented.
                //renderingParameters.CommitDirect3D11DepthBuffer(interopSurface);
              }
            }

            atLeastOneCameraRendered = true;
          }

          return atLeastOneCameraRendered;
        });

      // The holographic frame has an API that presents the swap chain for each
      // holographic camera.
      m_deviceResources->Present(m_holographicFrame);
    };


    // Used to be aware of gamepads that are plugged in after the app starts.
    void OnGamepadAdded(winrt::Windows::Foundation::IInspectable, winrt::Windows::Gaming::Input::Gamepad const& args) {

      for (GamepadWithButtonState const& gamepadWithButtonState : m_gamepads)
      {
        if (args == gamepadWithButtonState.gamepad)
        {
          // This gamepad is already in the list.
          return;
        }
      }

      GamepadWithButtonState newGamepad = { args, false };
      m_gamepads.push_back(newGamepad);

    };

    // Used to stop looking for gamepads that are removed while the app is running.
    void OnGamepadRemoved(winrt::Windows::Foundation::IInspectable, winrt::Windows::Gaming::Input::Gamepad const& args) {

      m_gamepads.erase(std::remove_if(m_gamepads.begin(), m_gamepads.end(), [&](RenderingSystem::GamepadWithButtonState& gamepadWithState)
        {
          return gamepadWithState.gamepad == args;
        }),
        m_gamepads.end());

    };

    // Clears event registration state. Used when changing to a new HolographicSpace
    // and when tearing down AppMain.
    void UnregisterHolographicEventHandlers() {
      if (m_holographicSpace != nullptr)
      {
        // Clear previous event registrations.
        m_holographicSpace.CameraAdded(m_cameraAddedToken);
        m_cameraAddedToken = {};
        m_holographicSpace.CameraRemoved(m_cameraRemovedToken);
        m_cameraRemovedToken = {};
      }

      if (m_spatialLocator != nullptr)
      {
        m_spatialLocator.LocatabilityChanged(m_locatabilityChangedToken);
      }
    };

    // Synchronously releases resources for holographic cameras that are no longer
    // attached to the system.
    void OnCameraRemoved(
      winrt::Windows::Graphics::Holographic::HolographicSpace const& sender,
      winrt::Windows::Graphics::Holographic::HolographicSpaceCameraRemovedEventArgs const& args) {

      using namespace Concurrency;

      create_task([this]()
        {
          //
          // TODO: Asynchronously unload or deactivate content resources (not back buffer 
          //       resources) that are specific only to the camera that was removed.
          //
        });

      // Before letting this callback return, ensure that all references to the back buffer 
      // are released.
      // Since this function may be called at any time, the RemoveHolographicCamera function
      // waits until it can get a lock on the set of holographic camera resources before
      // deallocating resources for this camera. At 60 frames per second this wait should
      // not take long.
      m_deviceResources->RemoveHolographicCamera(args.Camera());
    };


    // Asynchronously creates resources for new holographic cameras.
    void OnCameraAdded(
      winrt::Windows::Graphics::Holographic::HolographicSpace const& sender,
      winrt::Windows::Graphics::Holographic::HolographicSpaceCameraAddedEventArgs const& args) {

      using namespace winrt::Windows::Graphics::Holographic;
      using namespace Concurrency;

      winrt::Windows::Foundation::Deferral deferral = args.GetDeferral();
      HolographicCamera holographicCamera = args.Camera();
      create_task([this, deferral, holographicCamera]()
        {
          //
          // TODO: Allocate resources for the new camera and load any content specific to
          //       that camera. Note that the render target size (in pixels) is a property
          //       of the HolographicCamera object, and can be used to create off-screen
          //       render targets that match the resolution of the HolographicCamera.
          //

          // Create device-based resources for the holographic camera and add it to the list of
          // cameras used for updates and rendering. Notes:
          //   * Since this function may be called at any time, the AddHolographicCamera function
          //     waits until it can get a lock on the set of holographic camera resources before
          //     adding the new camera. At 60 frames per second this wait should not take long.
          //   * A subsequent Update will take the back buffer from the RenderingParameters of this
          //     camera's CameraPose and use it to create the ID3D11RenderTargetView for this camera.
          //     Content can then be rendered for the HolographicCamera.
          m_deviceResources->AddHolographicCamera(holographicCamera);

          // Holographic frame predictions will not include any information about this camera until
          // the deferral is completed.
          deferral.Complete();
        });
    };


    // Sets the holographic space. This is our closest analogue to setting a new window
    // for the app.
    void SetHolographicSpace(winrt::Windows::Graphics::Holographic::HolographicSpace const& holographicSpace) {

      using namespace std::placeholders;

      UnregisterHolographicEventHandlers();

      m_holographicSpace = holographicSpace;

      //
      // TODO: Add code here to initialize your holographic content.
      //

      m_spatialInputHandler = std::make_unique<SpatialInputHandler>();

      // Respond to camera added events by creating any resources that are specific
      // to that camera, such as the back buffer render target view.
      // When we add an event handler for CameraAdded, the API layer will avoid putting
      // the new camera in new HolographicFrames until we complete the deferral we created
      // for that handler, or return from the handler without creating a deferral. This
      // allows the app to take more than one frame to finish creating resources and
      // loading assets for the new holographic camera.
      // This function should be registered before the app creates any HolographicFrames.
      m_cameraAddedToken = m_holographicSpace.CameraAdded(std::bind(&RenderingSystem::OnCameraAdded, this, _1, _2));

      // Respond to camera removed events by releasing resources that were created for that
      // camera.
      // When the app receives a CameraRemoved event, it releases all references to the back
      // buffer right away. This includes render target views, Direct2D target bitmaps, and so on.
      // The app must also ensure that the back buffer is not attached as a render target, as
      // shown in DeviceResources::ReleaseResourcesForBackBuffer.
      m_cameraRemovedToken = m_holographicSpace.CameraRemoved(std::bind(&RenderingSystem::OnCameraRemoved, this, _1, _2));

      // Notes on spatial tracking APIs:
      // * Stationary reference frames are designed to provide a best-fit position relative to the
      //   overall space. Individual positions within that reference frame are allowed to drift slightly
      //   as the device learns more about the environment.
      // * When precise placement of individual holograms is required, a SpatialAnchor should be used to
      //   anchor the individual hologram to a position in the real world - for example, a point the user
      //   indicates to be of special interest. Anchor positions do not drift, but can be corrected; the
      //   anchor will use the corrected position starting in the next frame after the correction has
      //   occurred.
    };

    void SetWindow(winrt::Windows::UI::Core::CoreWindow const & window) {

      // Create a holographic space for the core window for the current view.
      // Presenting holographic frames that are created by this holographic space will put
      // the app into exclusive mode.
      m_holographicSpace = winrt::Windows::Graphics::Holographic::HolographicSpace::CreateForCoreWindow(window);

      // The DeviceResources class uses the preferred DXGI adapter ID from the holographic
      // space (when available) to create a Direct3D device. The HolographicSpace
      // uses this ID3D11Device to create and manage device-based resources such as
      // swap chains.
      m_deviceResources->SetHolographicSpace(m_holographicSpace);

      // The main class uses the holographic space for updates and rendering.
      SetHolographicSpace(m_holographicSpace);
    };

    void Initialize() {
      LOG(INFO) << "ndtech::HoloLensRenderingSystem::Initilizae::typeid(std::map<std::wstring, ID3D11VertexShader>).name() = " << typeid(std::map<std::wstring, ID3D11VertexShader>).name();

      m_vertexShadersStore.RunOn(m_vertexShadersStoreFiber);
      m_pixelShadersStore.RunOn(m_pixelShadersStoreFiber);
      m_geometryShadersStore.RunOn(m_geometryShadersStoreFiber);

      using namespace std;
      using namespace std::placeholders;
      using namespace winrt::Windows::Gaming::Input;
      using namespace winrt::Windows::Graphics::Holographic;

      m_deviceResources = std::make_unique<DeviceResources>();
      m_deviceResources->RegisterDeviceNotify(this);



      // If connected, a game controller can also be used for input.
      m_gamepadAddedEventToken = Gamepad::GamepadAdded(bind(&RenderingSystem::OnGamepadAdded, this, _1, _2));
      m_gamepadRemovedEventToken = Gamepad::GamepadRemoved(bind(&RenderingSystem::OnGamepadRemoved, this, _1, _2));

      for (Gamepad const& gamepad : Gamepad::Gamepads())
      {
        OnGamepadAdded(nullptr, gamepad);
      }

      m_canGetHolographicDisplayForCamera = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Holographic.HolographicCamera", L"Display");
      m_canGetDefaultHolographicDisplay = winrt::Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(L"Windows.Graphics.Holographic.HolographicDisplay", L"GetDefault");
      m_canCommitDirect3D11DepthBuffer = winrt::Windows::Foundation::Metadata::ApiInformation::IsMethodPresent(L"Windows.Graphics.Holographic.HolographicCameraRenderingParameters", L"CommitDirect3D11DepthBuffer");

      if (m_canGetDefaultHolographicDisplay)
      {
        // Subscribe for notifications about changes to the state of the default HolographicDisplay 
        // and its SpatialLocator.
        m_holographicDisplayIsAvailableChangedEventToken = HolographicSpace::IsAvailableChanged(bind(&RenderingSystem::OnHolographicDisplayIsAvailableChanged, this, _1, _2));
      }

      // Acquire the current state of the default HolographicDisplay and its SpatialLocator.
      OnHolographicDisplayIsAvailableChanged(nullptr, nullptr);

    };

    // IDeviceNotify
    void OnDeviceLost() {};
    void OnDeviceRestored() {};



    // Used to notify the app when the positional tracking state changes.
    void OnLocatabilityChanged(
      winrt::Windows::Perception::Spatial::SpatialLocator const& sender,
      winrt::Windows::Foundation::IInspectable const& args) {

      using namespace winrt::Windows::Perception::Spatial;

      switch (sender.Locatability())
      {
      case SpatialLocatability::Unavailable:
        // Holograms cannot be rendered.
      {
        winrt::hstring message{ L"Warning! Positional tracking is " + std::to_wstring(int(sender.Locatability())) + L".\n" };
        OutputDebugStringW(message.data());
      }
      break;

      // In the following three cases, it is still possible to place holograms using a
      // SpatialLocatorAttachedFrameOfReference.
      case SpatialLocatability::PositionalTrackingActivating:
        // The system is preparing to use positional tracking.

      case SpatialLocatability::OrientationOnly:
        // Positional tracking has not been activated.

      case SpatialLocatability::PositionalTrackingInhibited:
        // Positional tracking is temporarily inhibited. User action may be required
        // in order to restore positional tracking.
        break;

      case SpatialLocatability::PositionalTrackingActive:
        // Positional tracking is active. World-locked content can be rendered.
        break;
      }

    };

    // Used to respond to changes to the default spatial locator.
    void OnHolographicDisplayIsAvailableChanged(winrt::Windows::Foundation::IInspectable, winrt::Windows::Foundation::IInspectable) {

      using namespace std;
      using namespace std::placeholders;
      using namespace winrt::Windows::Perception::Spatial;
      using namespace winrt::Windows::Graphics::Holographic;

      // Get the spatial locator for the default HolographicDisplay, if one is available.
      SpatialLocator spatialLocator = nullptr;
      if (m_canGetDefaultHolographicDisplay)
      {
        HolographicDisplay defaultHolographicDisplay = HolographicDisplay::GetDefault();
        if (defaultHolographicDisplay)
        {
          spatialLocator = defaultHolographicDisplay.SpatialLocator();
        }
      }
      else
      {
        spatialLocator = SpatialLocator::GetDefault();
      }

      if (m_spatialLocator != spatialLocator)
      {
        // If the spatial locator is disconnected or replaced, we should discard all state that was
        // based on it.
        if (m_spatialLocator != nullptr)
        {
          m_spatialLocator.LocatabilityChanged(m_locatabilityChangedToken);
          m_spatialLocator = nullptr;
        }

        m_stationaryReferenceFrame = nullptr;

        if (spatialLocator != nullptr)
        {
          // Use the SpatialLocator from the default HolographicDisplay to track the motion of the device.
          m_spatialLocator = spatialLocator;

          // Respond to changes in the positional tracking state.
          m_locatabilityChangedToken = m_spatialLocator.LocatabilityChanged(std::bind(&RenderingSystem::OnLocatabilityChanged, this, _1, _2));

          // The simplest way to render world-locked holograms is to create a stationary reference frame
          // based on a SpatialLocator. This is roughly analogous to creating a "world" coordinate system
          // with the origin placed at the device's position as the app is launched.
          m_stationaryReferenceFrame = m_spatialLocator.CreateStationaryFrameOfReferenceAtCurrentLocation();
        }
      }
    };

    // The name is the thing
    // The name should embed the logic of creation
    // Configuration should be passed in
    // Template arguments are good as well, configuration and specialization

    template<typename ShaderType>
    void CreateShader(std::wstring shaderFileName) {

      static_assert(TypeUtilities::TypelistContains<ShaderType, ShaderTypes>(), "Invalid Shader Type");

      //co_await 1ms;

    };

    template<>
    void CreateShader<winrt::com_ptr<ID3D11VertexShader>>(std::wstring shaderFileName) {

      if (!m_vertexShadersStore.ItemExists(shaderFileName)) {


        if (!m_app->m_fileDataStore.ItemExists(shaderFileName)) {
          std::vector<::byte> shaderFileData = ndtech::utilities::ReadDataFiber(shaderFileName);
          m_app->m_fileDataStore.AddItem(shaderFileName, shaderFileData);
        };

        std::vector<::byte> shaderFileData = m_app->m_fileDataStore.GetItem(shaderFileName);
        winrt::com_ptr<ID3D11VertexShader> shader = nullptr;

        // After the  shader file is loaded, create the shader
        winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateVertexShader(
          shaderFileData.data(),
          shaderFileData.size(),
          nullptr,
          shader.put()
        ));

#ifdef _DEBUG
        {
          std::stringstream ss("ndtech::HoloLensRenderingSystem::");
          ss << std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(shaderFileName);

          std::string shaderName(ss.str());
          (shader)->SetPrivateData(WKPDID_D3DDebugObjectName, shaderName.size(), shaderName.c_str());
        }
#endif
        m_vertexShadersStore.AddItem(shaderFileName, shader);
      }

    };

    template<>
    void CreateShader<winrt::com_ptr<ID3D11PixelShader>>(std::wstring shaderFileName) {

      if (!m_pixelShadersStore.ItemExists(shaderFileName)) {

        if (!m_app->m_fileDataStore.ItemExists(shaderFileName)) {
          std::vector<::byte> shaderFileData = ndtech::utilities::ReadDataFiber(shaderFileName);
          m_app->m_fileDataStore.AddItem(shaderFileName, shaderFileData);
        };

        std::vector<::byte> shaderFileData = m_app->m_fileDataStore.GetItem(shaderFileName);
        winrt::com_ptr<ID3D11PixelShader> shader = nullptr;

        // After the  shader file is loaded, create the shader
        winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreatePixelShader(
          shaderFileData.data(),
          shaderFileData.size(),
          nullptr,
          shader.put()
        ));

#ifdef _DEBUG
        {
          std::stringstream ss("ndtech::HoloLensRenderingSystem::");
          ss << std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(shaderFileName);

          std::string shaderName(ss.str());
          (shader)->SetPrivateData(WKPDID_D3DDebugObjectName, shaderName.size(), shaderName.c_str());
        }
#endif
        m_pixelShadersStore.AddItem(shaderFileName, shader);
      }

    };

    template<>
    void CreateShader<winrt::com_ptr<ID3D11GeometryShader>>(std::wstring shaderFileName) {

      if (!m_geometryShadersStore.ItemExists(shaderFileName)) {

        if (!m_app->m_fileDataStore.ItemExists(shaderFileName)) {
          std::vector<::byte> shaderFileData = ndtech::utilities::ReadDataFiber(shaderFileName);
          m_app->m_fileDataStore.AddItem(shaderFileName, shaderFileData);
        };

        std::vector<::byte> shaderFileData = m_app->m_fileDataStore.GetItem(shaderFileName);
        winrt::com_ptr<ID3D11GeometryShader> shader = nullptr;

        // After the  shader file is loaded, create the shader
        winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateGeometryShader(
          shaderFileData.data(),
          shaderFileData.size(),
          nullptr,
          shader.put()
        ));

#ifdef _DEBUG
        {
          std::stringstream ss("ndtech::HoloLensRenderingSystem::");
          ss << std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(shaderFileName);

          std::string shaderName(ss.str());
          (shader)->SetPrivateData(WKPDID_D3DDebugObjectName, shaderName.size(), shaderName.c_str());
        }
#endif
        m_geometryShadersStore.AddItem(shaderFileName, shader);
      }

    };

    template <typename VertexType>
    winrt::com_ptr<ID3D11InputLayout> SetLayout(winrt::com_ptr<ID3D11InputLayout> inputLayout, std::wstring shaderFileName) {
      static_assert(TypeUtilities::TypelistContains<VertexType, VertexTypes>(), "Invalid Vertex Type");
      return inputLayout;
    }

    template <>
    winrt::com_ptr<ID3D11InputLayout> SetLayout<ndtech::vertexTypes::VertexPosition>(winrt::com_ptr<ID3D11InputLayout> inputLayout, std::wstring shaderFileName) {

      constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 1> vertexDesc =
      { {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      } };

      std::vector<::byte> vertexShaderFileData = m_app->m_fileDataStore.GetItem(shaderFileName);

      winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateInputLayout(
        vertexDesc.data(),
        static_cast<UINT>(vertexDesc.size()),
        vertexShaderFileData.data(),
        static_cast<UINT>(vertexShaderFileData.size()),
        m_inputLayout.put()
      ));

      return inputLayout;
    }

    template <>
    winrt::com_ptr<ID3D11InputLayout> SetLayout<ndtech::vertexTypes::VertexPositionColor>(winrt::com_ptr<ID3D11InputLayout> inputLayout, std::wstring shaderFileName) {

      constexpr std::array<D3D11_INPUT_ELEMENT_DESC, 2> vertexDesc =
      { {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      } };

      std::vector<::byte> vertexShaderFileData = m_app->m_fileDataStore.GetItem(shaderFileName);

      winrt::check_hresult(m_deviceResources->GetD3DDevice()->CreateInputLayout(
        vertexDesc.data(),
        static_cast<UINT>(vertexDesc.size()),
        vertexShaderFileData.data(),
        static_cast<UINT>(vertexShaderFileData.size()),
        inputLayout.put()
      ));

      return inputLayout;

    }

  };

}