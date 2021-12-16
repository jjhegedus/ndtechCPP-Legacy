#pragma once

#include <exception>
#include "Event.h"
#include "PointerPressedEventArgs.h"


namespace ndtech
{

    struct PointerPressedEvent : public Event<PointerPressedEvent>
    {
    };
}
