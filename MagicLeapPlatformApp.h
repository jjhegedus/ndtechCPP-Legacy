#pragma once

#include "BaseApp.h"
#include "RenderingSystem.h"
#include "Features.h"

namespace ndtech {
  template <typename TSettings>
  struct PlatformApp: public BaseApp {

    using Settings = TSettings;
    using Components = typename Settings::Components;
    using ComponentSystems = typename Settings::ComponentSystems;
    using ComponentSystemsTuple = TypeUtilities::Convert<ComponentSystems, std::tuple>;
    using ComponentVectors = TypeUtilities::Convert<Components, TypeUtilities::TupleOfVectors>;
    using EntityIndexType = typename Settings::EntityIndexType;


    using Features = TypeUtilities::Typelist<features::hand_tracking, features::finger_tracking>;

    RenderingSystem<TSettings> m_renderingSystem;

    MLLifecycleCallbacks m_lifecycle_callbacks = {};
    MLPerceptionSettings m_perception_settings;
    MLHandle m_head_tracker;
    MLHeadTrackingStaticData m_head_static_data;

    virtual ~PlatformApp() {
      MLPerceptionShutdown();
    }

    void Configure() {

      // let system know our app has started
      m_lifecycle_callbacks.on_stop = onStop;
      m_lifecycle_callbacks.on_pause = onPause;
      m_lifecycle_callbacks.on_resume = onResume;


      if (MLResult_Ok != MLLifecycleInit(&m_lifecycle_callbacks, (void*)&m_applicationContext)) {
        //ND_LOG(Error, "%s: Failed to initialize lifecycle.", application_context.applicationName.c_str());
      }

      // initialize perception system
      if (MLResult_Ok != MLPerceptionInitSettings(&m_perception_settings)) {
        //ND_LOG(Error, "%s: Failed to initialize perception.", application_context.applicationName.c_str());
      }

      if (MLResult_Ok != MLPerceptionStartup(&m_perception_settings)) {
        //ND_LOG(Error, "%s: Failed to startup perception.", application_context.applicationName.c_str());
      }

      Initialize();

    };

    // Callbacks
    static void onStop(void* application_context)
    {
      ((struct ApplicationContext*)application_context)->m_state = 0;

      //ND_LOG(Info, "%s: On resume called.", "*****");
      //ND_LOG(Info, "%s: On stop called.", ((struct ApplicationContext*)application_context)->applicationName.c_str());
    };

    static void onPause(void* application_context)
    {
      ((struct ApplicationContext*)application_context)->m_state = 1;

      //ND_LOG(Info, "%s: On resume called.", "*****");
      //ND_LOG(Info, "%s: On pause called.", ((struct ApplicationContext*)application_context)->applicationName.c_str());
    };

    static void onResume(void* application_context)
    {
      ((struct ApplicationContext*)application_context)->m_state = 2;

      //ND_LOG(Info, "%s: On resume called.", "*****");
      //ND_LOG(Info, "%s: On resume called.", ((struct ApplicationContext*)application_context)->applicationName.c_str());

    };


    bool AfterGraphicsInitialized() override {

      // Now that graphics is connected, the app is ready to go
      if (MLResult_Ok != MLLifecycleSetReadyIndication()) {
        //ND_LOG(Error, "%s: Failed to indicate lifecycle ready.", application_context.applicationName.c_str());
        return false;
      }

      MLResult head_track_result = MLHeadTrackingCreate(&m_head_tracker);
      if (MLResult_Ok == head_track_result && MLHandleIsValid(m_head_tracker)) {
        MLHeadTrackingGetStaticData(m_head_tracker, &m_head_static_data);
      }
      else {
        //ND_LOG(Error, "%s: Failed to create head tracker.", application_context.applicationName.c_str());
        return false;
      }

      AfterWindowSet();

      return true;
    };

    virtual void Update(StepTimer timer) override {
      ConcreteUpdate(timer);
    };

  };

}