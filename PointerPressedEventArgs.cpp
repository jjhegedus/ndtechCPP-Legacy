#include "PointerPressedEventArgs.h"


ndtech::PointerPressedEventArgs::PointerPressedEventArgs(winrt::Windows::UI::Input::Spatial::SpatialPointerPose pointerPose) :
  EventArgs(),
  m_pointerPose(pointerPose)

{
};