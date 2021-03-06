// This code is licensed under MIT license (see LICENSE.txt for details)
#pragma once

#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <mutex>

namespace events {

struct EventListenerBase {
    EventListenerBase() {}
    EventListenerBase(const std::string event_name) : event_name(event_name) {}
    
    virtual ~EventListenerBase() {}

    const std::string event_name;
};

template <typename... Args> struct EventListener : EventListenerBase {
    EventListener() {}    
    EventListener(const std::string event_name, const std::function<void(Args...)> handler)
        : EventListenerBase(event_name), handler(handler) {}

    virtual ~EventListener() {}

    const std::function<void(Args...)> handler;
};

class EventEmitter {
  public:
    EventEmitter() {}
    ~EventEmitter() {}

    void on(const std::string event_name, const std::function<void()> handler) noexcept;

    template <typename... Args>
    void on(const std::string event_name, const std::function<void(Args...)> handler) noexcept;

    template <typename LambdaType> void on(const std::string event_name, const LambdaType lambda) noexcept {
        this->on(event_name, make_function(lambda));
    }

    template <typename... Args> void emit(const std::string event_name, const Args... args) noexcept;

    EventEmitter(const EventEmitter&) = delete;
    const EventEmitter& operator=(const EventEmitter&) = delete;

  private:
    std::multimap<std::string, std::shared_ptr<EventListenerBase>> listeners;
    std::mutex mutex;

    // http://stackoverflow.com/a/21000981

    template <typename T>
    struct function_traits : public function_traits<decltype(&T::operator())> {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType (ClassType::*)(Args...) const> {
        typedef std::function<ReturnType(Args...)> f_type;
    };

    template <typename L> typename function_traits<L>::f_type make_function(L l) {
        return (typename function_traits<L>::f_type)(l);
    }
};

template <typename... Args>
void EventEmitter::on(std::string event_name, std::function<void(Args...)> handler) noexcept {
    std::lock_guard<std::mutex> lock(mutex);
    this->listeners.insert(
        std::make_pair(event_name, std::make_shared<EventListener<Args...>>(event_name, handler)));
}

template <typename... Args> 
void EventEmitter::emit(std::string event_name, Args... args) noexcept {    
    std::list<std::shared_ptr<EventListener<Args...>>> listeners;   
    
    {
        std::lock_guard<std::mutex> lock(mutex);

        auto range = this->listeners.equal_range(event_name);

        listeners.resize(std::distance(range.first, range.second));

        std::transform(range.first, range.second, listeners.begin(), [](auto pair) {
            return std::dynamic_pointer_cast<EventListener<Args...>>(pair.second);
        });
    }

    for (auto& listener : listeners) {
        listener->handler(args...);
    }
}

} // namespace events
