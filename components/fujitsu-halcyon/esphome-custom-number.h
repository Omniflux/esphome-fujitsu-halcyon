#pragma once

#include <functional>

#include <esphome/core/component.h>
#include <esphome/components/number/number.h>

namespace esphome::custom {

class CustomNumber : public Component, public number::Number {
    public:
        CustomNumber(std::function<float(float)> func) : func(func) {};
        void control(float value) override { this->publish_state(this->func(value)); };

    private:
        CustomNumber() {};
        std::function<float(float)> func;
};

}
