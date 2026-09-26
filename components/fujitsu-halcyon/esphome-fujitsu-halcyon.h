#pragma once

#include <array>
#include <cmath>
#include <memory>

#include <esphome/core/component.h>
#include <esphome/core/helpers.h>
#include <esphome/components/binary_sensor/binary_sensor.h>
#include <esphome/components/button/button.h>
#include <esphome/components/climate/climate.h>
#include <esphome/components/number/number.h>
#include <esphome/components/sensor/sensor.h>
#include <esphome/components/switch/switch.h>
#include <esphome/components/text_sensor/text_sensor.h>
#include <esphome/components/uart/uart.h>
#include <esphome/components/uart/uart_component_esp_idf.h>

#if defined(USE_TZSP)
#include <esphome/components/tzsp/tzsp.h>
#endif

#include "Controller.h"

namespace esphome::fujitsu_general_airstage_h_controller {

#if defined(USE_TZSP)
class FujitsuHalcyonController : public Component, public climate::Climate, public uart::UARTDevice, public tzsp::TZSPSender {
#else
class FujitsuHalcyonController : public Component, public climate::Climate, public uart::UARTDevice {
#endif
    public:

        // Called by the ReinitializeButton entity (created inline in python).
        // Also resets the initialization watchdog, see check_init_timeout_().
        void reinitialize() {
            this->init_started_ms_ = millis();
            this->init_attempts_ = 0;
            this->controller->reinitialize();
        }
        // How long initialization may take before the watchdog restarts it. 0 disables.
        void set_init_timeout(uint32_t timeout_ms) { this->init_timeout_ms_ = timeout_ms; }
        // Called by the ResetFilterButton entity (created inline in python, see climate.py).
        void reset_filter() { this->controller->reset_filter(this->ignore_lock_); }
        // Called by the AdvanceVerticalLouverButton / AdvanceHorizontalLouverButton
        // entities, created inline in python and parented to this controller.
        void advance_vertical_louver() { this->controller->advance_vertical_louver(this->ignore_lock_); }
        void advance_horizontal_louver() { this->controller->advance_horizontal_louver(this->ignore_lock_); }
        // The use_sensor switch is created inline in python and parented to this
        // controller. It calls use_sensor() to write, and set_use_sensor_switch()
        // gives the controller a pointer so it can publish the unit state back.
        // The switch state is the user's intent. What the unit actually receives
        // also depends on the external sensor being usable, see apply_use_sensor_().
        bool use_sensor(bool state) { return this->controller->use_sensor(state && this->sensor_usable_(), this->ignore_lock_); }
        void set_use_sensor_switch(switch_::Switch* sw) { this->use_sensor_switch_ = sw; }
        // How long the external sensor may go without a valid reading before the
        // unit is switched back to its own sensor. 0 disables the check.
        void set_sensor_timeout(uint32_t timeout_ms) { this->sensor_timeout_ms_ = timeout_ms; }

        // Called by / set on the zone switches and the filter binary sensor, which are
        // created inline in python (switches parented, the sensor set through a pointer).
        bool set_zone(uint8_t zone, bool state) { return this->controller->set_zone(zone, state, this->ignore_lock_); }
        bool set_zone_group_day(bool state) { return this->controller->set_zone_group_day(state, this->ignore_lock_); }
        bool set_zone_group_night(bool state) { return this->controller->set_zone_group_night(state, this->ignore_lock_); }
        void set_zone_switch(uint8_t i, switch_::Switch* sw) { this->zone_switches_[i] = sw; }
        void set_zone_group_day_switch(switch_::Switch* sw) { this->zone_group_day_switch_ = sw; }
        void set_zone_group_night_switch(switch_::Switch* sw) { this->zone_group_night_switch_ = sw; }
        void set_filter_sensor(binary_sensor::BinarySensor* s) { this->filter_sensor_ = s; }
        void set_remote_sensor(sensor::Sensor* s) { this->remote_sensor_ = s; }
        // Always-present diagnostics, created in python and set here so the hub can publish to them.
        void set_standby_sensor(binary_sensor::BinarySensor* s) { this->standby_sensor_ = s; }
        void set_error_sensor(binary_sensor::BinarySensor* s) { this->error_sensor_ = s; }
        void set_connected_sensor(binary_sensor::BinarySensor* s) { this->connected_sensor_ = s; }
        void set_error_code_sensor(text_sensor::TextSensor* s) { this->error_code_sensor_ = s; }
        void set_initialization_sensor(text_sensor::TextSensor* s) { this->initialization_sensor_ = s; }
        void set_supported_features_sensor(text_sensor::TextSensor* s) { this->supported_features_sensor_ = s; }

        // The function number entities and the get/set function buttons are created
        // inline in python. The numbers are registered here via setters, the buttons
        // call get_function() / set_function().
        void set_function_number(number::Number* n) { this->function_number_ = n; }
        void set_function_value_number(number::Number* n) { this->function_value_number_ = n; }
        void set_function_unit_number(number::Number* n) { this->function_unit_number_ = n; }
        void get_function() {
            if (this->function_number_->has_state() && this->function_unit_number_->has_state()) {
                this->function_value_number_->publish_state(NAN);
                this->controller->get_function(this->function_number_->state, this->function_unit_number_->state);
            }
        }
        void set_function() {
            if (this->function_number_->has_state() && this->function_value_number_->has_state() && this->function_unit_number_->has_state())
                this->controller->set_function(this->function_number_->state, this->function_value_number_->state, this->function_unit_number_->state);
        }

        explicit FujitsuHalcyonController(uint8_t controller_address) : controller_address_(controller_address) {}

        void loop() override;
        void setup() override;
        void dump_config() override;
        float get_setup_priority() const override { return esphome::setup_priority::DATA; }

        void control(const climate::ClimateCall& call) override;
        climate::ClimateTraits traits() override;

        void set_ignore_lock(bool ignore_lock) { this->ignore_lock_ = ignore_lock; }
        void set_humidity_sensor(sensor::Sensor* humidity_sensor) { this->humidity_sensor_ = humidity_sensor; }
        void set_temperature_sensor(sensor::Sensor* temperature_sensor) { this->temperature_sensor_ = temperature_sensor; }
        void set_temperature_controller_address(uint8_t temperature_controller_address) { this->temperature_controller_address_ = temperature_controller_address; }

        // Feature negotiation overrides (called from to_code() in climate.py).
        // Setters mutate features_override_ in place; fields not touched keep the
        // DefaultFeatures value the struct was initialized with.
        void set_autoconf(bool v) { this->autoconf_ = v; }
        void set_supported_modes(bool a, bool h, bool f, bool d, bool c) {
            this->features_override_.Mode.Auto = a;
            this->features_override_.Mode.Heat = h;
            this->features_override_.Mode.Fan  = f;
            this->features_override_.Mode.Dry  = d;
            this->features_override_.Mode.Cool = c;
        }
        void set_supported_fan_modes(bool q, bool l, bool m, bool h, bool a) {
            this->features_override_.FanSpeed.Quiet  = q;
            this->features_override_.FanSpeed.Low    = l;
            this->features_override_.FanSpeed.Medium = m;
            this->features_override_.FanSpeed.High   = h;
            this->features_override_.FanSpeed.Auto   = a;
        }
        void set_supported_swing_modes(bool vert, bool horiz) {
            this->features_override_.VerticalLouvers   = vert;
            this->features_override_.HorizontalLouvers = horiz;
        }
        void set_supported_features(bool filter_timer, bool sensor_switching, bool maintenance, bool economy) {
            this->features_override_.FilterTimer     = filter_timer;
            this->features_override_.SensorSwitching = sensor_switching;
            this->features_override_.Maintenance     = maintenance;
            this->features_override_.EconomyMode     = economy;
        }

        // Track which feature entities the user declared in YAML, to warn at
        // initialization if the unit does not actually report that feature.
        void set_use_sensor_declared(bool v)    { this->use_sensor_declared_ = v; }
        void set_filter_entity_declared(bool v) { this->filter_entity_declared_ = v; }
        void set_louver_v_declared(bool v)      { this->louver_v_declared_ = v; }
        void set_louver_h_declared(bool v)      { this->louver_h_declared_ = v; }
        void set_zones_declared(bool v)         { this->zones_declared_ = v; }

    protected:
        uint8_t controller_address_{};
        uint8_t temperature_controller_address_{};
        bool ignore_lock_{};

        // Set from the UART read/write callbacks; used by the transmit-token
        // watchdog scheduled in setup() to detect a read-only (never granted the
        // token) controller.
        bool received_bytes_{false};
        bool transmitted_{false};

        // Initialization watchdog. If the sequence has not reached Complete within
        // init_timeout_ms_, and the unit has answered at least once, it is restarted
        // (same as the Reinitialize button). A missed packet during startup, or a glitch
        // that reboots the ESP mid-sequence, otherwise leaves the component stuck
        // with no features and no control until someone presses Reinitialize.
        uint32_t init_timeout_ms_{0};
        uint32_t init_started_ms_{0};
        uint8_t init_attempts_{0};
        void check_init_timeout_();

        sensor::Sensor* humidity_sensor_{};
        sensor::Sensor* temperature_sensor_{};

        // Feature negotiation state. Initialized to DefaultFeatures so anything not
        // overridden by YAML keeps the in-code default. Applied to Controller in setup().
        bool autoconf_ = true;
        fujitsu_general::airstage::h::Features features_override_ = fujitsu_general::airstage::h::DefaultFeatures;

        // Back-pointers to feature entities created inline in python, null when the
        // matching key is absent. The hub publishes unit state through them.
        switch_::Switch* use_sensor_switch_{nullptr};
        binary_sensor::BinarySensor* filter_sensor_{nullptr};
        sensor::Sensor* remote_sensor_{nullptr};
        binary_sensor::BinarySensor* standby_sensor_{nullptr};
        binary_sensor::BinarySensor* error_sensor_{nullptr};
        binary_sensor::BinarySensor* connected_sensor_{nullptr};
        text_sensor::TextSensor* error_code_sensor_{nullptr};
        text_sensor::TextSensor* initialization_sensor_{nullptr};
        text_sensor::TextSensor* supported_features_sensor_{nullptr};
        number::Number* function_number_{nullptr};
        number::Number* function_value_number_{nullptr};
        number::Number* function_unit_number_{nullptr};
        std::array<switch_::Switch*, fujitsu_general::airstage::h::MaxZone> zone_switches_{};
        switch_::Switch* zone_group_day_switch_{nullptr};
        switch_::Switch* zone_group_night_switch_{nullptr};

        // Set from to_code() when the corresponding feature entity is declared.
        bool use_sensor_declared_{false};
        bool filter_entity_declared_{false};
        bool louver_v_declared_{false};
        bool louver_h_declared_{false};
        bool zones_declared_{false};

        // The use_sensor switch is a plain Switch (not a Component), so nothing
        // restores it on boot. Its restored state is read in setup() and applied
        // once the unit confirms sensor switching, since a write before that is
        // rejected while the feature flag is still false.
        optional<bool> pending_use_sensor_{};
        bool use_sensor_applied_{false};

        // External sensor freshness. The unit is only told to use the external
        // sensor while a valid reading exists and is not older than
        // sensor_timeout_ms_. An unavailable Home Assistant sensor reports NaN, and
        // a lost API connection reports nothing at all, so the check is based on
        // the age of the last valid reading rather than on receiving NaN.
        uint32_t sensor_timeout_ms_{0};
        uint32_t last_valid_temperature_ms_{0};
        bool temperature_valid_{false};
        bool temperature_stale_{false};
        bool sensor_usable_() const { return this->temperature_valid_ && !this->temperature_stale_; }
        bool apply_use_sensor_();
        void check_sensor_timeout_();

    private:
        // Initialized in setup(). Stays nullptr if setup() bails out early (e.g.
        // uart_set_mode failure), so dump_config()/traits() must null-check before
        // dereferencing since they can run on a failed component.
        fujitsu_general::airstage::h::Controller* controller{nullptr};

        void update_from_device(const fujitsu_general::airstage::h::Config& data);
        void update_from_device(const fujitsu_general::airstage::h::ZoneConfig& data);
        void update_from_device(const fujitsu_general::airstage::h::Packet& data);
        void update_from_device(const fujitsu_general::airstage::h::Function& data);
        void update_from_controller(const uint8_t address, const fujitsu_general::airstage::h::Config& data);
        void on_initialization_stage(const fujitsu_general::airstage::h::InitializationStageEnum stage);

        void log_buffer(const char* dir, const uint8_t* buf, size_t length);

        void format_undeclared_features_(char* buf, size_t size);

        static constexpr climate::ClimateMode mode_to_climate_mode(fujitsu_general::airstage::h::ModeEnum mode) noexcept;
        static constexpr climate::ClimateFanMode fan_speed_to_climate_fan_mode(fujitsu_general::airstage::h::FanSpeedEnum fan_speed) noexcept;
        static constexpr climate::ClimateSwingMode swing_mode_to_climate_swing_mode(bool horizontal, bool vertical) noexcept;

        static constexpr fujitsu_general::airstage::h::ModeEnum climate_mode_to_mode(climate::ClimateMode mode) noexcept;
        static constexpr fujitsu_general::airstage::h::FanSpeedEnum climate_fan_mode_to_fan_speed(climate::ClimateFanMode fan_speed) noexcept;
        static constexpr std::pair<bool, bool> climate_swing_mode_to_swing_mode(climate::ClimateSwingMode swing_mode) noexcept;
};

// Feature entities created in python only when declared, parented to the controller
// with register_parented. They call it back in press_action / write_state.
class AdvanceVerticalLouverButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->advance_vertical_louver(); }
};

class AdvanceHorizontalLouverButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->advance_horizontal_louver(); }
};

// The use_sensor switch. write_state() routes the request to the unit and publishes
// the resulting state (or reverts to the current state if the unit rejected it).
class UseSensorSwitch : public switch_::Switch, public Parented<FujitsuHalcyonController> {
    protected:
        void write_state(bool state) override {
            this->publish_state(this->parent_->use_sensor(state) ? state : this->state);
        }
};

class ResetFilterButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->reset_filter(); }
};

// One class per zone, the zone index is set from to_code().
class ZoneSwitch : public switch_::Switch, public Parented<FujitsuHalcyonController> {
    public:
        void set_zone_index(uint8_t zone) { this->zone_ = zone; }
    protected:
        void write_state(bool state) override {
            this->publish_state(this->parent_->set_zone(this->zone_, state) ? state : this->state);
        }
        uint8_t zone_{0};
};

class ZoneGroupDaySwitch : public switch_::Switch, public Parented<FujitsuHalcyonController> {
    protected:
        void write_state(bool state) override {
            this->publish_state(this->parent_->set_zone_group_day(state) ? state : this->state);
        }
};

class ZoneGroupNightSwitch : public switch_::Switch, public Parented<FujitsuHalcyonController> {
    protected:
        void write_state(bool state) override {
            this->publish_state(this->parent_->set_zone_group_night(state) ? state : this->state);
        }
};

// Always-present controls, created inline in python.
class ReinitializeButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->reinitialize(); }
};

class GetFunctionButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->get_function(); }
};

class SetFunctionButton : public button::Button, public Parented<FujitsuHalcyonController> {
    protected:
        void press_action() override { this->parent_->set_function(); }
};

// Raw function register value, stored as an integer.
class FunctionNumber : public number::Number {
    protected:
        void control(float value) override { this->publish_state((int) value); }
};

}
