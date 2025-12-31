#pragma once

#include <functional>

#include <esphome/core/component.h>
#include <esphome/components/switch/switch.h>

namespace esphome::custom {

class CustomSwitch : public Component, public switch_::Switch {
    public:
        CustomSwitch(std::function<bool(bool)> func) : func(func) {};
        void write_state(bool state) override { this->publish_state(this->func(state) ? state : this->state); };

    private:
        CustomSwitch() {};
        std::function<bool(bool)> func;
};

}
