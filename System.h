#pragma once

#include "App.h"
#include <vector>

namespace ndtech
{
  template <typename ConcreteSystem, typename Entity>
  class System
  {
  public:
    System() = default;
    ~System() = delete;
    System & operator=(const System&) = delete;
    System(const System&) = delete;
    System(System &&) = default;

    void Initialize(&App app)
    {
      static_cast<ConcreteSystem>(this)->Initialize(app);
    }

    void Uninitialize(&App app)
    {
      static_cast<ConcreteSystem>(this)->UnInitialize();
    }

    std::vector<Entity> InitializeEntities(&App app, std::vector<Entity> entities) {
      return static_cast<ConcreteSystem>(this)->InitializeEntities(app, entities);
    }

    std::vector<Entity> UninitializeEntities(&App app, std::vector<Entity> entities) {
      return static_cast<ConcreteSystem>(this)->UninitializeEntities(app, entities);
    }

    std::vector<Entity> Update(std::vector<Entity> entities)
    {
      return static_cast<ConcreteSystem>(this)->Update(entities);
    }

    void Render(&App app, const std::vector<Entity> entities)
    {
      static_cast<ConcreteSystem>(this)->Render(app, entities);
    }

  };
}