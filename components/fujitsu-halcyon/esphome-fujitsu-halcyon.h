#pragma once

#include <functional>
#include <memory>

#include <esphome/core/component.h>
#include <esphome/components/binary_sensor/binary_sensor.h>
#include <esphome/components/button/button.h>
#include <esphome/components/climate/climate.h>
#include <esphome/components/sensor/sensor.h>
#include <esphome/components/switch/switch.h>
#include <esphome/components/text_sensor/text_sensor.h>
#include <esphome/components/uart/uart.h>
#include <esphome/components/uart/uart_component_esp_idf.h>

#include <esphome/components/tzsp/tzsp.h>

#include "Controller.h"

namespace esphome {
namespace fujitsu_halcyon {

class CustomButton : public Component, public button::Button {
    public:
        CustomButton(std::function<void()> func) : func(func) {};
        void press_action() override { this->func(); };

    private:
        CustomButton() {};
        std::function<void()> func;
};

class CustomSwitch : public Component, public switch_::Switch {
    public:
        CustomSwitch(std::function<bool(bool)> func) : func(func) {};
        void write_state(bool state) override { this->publish_state(this->func(state) ? state : this->state); };

    private:
        CustomSwitch() {};
        std::function<bool(bool)> func;
};

class FujitsuHalcyonController : public Component, public climate::Climate, public uart::UARTDevice, public tzsp::TZSPSender {
    public:
        binary_sensor::BinarySensor* standby_sensor = new binary_sensor::BinarySensor();
        binary_sensor::BinarySensor* filter_sensor = new binary_sensor::BinarySensor();
        binary_sensor::BinarySensor* error_sensor = new binary_sensor::BinarySensor();
        text_sensor::TextSensor* error_code_sensor = new text_sensor::TextSensor();
        sensor::Sensor* remote_sensor = new sensor::Sensor();

        CustomButton* reinitialize_button = new CustomButton([this]() { this->controller->reinitialize(); });
        CustomButton* reset_filter_button = new CustomButton([this]() { this->controller->reset_filter(this->ignore_lock_); });
        CustomButton* advance_vertical_louver_button = new CustomButton([this]() { this->controller->advance_vertical_louver(this->ignore_lock_); });
        CustomButton* advance_horizontal_louver_button = new CustomButton([this]() { this->controller->advance_horizontal_louver(this->ignore_lock_); });
        CustomSwitch* use_sensor_switch = new CustomSwitch([this](bool state) { return this->controller->use_sensor(state, this->ignore_lock_); });

        FujitsuHalcyonController(uart::IDFUARTComponent *parent, uint8_t controller_address) : uart::UARTDevice(parent), controller_address_(controller_address) {}

        void setup() override;
        void dump_config() override;
        float get_setup_priority() const override { return esphome::setup_priority::DATA; }
//        bool can_proceed() { return this->is_failed() || this->controller->is_initialized(); }

        void control(const climate::ClimateCall& call) override;
        climate::ClimateTraits traits() override;

        void set_ignore_lock(bool ignore_lock) { this->ignore_lock_ = ignore_lock; }
        void set_humidity_sensor(sensor::Sensor* humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
        void set_temperature_sensor(sensor::Sensor* temperature_sensor) { this->temperature_sensor_ = temperature_sensor; }
        void set_temperature_controller_address(uint8_t temperature_controller_address) { this->temperature_controller_address_ = temperature_controller_address; }

    protected:
        uint8_t controller_address_{};
        uint8_t temperature_controller_address_{};
        bool ignore_lock_{};
        sensor::Sensor* humidity_sensor_{};
        sensor::Sensor* temperature_sensor_{};

    private:
        fujitsu_halcyon_controller::Controller* controller;

        void update_from_device(const fujitsu_halcyon_controller::Config& data);
        void update_from_device(const fujitsu_halcyon_controller::Packet& data);
        void update_from_controller(const uint8_t address, const fujitsu_halcyon_controller::Config& data);

        void log_buffer(const char* dir, const uint8_t* buf, size_t length);

        static constexpr climate::ClimateMode mode_to_climate_mode(fujitsu_halcyon_controller::ModeEnum mode) noexcept;
        static constexpr climate::ClimateFanMode fan_speed_to_climate_fan_mode(fujitsu_halcyon_controller::FanSpeedEnum fan_speed) noexcept;
        static constexpr climate::ClimateSwingMode swing_mode_to_climate_swing_mode(bool horizontal, bool vertical) noexcept;

        static constexpr fujitsu_halcyon_controller::ModeEnum climate_mode_to_mode(climate::ClimateMode mode) noexcept;
        static constexpr fujitsu_halcyon_controller::FanSpeedEnum climate_fan_mode_to_fan_speed(climate::ClimateFanMode fan_speed) noexcept;
        static constexpr std::pair<bool, bool> climate_swing_mode_to_swing_mode(climate::ClimateSwingMode swing_mode) noexcept;

        static constexpr uint8_t uart_data_bits_to_uart_config_data_bits(uart_word_length_t bits) noexcept;
        static constexpr uint8_t uart_stop_bits_to_uart_config_stop_bits(uart_stop_bits_t bits) noexcept;
        static constexpr uart::UARTParityOptions uart_parity_to_uart_config_parity(uart_parity_t parity) noexcept;
};

}
}
