#pragma once

#include <vector>
#include <memory>
#include "EventHandler.h"
#include "EventArgs.h"

namespace ndtech
{
    template <typename ConcreteEventType>
    class Event
    {
    public:
        static void RegisterHandler(std::unique_ptr<EventHandlerBase> handler)
        {
            m_eventHandlers.push_back(std::move(handler));
        };

        //static IAsyncAction Raise();

        static winrt::Windows::Foundation::IAsyncAction Raise(std::unique_ptr<EventArgs> args)
        {
            using namespace winrt;

            co_await resume_background();

            for (auto & handler : m_eventHandlers)
            {
                handler->HandleEvent(args.get());
            }
        };



    protected:
        static std::vector<std::unique_ptr<EventHandlerBase>> m_eventHandlers;

    private:
        class Initializer
        {
        public:
            Initializer()
            {
                ndtech::Event::m_eventHandlers = std::vector<std::unique_ptr<EventHandlerBase>>();
            };;
        };

        friend class Initializer;

        static Initializer initializer;
    };

    template <typename ConcreteEventType>
    std::vector<std::unique_ptr<EventHandlerBase>> ndtech::Event<ConcreteEventType>::m_eventHandlers =
        std::vector<std::unique_ptr<EventHandlerBase>>();
}
