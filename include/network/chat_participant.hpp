#pragma once
#include <string>
#include <memory>

class chat_participant {
public:
    virtual ~chat_participant() = default;
    virtual void deliver(const std::string &msg) = 0;
};
using chat_participant_ptr = std::shared_ptr<chat_participant>;
