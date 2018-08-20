// Copyright (c) 2018 Claus Jørgensen
#pragma once

#include "stdafx.h"
#include "EventEmitter.h"

EventEmitter::EventEmitter() {}

EventEmitter::~EventEmitter() {}

void EventEmitter::on(const std::string eventName, std::function<void()> handler) {
    this->listeners.insert(
        std::make_pair(eventName, std::make_shared<EventListener<>>(eventName, handler)));
}
