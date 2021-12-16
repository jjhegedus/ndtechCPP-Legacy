#pragma once

#include "EventArgs.h"
#include <type_traits>

namespace ndtech
{
    class EventHandlerBase
    {
    public:

        virtual void HandleEvent();

        virtual void HandleEvent(EventArgs* args) = 0;

    };

    template<typename HandlerArgsType>
    class EventHandler : EventHandlerBase
    {
    public:
        //using NoArgsHandlerFunctionPointerType = void (ThisObjectType::*)();
        using HandlerFunctionPointerType = void (*)(EventArgs* args);

        EventHandler<HandlerArgsType>() = delete;

        EventHandler<HandlerArgsType>(HandlerFunctionPointerType handlerFunctionPointer) :
            m_handlerFunctionPointer(handlerFunctionPointer)
        {
        }

        virtual ~EventHandler<HandlerArgsType>(void)
        {
        }

        //virtual void HandleEvent()
        //{
        //    (m_handlerFunctionPointer)();
        //}

        virtual void HandleEvent(EventArgs* args) override
        {
            (m_handlerFunctionPointer)(args);
        }

        HandlerFunctionPointerType GetHandlerFunctionPointer() override
        {
            return m_handlerFunctionPointer;
        }

    private:
        // private instance data
        //NoArgsHandlerFunctionPointerType m_noArgsHandlerFunctionPointer;
        HandlerFunctionPointerType m_handlerFunctionPointer;
    };

}