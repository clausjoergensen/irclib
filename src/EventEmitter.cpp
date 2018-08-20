// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "EventEmitter.h"

EventEmitter::EventEmitter() {
}

EventEmitter::~EventEmitter() {
}

void EventEmitter::on(std::string eventName, std::function<void()> handler) {
    this->listeners.insert(
        std::make_pair(eventName, std::make_shared<EventListener<>>(eventName, handler))
    );
}
