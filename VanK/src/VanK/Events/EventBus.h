#pragma once

#include "Event.h"
#include <queue>
#include <functional>
#include <unordered_map>
//replace with custom keycode
namespace Hazel {

    class EventListener {
    public:
        virtual void onEvent(Event& event) = 0;
    };

    class EventBus {
    public:
        // Post an event to the event queue
        void postEvent(const Event& event)
        {
            eventQueue.push(event);
        }

        // Process and dispatch all events in the queue
        void processEvents()
        {
            while (!eventQueue.empty())
            {
                Event event = eventQueue.front();
                eventQueue.pop();
                dispatchEvent(event);
            }
        }

        // Register an event listener for a specific event type
        template<typename T>
        void registerListener(EventListener* listener)
        {
            listeners[T::GetStaticType()].push_back(listener);
        }

        // Dispatch an event to the appropriate listeners
        void dispatchEvent(Event& event)
        {
            auto& listenerList = listeners[event.GetEventType()];
            for (auto listener : listenerList)
            {
                listener->onEvent(event);
                if (event.Handled) break; // Stop propagation if handled
            }
        }

    private:
        std::queue<Event> eventQueue;
        std::unordered_map<EventType, std::vector<EventListener*>> listeners;
    };

}