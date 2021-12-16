#pragma once

#include "TypeUtilities.h"

namespace ndtech {

  template<typename ComponentsTypelist, typename ComponentSystemsTypelist>
  struct ApplicationSettings {
    using Components = ComponentsTypelist;
    using ComponentSystems = ComponentSystemsTypelist;

#if NDTECH_HOLO
    using ShaderTypes = TypeUtilities::Typelist< winrt::com_ptr<ID3D11VertexShader>, winrt::com_ptr<ID3D11PixelShader>, winrt::com_ptr<ID3D11GeometryShader>>;
#endif
#if NDTECH_ML
#endif

    using EntityIndexType = size_t;
    using EntityType = struct {
      EntityIndexType index;
      bool isAlive = false;
    };

    template <typename T>
    static constexpr bool isComponent() noexcept
    {
      return TypeUtilities::TypelistContains<T, Components>();
    };

    template <typename T>
    static constexpr bool isComponentSystem() noexcept
    {
      return TypeUtilities::TypelistContains<T, ComponentSystems>();
    };

  };

}