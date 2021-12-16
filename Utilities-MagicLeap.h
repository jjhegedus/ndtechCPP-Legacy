#pragma once

#include "pch.h"

namespace ndtech {
  namespace utilities {

    /** Converts an MLTransform to a GLM matrix. */
    static glm::mat4 MLTransformToGLMMat4(const MLTransform &mlTransform) {

      glm::quat quaternion;
      quaternion.w = mlTransform.rotation.w;
      quaternion.x = mlTransform.rotation.x;
      quaternion.y = mlTransform.rotation.y;
      quaternion.z = mlTransform.rotation.z;

      return glm::translate(glm::mat4(), glm::vec3(mlTransform.position.x, mlTransform.position.y, mlTransform.position.z)) * glm::toMat4(quaternion);

    }


  }

}
