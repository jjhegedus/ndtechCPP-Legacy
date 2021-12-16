#pragma once

#include "EventArgs.h"

namespace ndtech
{
        template<typename ThisObjectType>
        class MemberEventHandler :
            public EventHandlerBase
        {
        public:
            //using NoArgsHandlerFunctionPointerType = void (ThisObjectType::*)();
            using HandlerFunctionPointerType = void (ThisObjectType::*)(EventArgs* args);
    
            MemberEventHandler() = delete;

            MemberEventHandler(
                ThisObjectType* thisObject,
                HandlerFunctionPointerType handlerFunctionPointer) :
                m_thisObject(thisObject),
                m_handlerFunctionPointer(handlerFunctionPointer)
            {
            }

            virtual ~MemberEventHandler(void)
            {
            }

            //virtual void HandleEvent()
            //{
            //    (m_thisObject->*m_handlerFunctionPointer)();
            //}

            virtual void HandleEvent(EventArgs* args)
            {
                (m_thisObject->*m_handlerFunctionPointer)(args);
            }

            HandlerFunctionPointerType GetHandlerFunctionPointer()
            {
                return m_handlerFunctionPointer;
            }

            ThisObjectType* GetThisObject()
            {
                return m_thisObject;
            }

        private:
            // private instance data
            ThisObjectType* m_thisObject;
            //NoArgsHandlerFunctionPointerType m_noArgsHandlerFunctionPointer;
            HandlerFunctionPointerType m_handlerFunctionPointer;
        };

}