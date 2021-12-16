#pragma once

#include "TypeUtilities.h"

/* User specified feature pre-processor flags
   These all default to off unless specified by the build system

  NDTECH_SUPPORTS_HAND_TRACKING \
  NDTECH_SUPPORTS_FINGER_TRACKING \
  NDTECH_SUPPORTS_BACK_OF_LEFT_HAND_GESTURE \
  NDTECH_SUPPORTS_BACK_OF_RIGHT_HAND_GESTURE \

*/

namespace ndtech {
  namespace features {
    struct hand_tracking;
    struct finger_tracking;
    struct back_of_left_hand_gesture;
    struct back_of_right_hand_gesture;

    template<typename TestType, typename Feature>
    constexpr bool SupportsFeature() {
      return TypeUtilities::Contains<Feature, typename TestType::Features>();
    }

  }
}