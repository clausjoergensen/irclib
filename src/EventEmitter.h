// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>

struct EventListenerBase {
    EventListenerBase() {}
    EventListenerBase(const std::string name) : name(name) {}
    virtual ~EventListenerBase() {}

    const std::string name;
};

template <typename... Args> struct EventListener : EventListenerBase {
    EventListener() {}
    EventListener(const std::string name, const std::function<void(Args...)> handler)
        : EventListenerBase(name), handler(handler) {}

    virtual ~EventListener() {}

    const std::function<void(Args...)> handler;
};

class EventEmitter {
  public:
    EventEmitter();
    ~EventEmitter();

    void on(const std::string eventName, const std::function<void()> handler);

    template <typename... Args>
    void on(const std::string eventName, const std::function<void(Args...)> handler);

    template <typename LambdaType> void on(const std::string eventName, const LambdaType lambda) {
        this->on(eventName, make_function(lambda));
    }

    template <typename... Args> void emit(const std::string eventName, const Args... args);

  private:
    std::multimap<std::string, std::shared_ptr<EventListenerBase>> listeners;

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
void EventEmitter::on(std::string eventName, std::function<void(Args...)> handler) {
    this->listeners.insert(
        std::make_pair(eventName, std::make_shared<EventListener<Args...>>(eventName, handler)));
}

template <typename... Args> void EventEmitter::emit(std::string eventName, Args... args) {
    auto range = this->listeners.equal_range(eventName);

    std::list<std::shared_ptr<EventListener<Args...>>> listeners;
    listeners.resize(std::distance(range.first, range.second));

    std::transform(range.first, range.second, listeners.begin(), [](auto pair) {
        return std::dynamic_pointer_cast<EventListener<Args...>>(pair.second);
    });

    for (auto &listener : listeners) {
        listener->handler(args...);
    }
}
