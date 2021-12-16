#pragma once

#include "TypeUtilities.h"

namespace ndtech {
  namespace vertexTypes {

    struct VertexPosition {
      glm::vec3 pos;
    };

    struct VertexPositionColor {
      glm::vec3 pos;
      glm::vec3 color;
    };

    using VertexTypes = TypeUtilities::Typelist<VertexPosition, VertexPositionColor>;

  }
}
