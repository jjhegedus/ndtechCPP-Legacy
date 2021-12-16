#pragma once

#include "pch.h"
#include "BaseApp.h"
#include "RenderingSystem.h"
#include "Features.h"

namespace ndtech {
  using namespace winrt::Windows::UI::Core;
  using namespace winrt::Windows::ApplicationModel::Core;
  using namespace winrt::Windows::ApplicationModel::Activation;

  template <typename TSettings>
  struct PlatformApp : public BaseApp, public winrt::implements<PlatformApp<TSettings>, winrt::Windows::ApplicationModel::Core::IFrameworkView> {

    using Features = TypeUtilities::Typelist<>;

    RenderingSystem<TSettings>                      m_renderingSystem;

    bool                                            m_windowClosed = false;
    bool                                            m_windowVisible = true;
    winrt::event_token                              m_suspendingEventToken;
    winrt::event_token                              m_resumingEventToken;

    virtual bool Initialize() override {
      return BaseApp::Initialize();
    }

    void Uninitialize()
    {
      LOG(INFO) << "Uninitialize called";
      CoreApplication::Suspending(m_suspendingEventToken);
      CoreApplication::Resuming(m_resumingEventToken);

      //auto const& window = CoreWindow::GetForCurrentThread();
      //window.KeyDown(m_keyDownEventToken);
      //window.PointerPressed(m_pointerPressedEventToken);
      //window.Closed(m_windowClosedEventToken);
      //window.VisibilityChanged(m_visibilityChangedEventToken);
    }

    void Load(winrt::hstring)
    {
    }


    // Window event handlers.
    void OnVisibilityChanged(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::VisibilityChangedEventArgs const& args) {
      m_windowVisible = args.Visible();
    };

    void OnWindowClosed(winrt::Windows::UI::Core::CoreWindow const& sender, winrt::Windows::UI::Core::CoreWindowEventArgs const& args) {
      m_windowClosed = true;
      m_applicationContext.m_state = 0;
    };

    // CoreWindow input event handlers.
    void OnKeyPressed(CoreWindow const& sender, KeyEventArgs const& args)
    {
      //
      // TODO: Bluetooth keyboards are supported by HoloLens. You can use this method for
      //       keyboard input if you want to support it as an optional input method for
      //       your holographic app.
      //
    };

    void SetWindow(winrt::Windows::UI::Core::CoreWindow const & window) {
      window.KeyDown({ this, &PlatformApp::OnKeyPressed });
      window.Closed({ this, &PlatformApp::OnWindowClosed });
      window.VisibilityChanged({ this, &PlatformApp::OnVisibilityChanged });

      // The main class uses the holographic space for updates and rendering.
      m_renderingSystem.SetWindow(window);
      AfterWindowSet();
    };

    void LoadAppState() {
      //
      // TODO: Insert code here to load your app state.
      //       This method is called when the app resumes.
      //
      //       For example, load information from the SpatialAnchorStore.
      //
    };

    void OnSuspending(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::ApplicationModel::SuspendingEventArgs const& args) {

      using namespace Concurrency;
      using namespace winrt::Windows::ApplicationModel;
      using namespace winrt::Windows::Foundation;

      // Save app state asynchronously after requesting a deferral. Holding a deferral
      // indicates that the application is busy performing suspending operations. Be
      // aware that a deferral may not be held indefinitely; after about five seconds,
      // the app will be forced to exit.
      SuspendingDeferral deferral = args.SuspendingOperation().GetDeferral();

      create_task([this, deferral]()
        {
          //m_renderingSystem.m_deviceResources->Trim();

          //if (m_main != nullptr)
          //{
          //  m_main->SaveAppState();
          //}

          //
          // TODO: Insert code here to save your app state.
          //
          m_applicationContext.m_state = 1;
          deferral.Complete();
        });
    };

    void OnResuming(IInspectable const& sender, IInspectable const& args) {
      // Restore any data or state that was unloaded on suspend. By default, data
      // and state are persisted when resuming from suspend. Note that this event
      // does not occur if the app was previously terminated.

      LoadAppState();
      m_applicationContext.m_state = 2;
    };

    // Called when the app view is activated. Activates the app's CoreWindow.
    void OnViewActivated(CoreApplicationView const& sender, IActivatedEventArgs const& args)
    {
      // Run() won't start until the CoreWindow is activated.
      sender.CoreWindow().Activate();
      m_applicationContext.m_state = 2;
    };

    // implemented inline
    void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const & appView) {
      using namespace std;
      using namespace std::placeholders;

      appView.Activated({ this, &PlatformApp::OnViewActivated });
      m_suspendingEventToken = winrt::Windows::ApplicationModel::Core::CoreApplication::Suspending({ this, &PlatformApp::OnSuspending });
      m_resumingEventToken = winrt::Windows::ApplicationModel::Core::CoreApplication::Resuming({ this, &PlatformApp::OnResuming });

      Initialize();
    }

    void Configure() {
      LOG(INFO) << m_applicationContext.m_name << ": Configure()";
    };


    // Called when the app is prelaunched. Use this method to load resources ahead of time
    // and enable faster launch times.
    void OnLaunched(LaunchActivatedEventArgs const& args)
    {
      if (args.PrelaunchActivated())
      {
        //
        // TODO: Insert code to preload resources here.
        //
      }
      m_applicationContext.m_state = 1;
    };

    void OnPointerPressed(CoreWindow const& sender, PointerEventArgs const& args)
    {
      m_renderingSystem.m_pointerPressed = true;
    };

    virtual void Update(StepTimer timer) override {

      using namespace winrt::Windows::UI::Input::Spatial;
      using namespace winrt::Windows::Graphics::Holographic;

      // Before doing the timer update, there is some work to do per-frame
      // to maintain holographic rendering. First, we will get information
      // about the current frame.

      // The HolographicFrame has information that the app needs in order
      // to update and render the current frame. The app begins each new
      // frame by calling CreateNextFrame.
      HolographicFrame holographicFrame = m_renderingSystem.m_holographicSpace.CreateNextFrame();

      // Get a prediction of where holographic cameras will be when this frame
      // is presented.
      HolographicFramePrediction prediction = holographicFrame.CurrentPrediction();

      // Back buffers can change from frame to frame. Validate each buffer, and recreate
      // resource views and depth buffers as needed.
      m_renderingSystem.m_deviceResources->EnsureCameraResources(holographicFrame, prediction);

      if (m_renderingSystem.m_stationaryReferenceFrame != nullptr)
      {
        // Check for new input state since the last frame.
        for (RenderingSystem<TSettings>::GamepadWithButtonState& gamepadWithButtonState : m_renderingSystem.m_gamepads)
        {
          using namespace winrt::Windows::Gaming::Input;

          bool buttonDownThisUpdate = ((gamepadWithButtonState.gamepad.GetCurrentReading().Buttons & GamepadButtons::A) == GamepadButtons::A);
          if (buttonDownThisUpdate && !gamepadWithButtonState.buttonAWasPressedLastFrame)
          {
            m_renderingSystem.m_pointerPressed = true;
          }
          gamepadWithButtonState.buttonAWasPressedLastFrame = buttonDownThisUpdate;
        }

        SpatialInteractionSourceState pointerState = m_renderingSystem.m_spatialInputHandler->CheckForInput();
        if (pointerState != nullptr)
        {
          m_renderingSystem.m_pose = pointerState.TryGetPointerPose(m_renderingSystem.m_stationaryReferenceFrame.CoordinateSystem());
        }
        else if (m_renderingSystem.m_pointerPressed)
        {
          m_renderingSystem.m_pose = SpatialPointerPose::TryGetAtTimestamp(m_renderingSystem.m_stationaryReferenceFrame.CoordinateSystem(), prediction.Timestamp());
        }
        m_renderingSystem.m_pointerPressed = false;

        CoreWindow w = CoreWindow::GetForCurrentThread();
        CoreDispatcher d = CoreWindow::GetForCurrentThread().Dispatcher();

        CoreWindow::GetForCurrentThread().Dispatcher().ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
      }

      ConcreteUpdate(timer);

      if (!m_renderingSystem.m_canCommitDirect3D11DepthBuffer)
      {
        // On versions of the platform that do not support the CommitDirect3D11DepthBuffer API, we can control
        // image stabilization by setting a focus point with optional plane normal and velocity.
        for (HolographicCameraPose const& cameraPose : prediction.CameraPoses())
        {
          // The HolographicCameraRenderingParameters class provides access to set
          // the image stabilization parameters.
          HolographicCameraRenderingParameters renderingParameters = holographicFrame.GetRenderingParameters(cameraPose);

          // SetFocusPoint informs the system about a specific point in your scene to
          // prioritize for image stabilization. The focus point is set independently
              // for each holographic camera. When setting the focus point, put it on or 
              // near content that the user is looking at.
              // In this example, we put the focus point at the center of the sample hologram.
              // You can also set the relative velocity and facing of the stabilization
              // plane using overloads of this method.
          if (m_renderingSystem.m_stationaryReferenceFrame != nullptr)
          {
            renderingParameters.SetFocusPoint(
              m_renderingSystem.m_stationaryReferenceFrame.CoordinateSystem(),
              m_renderingSystem.GetFocusPosition()
            );
          }
        }
      }

      // The holographic frame will be used to get up-to-date view and projection matrices and
      // to present the swap chain.
      this->m_renderingSystem.m_holographicFrame = holographicFrame;
    };

  };

}