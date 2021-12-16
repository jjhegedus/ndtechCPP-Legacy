#pragma once

#include "EventArgs.h"

#include "winrt/Windows.UI.Input.Spatial.h"

namespace ndtech
{
    struct PointerPressedEventArgs : EventArgs
    {
        PointerPressedEventArgs() = delete;
        PointerPressedEventArgs(winrt::Windows::UI::Input::Spatial::SpatialPointerPose pointerPose);

        winrt::Windows::UI::Input::Spatial::SpatialPointerPose m_pointerPose = nullptr;
    };

}