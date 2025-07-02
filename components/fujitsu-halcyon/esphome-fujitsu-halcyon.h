#pragma once

#include <memory>

#include <esphome/core/component.h>
#include <esphome/components/binary_sensor/binary_sensor.h>
#include <esphome/components/climate/climate.h>
#include <esphome/components/sensor/sensor.h>
#include <esphome/components/text_sensor/text_sensor.h>
#include <esphome/components/uart/uart.h>
#include <esphome/components/uart/uart_component_esp_idf.h>

#include <esphome/components/tzsp/tzsp.h>

#include "esphome-custom-button.h"
#include "esphome-custom-number.h"
#include "esphome-custom-switch.h"
#include "Controller.h"

namespace esphome {
namespace fujitsu_halcyon {

class FujitsuHalcyonController : public Component, public climate::Climate, public uart::UARTDevice, public tzsp::TZSPSender {
    public:
        binary_sensor::BinarySensor* standby_sensor = new binary_sensor::BinarySensor();
        binary_sensor::BinarySensor* filter_sensor = new binary_sensor::BinarySensor();
        binary_sensor::BinarySensor* error_sensor = new binary_sensor::BinarySensor();
        text_sensor::TextSensor* error_code_sensor = new text_sensor::TextSensor();
        sensor::Sensor* remote_sensor = new sensor::Sensor();

        custom::CustomButton* reinitialize_button = new custom::CustomButton([this]() { this->controller->reinitialize(); });
        custom::CustomButton* reset_filter_button = new custom::CustomButton([this]() { this->controller->reset_filter(this->ignore_lock_); });
        custom::CustomButton* advance_vertical_louver_button = new custom::CustomButton([this]() { this->controller->advance_vertical_louver(this->ignore_lock_); });
        custom::CustomButton* advance_horizontal_louver_button = new custom::CustomButton([this]() { this->controller->advance_horizontal_louver(this->ignore_lock_); });
        custom::CustomSwitch* use_sensor_switch = new custom::CustomSwitch([this](bool state) { return this->controller->use_sensor(state, this->ignore_lock_); });

        custom::CustomNumber* function = new custom::CustomNumber([this](float state) { return int(state); });
        custom::CustomNumber* function_value = new custom::CustomNumber([this](float state) { return int(state); });
        custom::CustomNumber* function_unit = new custom::CustomNumber([this](float state) { return int(state); });
        custom::CustomButton* get_function = new custom::CustomButton([this]() {
            if (this->function->has_state() && this->function_unit->has_state()) {
                this->function_value->publish_state(NAN);
                this->controller->get_function(this->function->state, this->function_unit->state);
            }
        });
        custom::CustomButton* set_function = new custom::CustomButton([this]() {
            if (this->function->has_state() && this->function_value->has_state() && this->function_unit->has_state())
                this->controller->set_function(this->function->state, this->function_value->state, this->function_unit->state);
        });

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
        void update_from_device(const fujitsu_halcyon_controller::Function& data);
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
