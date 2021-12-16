#pragma once
#include "pch.h"
#include "ApplicationContext.h"
#include "StepTimer.h"
#include "TypeUtilities.h"
#include "NamedItemStore.h"
#include <boost/fiber/all.hpp>

namespace ndtech {
  
  struct BaseApp {

    struct ApplicationContext m_applicationContext { 2, "ndtech.BaseApp" };
    StepTimer m_timer;

    NamedItemStore<std::vector<::byte>, std::wstring> m_fileDataStore;
    boost::fibers::fiber m_fileDataStoreFiber;

    BaseApp() = default;
    virtual ~BaseApp();
    BaseApp & operator=(const BaseApp&) = default;
    BaseApp(const BaseApp&) = default;
    BaseApp(BaseApp &&) = default;

    virtual void ConcreteUpdate(StepTimer timer) {};
    virtual void ConcreteRender() {};

    virtual bool Initialize();
    virtual void Loop() = 0;

    void Run();

    virtual bool AfterGraphicsInitialized();
    virtual void Update(StepTimer timer) = 0;

    virtual void AfterWindowSet();

    std::vector<::byte> GetFileData(std::wstring shaderFileName);

  };    
  
  template <typename TestType>
    using TestTypeHasInitializeImpl = decltype(std::declval<TestType>().Initialize(std::declval<BaseApp*>()));

  template <typename TestType>
  using TestTypeHasInitialize = ndtech::TypeUtilities::is_detected<TestTypeHasInitializeImpl, TestType>;

  template <typename TestType>
  using TestTypeHasInitializeFromBaseAppImpl = decltype(std::declval<TestType>().Initialize(std::declval<BaseApp*>()));

  template <typename TestType>
  using TestTypeHasInitializeFromBasApp = ndtech::TypeUtilities::is_detected<TestTypeHasInitializeFromBaseAppImpl, TestType>;

  template <typename TestType>
  using TestTypeHasUpdateComponentImpl = decltype(
    std::declval<TestType>().UpdateComponent(
      std::declval<typename TestType::Component*>(),
      std::declval<BaseApp*>()
    )
    );

  template <typename TestType>
  using TestTypeHasUpdateComponent = ndtech::TypeUtilities::is_detected<TestTypeHasUpdateComponentImpl, TestType>;

  template <typename TestType, typename AppType>
  using TestTypeHasInitializeComponentImpl =
    decltype(std::declval<TestType>().InitializeComponent(
      std::declval<typename TestType::Component>(),
      std::declval<AppType*>()
    ));

  template <typename TestType, typename AppType>
  using TestTypeHasInitializeComponent = ndtech::TypeUtilities::is_detected<TestTypeHasInitializeComponentImpl, TestType, AppType>;

  template <typename TestType>
  using TestTypeHasRenderComponentImpl = decltype(
    std::declval<TestType>().RenderComponent(
      std::declval<typename TestType::Component>(),
      std::declval<BaseApp*>(),
      std::declval<int>()
    )
    );

  template <typename TestType>
  using TestTypeHasRenderComponent = ndtech::TypeUtilities::is_detected<TestTypeHasRenderComponentImpl, TestType>;



  template <typename TestType>
  using TestTypeHasPreRenderComponentSystemImpl = decltype(
    std::declval<TestType>().PreRenderComponentSystem(
      std::declval<BaseApp*>()
    )
    );

  template <typename TestType>
  using TestTypeHasPreRenderComponentSystem = ndtech::TypeUtilities::is_detected<TestTypeHasPreRenderComponentSystemImpl, TestType>;




  template <typename TestType, typename TestComponentSystemType>
  using TestTypeHasPreUpdateThisComponentSystemImpl = decltype(
    std::declval<TestType>().PreUpdateComponentSystem(
      std::declval<TestComponentSystemType>(),
      std::declval<std::vector<typename TestComponentSystemType::Component>*>(),
      std::declval<BaseApp*>()
    )
    );

  template <typename TestType, typename... ArgTypes>
  using TestTypeHasPreUpdateThisComponentSystem = ndtech::TypeUtilities::is_detected<TestTypeHasPreUpdateThisComponentSystemImpl, TestType, ArgTypes...>;

  template <typename TestType, typename TestComponentSystemType>
  using TestTypeHasPostUpdateThisComponentSystemImpl = decltype(
    std::declval<TestType>().PostUpdateComponentSystem(
      std::declval<TestComponentSystemType>(),
      std::declval<std::vector<typename TestComponentSystemType::Component>*>(),
      std::declval<BaseApp*>()
    )
    );

  template <typename TestType, typename... ArgTypes>
  using TestTypeHasPostUpdateThisComponentSystem = ndtech::TypeUtilities::is_detected<TestTypeHasPostUpdateThisComponentSystemImpl, TestType, ArgTypes...>;



}