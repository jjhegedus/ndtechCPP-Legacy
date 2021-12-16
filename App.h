#pragma once
#include "pch.h"
#include "PlatformApp.h"
#include "TypeUtilities.h"
#include <tuple>
#include <type_traits>
#include "Scheduler.h"

namespace ndtech {

  template <typename CompType, typename... RemainingComponentSystems>
  struct GetComponentSystemImpl {
    using type = void;
  };

  template <typename ComponentType, typename MatchComponentSystemType, typename... RemainingComponentSystems>
  struct GetComponentSystemImpl < ComponentType, TypeUtilities::Typelist<MatchComponentSystemType, RemainingComponentSystems...>> {
    using MatchComponentType = typename MatchComponentSystemType::Component;


    using type = typename std::conditional<
      std::is_same<ComponentType, MatchComponentType>::value,
      MatchComponentSystemType,
      typename GetComponentSystemImpl<ComponentType, typename TypeUtilities::Typelist<RemainingComponentSystems...>>::type>::type;
  };

  template <typename TSettings, typename Derived>
  struct App : public PlatformApp<TSettings> {

    using Settings = TSettings;
    using ThisType = App<Settings, Derived>;
    using Components = typename Settings::Components;
    using ComponentSystems = typename Settings::ComponentSystems;
    using EntityIndexType = typename Settings::EntityIndexType;
    using EntityType = typename Settings::EntityType;
    using EntityVector = std::vector<EntityType>;

    using ComponentSystemsTuple = TypeUtilities::Convert<ComponentSystems, std::tuple>;
    ComponentSystemsTuple m_componentSystems;
    int m_numberOfComponentSystems = std::tuple_size<ComponentSystemsTuple>::value;

    using ComponentVectors = TypeUtilities::Convert<Components, TypeUtilities::TupleOfVectors>;

    EntityVector m_entities;
    EntityIndexType m_freeEntityIndex = 0;
    EntityIndexType m_entitiesCapacity = 0;

    std::vector<EntityIndexType> m_freeComponentIndices;
    std::vector<EntityIndexType> m_componentCapacities;

    ComponentVectors m_componentVectors;
    int m_numberOfComponentVectors = std::tuple_size<ComponentVectors>::value;

    ndtech::Scheduler                               m_scheduler;

    App<TSettings, Derived>() {
    }

    ~App<TSettings, Derived>() {
    }


    EntityType& AddEntity() {
      IncreaseEntityStorageIfNeeded();
      EntityType& entity(m_entities[m_freeEntityIndex++]);
      entity.isAlive = true;
      return entity;
    }

    template <typename T>
    T& AddComponent(EntityType& entity, T inputComponent) noexcept
    {

      static_assert(Settings::template isComponent<T>(), "ndtech::App<TSettings>::AddComponent T is not a component");

      //e.bitset[Settings::template componentBit<T>()] = true;

      int componentVectorNumber = TypeUtilities::IndexOf<T, App::Components>();
      std::vector<T>* componentVector = &std::get<std::vector<T>>(m_componentVectors);

      IncreaseComponentStorageIfNeeded<T>();
      T* component = &(componentVector->operator[](m_freeComponentIndices[componentVectorNumber]++));

      using ComponentSystemType = typename GetComponentSystemImpl<T, ComponentSystems>::type;
      if constexpr (TestTypeHasInitializeComponent<ComponentSystemType, App<TSettings, Derived>>{}) {
        ComponentSystemType* componentSystem = &std::get<ComponentSystemType>(m_componentSystems);
        *component = componentSystem->InitializeComponent(inputComponent, this);
      }

      return *component;
    }

    void LogStaticStats() {

      LOG(INFO) << "There are " << m_numberOfComponentVectors << " component vectors";
      LOG(INFO) << "componentCapacities.size() = " << m_componentCapacities.size();
      LOG(INFO) << "freeComponentIndices.size() = " << m_freeComponentIndices.size();

    }

    void LogDynamicStats() {

      LOG(INFO) << "entities.size() = " << m_entities.size();
      LOG(INFO) << "entitiesCapacity is " << m_entitiesCapacity;
      LOG(INFO) << "freeEntityIndex is " << m_freeEntityIndex;

      int vectorNumber = 0;
      TypeUtilities::ForTuple(
        [this, &vectorNumber](auto componentVector) {
          LOG(INFO) << "component vector #" << vectorNumber << " has " << componentVector.size() << " elements, ";
          LOG(INFO) << "component vector #" << vectorNumber << " has " << m_componentCapacities[vectorNumber] << " capacity";
          LOG(INFO) << "component vector #" << vectorNumber << " has " << m_freeComponentIndices[vectorNumber] << " freeComponentIndex";

          vectorNumber++;
        },
        m_componentVectors);
    }

    void LogStats() {
      LogStaticStats();
      LogDynamicStats();
    }

    void LogEntities() {
      LOG(INFO) << "Logging Entities :: BEGIN";
      for (EntityType entity : m_entities) {
        LOG(INFO) << "entity.index = " << entity.index << ", entity.isAlive = " << entity.isAlive;
      }
      LOG(INFO) << "Logging Entities :: END";
    }

    void LogAliveEntities() {
      LOG(INFO) << "Logging Alive Entities :: BEGIN";
      for (EntityIndexType entityIndex = 0; entityIndex < m_freeEntityIndex; entityIndex++) {
        EntityType entity = m_entities[entityIndex];
        LOG(INFO) << "entity.index = " << entity.index << ", entity.isAlive = " << entity.isAlive;
      }
      LOG(INFO) << "Logging Alive Entities :: END";
    }

    template<typename ComponentSystemType>
    void InitializeComponentSystem() {

      if constexpr (TestTypeHasInitialize<ComponentSystemType>{}) {
        ComponentSystemType& componentSystem = std::get<ComponentSystemType>(this->m_componentSystems);
        componentSystem.Initialize(this);
      }

    }

    template<typename... ComponentSystemTypes>
    void InitializeComponentSystems(ndtech::TypeUtilities::Typelist<ComponentSystemTypes...> typelist) {

      (InitializeComponentSystem<ComponentSystemTypes>(), ...);

      //TypeUtilities::ForTuple(
      //  [this](auto componentSystem) mutable {

      //    if constexpr (TestTypeHasInitialize<decltype(componentSystem)>{}) {
      //      decltype(componentSystem)* mutableComponentSystem = &std::get<decltype(componentSystem)>(this->m_componentSystems);
      //      mutableComponentSystem->Initialize(this);
      //    }

      //  },
      //  m_componentSystems);
    }

    template<typename ComponentSystemType>
    void UpdateComponentSystem() {

      if constexpr (TestTypeHasUpdateComponent<ComponentSystemType>{}) {

        ComponentSystemType& componentSystem = std::get<ComponentSystemType>(this->m_componentSystems);
        std::vector<typename ComponentSystemType::Component>* componentVector = &std::get<std::vector<typename ComponentSystemType::Component>>(m_componentVectors);


        if constexpr (TestTypeHasPreUpdateThisComponentSystem<Derived, ComponentSystemType>{}) {
          componentSystem.PreUpdateComponentSystem(componentSystem, componentVector, this);
        }

        for (EntityIndexType componentIndex = 0; componentIndex < this->m_freeComponentIndices[TypeUtilities::IndexOf<typename ComponentSystemType::Component, App::Components>()]; componentIndex++) {
          auto component = &componentVector->at(componentIndex);
          componentSystem.UpdateComponent(component, this);
        }

        if constexpr (TestTypeHasPostUpdateThisComponentSystem<Derived, decltype(componentSystem)>{}) {
          componentSystem.PostUpdateComponentSystem(componentSystem, componentVector, this);
        }

      }
    }

    template<typename... ComponentSystemTypes>
    void UpdateComponentSystems(ndtech::TypeUtilities::Typelist<ComponentSystemTypes...>) {

      (UpdateComponentSystem<ComponentSystemTypes>(), ...);

      //TypeUtilities::ForTuple(
      //  [this](auto componentSystem) {

      //    if constexpr (TestTypeHasUpdateComponent<decltype(componentSystem)>{}) {

      //      std::vector<typename decltype(componentSystem)::Component>* componentVector = &std::get<std::vector<typename decltype(componentSystem)::Component>>(m_componentVectors);


      //      if constexpr (TestTypeHasPreUpdateThisComponentSystem<Derived, decltype(componentSystem)>{}) {
      //        static_cast<Derived*>(this)->PreUpdateComponentSystem(componentSystem, componentVector, this);
      //      }

      //      for (EntityIndexType componentIndex = 0; componentIndex < m_freeComponentIndices[TypeUtilities::IndexOf<typename decltype(componentSystem)::Component, App::Components>()]; componentIndex++) {
      //        auto component = &componentVector->at(componentIndex);
      //        componentSystem.UpdateComponent(component, this);
      //      }

      //      if constexpr (TestTypeHasPostUpdateThisComponentSystem<Derived, decltype(componentSystem)>{}) {
      //        static_cast<Derived*>(this)->PostUpdateComponentSystem(componentSystem, componentVector, this);
      //      }

      //    }

      //  },
      //  m_componentSystems);
    }

    void RenderComponentSystems() {

      TypeUtilities::ForTuple(
        [this](auto componentSystem) {

          if constexpr (TestTypeHasRenderComponent<decltype(componentSystem)>{}) {
            LOG(INFO) << "Rendering componentSystems typeid(decltype(componentSystem).name() = " << typeid(decltype(componentSystem)).name();

            std::vector<typename decltype(componentSystem)::Component> componentVector = std::get<std::vector<typename decltype(componentSystem)::Component>>(this->m_componentVectors);

            for (EntityIndexType componentIndex = 0; componentIndex < m_freeComponentIndices[TypeUtilities::IndexOf<typename decltype(componentSystem)::Component, App::Components>()]; componentIndex++) {
              auto component = componentVector.at(componentIndex);
              componentSystem.RenderComponent(component, this);
            }

          }

        },
        m_componentSystems);
    }

    virtual bool Initialize() override {

      if (PlatformApp<TSettings>::Initialize()) {
        this->m_applicationContext.m_name = "ndtechMagicGLApp";
        this->m_applicationContext.m_state = 2;

        m_componentCapacities.resize(m_numberOfComponentVectors);
        m_freeComponentIndices.resize(m_numberOfComponentVectors);

        this->m_renderingSystem.m_app = this;
        this->m_renderingSystem.m_componentSystems = &this->m_componentSystems;
        this->m_renderingSystem.m_freeComponentIndices = &this->m_freeComponentIndices;
        this->m_renderingSystem.m_componentVectors = &this->m_componentVectors;

        this->m_renderingSystem.Initialize();

        return this->AfterGraphicsInitialized();
      }
      else {
        return false;
      }

    };

    void AfterWindowSet() {
      InitializeComponentSystems(ComponentSystems{});
    }

    void Loop() {

      while (this->m_applicationContext.m_state) {

        this->m_timer.Tick([&]()
          {
            //
            // TODO: Update scene objects.
            //
            // Put time-based updates here. By default this code will run once per frame,
            // but if you change the StepTimer to use a fixed time step this code will
            // run as many times as needed to get to the current step.
            //

            UpdateComponentSystems(ComponentSystems{});

            this->Update(this->m_timer);

            this->m_renderingSystem.Render(this);
            //RenderComponentSystems();

          });
      }

      m_scheduler.Join();

    };

  protected:


    void IncreaseEntityStorageTo(EntityIndexType newCapacity)
    {
      assert(newCapacity > m_entitiesCapacity);

      m_entities.resize(newCapacity);

      for (auto i(m_entitiesCapacity); i < newCapacity; ++i)
      {
        auto& e(m_entities[i]);

        e.index = i;
        //e.bitset.reset();
        //e.alive = false;
      }

      m_entitiesCapacity = newCapacity;
    }

    void IncreaseEntityStorageIfNeeded() {
      if (m_entitiesCapacity > m_freeEntityIndex) return;
      IncreaseEntityStorageTo((m_entitiesCapacity + 10) * 2);
    }

    void IncreaseComponentStorageTo(size_t componentVectorNumber, EntityIndexType newCapacity)
    {
      assert(newCapacity > m_componentCapacities[componentVectorNumber]);

      int vectorNumber = 0;
      TypeUtilities::ForTuple(
        [this, &vectorNumber, &componentVectorNumber, newCapacity](auto componentVector) mutable {
          if (vectorNumber == componentVectorNumber) {
            std::get<decltype(componentVector)>(m_componentVectors).resize(newCapacity);
            m_componentCapacities[componentVectorNumber] = newCapacity;
          }

          vectorNumber++;
        },
        m_componentVectors);
    }

    template<typename ComponentType>
    void IncreaseComponentStorageTo(EntityIndexType newCapacity) {
      int componentVectorNumber = TypeUtilities::IndexOf<ComponentType, App::Components>();

      assert(newCapacity > m_componentCapacities[componentVectorNumber]);

      std::get<std::vector<ComponentType>>(m_componentVectors).resize(newCapacity);
      m_componentCapacities[componentVectorNumber] = newCapacity;
    }

    void IncreaseComponentStorageIfNeeded(size_t componentVectorNumber) {
      if (m_componentCapacities[componentVectorNumber] > m_freeComponentIndices[componentVectorNumber]) return;
      IncreaseComponentStorageTo(componentVectorNumber, (m_componentCapacities[componentVectorNumber] + 10) * 2);
    };

    template<typename ComponentType>
    void IncreaseComponentStorageIfNeeded() {
      int componentVectorNumber = TypeUtilities::IndexOf<ComponentType, App::Components>();
      if (m_componentCapacities[componentVectorNumber] > m_freeComponentIndices[componentVectorNumber]) return;
      IncreaseComponentStorageTo<ComponentType>((m_componentCapacities[componentVectorNumber] + 10) * 2);
    }

  };




}