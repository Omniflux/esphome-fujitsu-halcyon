#include "esphome-fujitsu-halcyon.h"

#include <array>

#include <esphome/core/helpers.h>
#include <esphome/core/version.h>

namespace esphome::fujitsu_general_airstage_h_controller {

static const auto TAG = "esphome::fujitsu_general_airstage_h_controller";

constexpr std::array ControllerName = { "Primary", "Secondary", "Undocumented" };

void FujitsuHalcyonController::setup() {
    this->controller = new fujitsu_general::airstage::h::Controller(
        static_cast<uart::IDFUARTComponent*>(this->parent_)->get_hw_serial_number(),
        this->controller_address_,
        {
            .Config = [this](const fujitsu_general::airstage::h::Config& data){ this->update_from_device(data); },
            .Error  = [this](const fujitsu_general::airstage::h::Packet& data){ this->update_from_device(data); },
            .ZoneConfig = [this](const fujitsu_general::airstage::h::ZoneConfig& data){ this->update_from_device(data); },
            .Function = [this](const fujitsu_general::airstage::h::Function& data){ this->update_from_device(data); },
            .ControllerConfig = [this](const uint8_t address, const fujitsu_general::airstage::h::Config& data){ this->update_from_controller(address, data); },
            .InitializationStage = [this](const fujitsu_general::airstage::h::InitializationStageEnum stage){
                this->initialization_sensor->publish_state(str_sprintf("(%d/%d)", stage, fujitsu_general::airstage::h::InitializationStageEnum::Complete));
            },
            .ReadBytes  = [this](uint8_t *buf, size_t length){
                this->read_array(buf, length);
                this->log_buffer("RX", buf, length);
            },
            .WriteBytes = [this](const uint8_t *buf, size_t length){
                this->write_array(buf, length);
                this->log_buffer("TX", buf, length);
            }
        },
        *static_cast<uart::IDFUARTComponent*>(this->parent_)->get_uart_event_queue()
    );

    if (!this->controller->start()) {
        ESP_LOGE(TAG, "Failed to start controller");
        this->mark_failed();
        return;
    }

    // Use specified sensor for this components reported temperature
    if (this->temperature_sensor_ != nullptr) {
        // Temperature sensor is in Fahrenheit, but need Celsius
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2025, 11, 0)
        const auto unit_of_measurement = this->temperature_sensor_->get_unit_of_measurement_ref();
        if (unit_of_measurement[unit_of_measurement.size() - 1] == 'F')
#else
        if (this->temperature_sensor_->get_unit_of_measurement().ends_with("F"))
#endif
        {
            this->temperature_sensor_->add_on_raw_state_callback([this](float state) {
                this->current_temperature = esphome::fahrenheit_to_celsius(state);
                this->publish_state();

                // Send this temperature to the Fujitsu IU
                this->controller->set_current_temperature(this->current_temperature);
            });

            this->current_temperature = esphome::fahrenheit_to_celsius(this->temperature_sensor_->state);
        }
        // Temperature sensor is in Celsius
        else
        {
            this->temperature_sensor_->add_on_raw_state_callback([this](float state) {
                this->current_temperature = state;
                this->publish_state();

                // Send this temperature to the Fujitsu IU
                this->controller->set_current_temperature(state);
            });

            this->current_temperature = this->temperature_sensor_->state;
        }
    }

    if (this->humidity_sensor_ != nullptr) {
        this->humidity_sensor_->add_on_raw_state_callback([this](float state) {
            this->current_humidity = state;
            this->publish_state();
        });

        this->current_humidity = this->humidity_sensor_->state;
    }

/*
    // Not sure if should timeout, or wait forever.
    // Not sure if getting stuck at can_proceed() causes boot failure count to increment
    // which can be problematic later
    this->set_timeout(10000, [this](){
        if (!this->can_proceed()) {
            ESP_LOGE(TAG, "Failed to initialize");
            this->mark_failed();
        }
    });
*/
}

void FujitsuHalcyonController::log_buffer(const char* dir, const uint8_t* buf, size_t length) {
    auto tbuf = std::vector<uint8_t>(buf, buf + length);
    for (auto &b : tbuf)
        b ^= 0xFF;

    this->tzsp_send(tbuf);
    ESP_LOGD(TAG, "%s: %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX %02hhX", dir, tbuf[0], tbuf[1], tbuf[2], tbuf[3], tbuf[4], tbuf[5], tbuf[6], tbuf[7]);
}

void FujitsuHalcyonController::dump_config() {
    LOG_CLIMATE("", "FujitsuHalcyonController", this);
    ESP_LOGCONFIG(TAG, "  Controller Address: %u (%s)", this->controller_address_, ControllerName[std::clamp(static_cast<size_t>(this->controller_address_), 0u, ControllerName.size() - 1)]);
    ESP_LOGCONFIG(TAG, "  Remote Temperature Controller Address: %u (%s)", this->temperature_controller_address_, ControllerName[std::clamp(static_cast<size_t>(this->temperature_controller_address_), 0u, ControllerName.size() - 1)]);
    LOG_SENSOR("  ", "Remote Temperature Controller Sensor", this->remote_sensor);
    LOG_SENSOR("  ", "Temperature Sensor", this->temperature_sensor_);
    LOG_SENSOR("  ", "Humidity Sensor", this->humidity_sensor_);
    ESP_LOGCONFIG(TAG, "  Ignore Lock: %s", this->ignore_lock_ ? "YES" : "NO");
    ESP_LOGCONFIG(TAG, "  Standby Mode: %s", this->standby_sensor->state ? "ACTIVE" : "NORMAL");

    if (this->controller->is_initialized()) {
        auto features = this->controller->get_features();

        ESP_LOGCONFIG(TAG, "  Additional Features:%s", features.FilterTimer || features.Maintenance || features.SensorSwitching ? "" : " NONE");
        if (features.FilterTimer)
            ESP_LOGCONFIG(TAG, "    - Filter Timer");
        if (features.Maintenance)
            ESP_LOGCONFIG(TAG, "    - Maintenance");
        if (features.SensorSwitching)
            ESP_LOGCONFIG(TAG, "    - Sensor Switching");
        if (features.Zones) {
            auto zones = this->controller->get_zones();

            ESP_LOGCONFIG(TAG, "    - Zones: %s", zones.EnabledZones.count());
            ESP_LOGCONFIG(TAG, "        Common Zone: %s", zones.ZoneCommon ? "YES" : "NO");
        }
    }

    if (!this->filter_sensor->is_internal())
        ESP_LOGCONFIG(TAG, "  Filter Timer: %s", this->filter_sensor->state ? "EXPIRED" : "OK");
    if (!this->use_sensor_switch->is_internal())
        ESP_LOGCONFIG(TAG, "  Use Temperature Sensor: %s", this->use_sensor_switch->state ? "YES" : "NO");

    LOG_TZSP("  ", this);

    this->check_uart_settings(
        fujitsu_general::airstage::h::UARTConfig.baud_rate,
        this->uart_stop_bits_to_uart_config_stop_bits(fujitsu_general::airstage::h::UARTConfig.stop_bits),
        this->uart_parity_to_uart_config_parity(fujitsu_general::airstage::h::UARTConfig.parity),
        this->uart_data_bits_to_uart_config_data_bits(fujitsu_general::airstage::h::UARTConfig.data_bits)
    );

    this->dump_traits_(TAG);
}

climate::ClimateTraits FujitsuHalcyonController::traits() {
    using namespace climate;

    auto features = this->controller->get_features();
    auto zones = this->controller->get_zones();
    auto traits = ClimateTraits();

    // Target temperature / Setpoint
    traits.set_visual_temperature_step(1);
    traits.set_visual_min_temperature(fujitsu_general::airstage::h::MinSetpoint);
    traits.set_visual_max_temperature(fujitsu_general::airstage::h::MaxSetpoint);
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2025, 11, 0)
    // Current temperature
    if (this->temperature_sensor_ != nullptr || !this->remote_sensor->is_internal())
        traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);

    // Current humidity
    if (this->humidity_sensor_ != nullptr)
        traits.add_feature_flags(climate::CLIMATE_SUPPORTS_CURRENT_HUMIDITY);
#else
    // Current temperature
    if (this->temperature_sensor_ != nullptr || !this->remote_sensor->is_internal())
        traits.set_supports_current_temperature(true);

    // Current humidity
    traits.set_supports_current_humidity(this->humidity_sensor_ != nullptr);
#endif

    // Mode
    if (features.Mode.Auto)
        traits.add_supported_mode(ClimateMode::CLIMATE_MODE_HEAT_COOL);
    if (features.Mode.Heat)
        traits.add_supported_mode(ClimateMode::CLIMATE_MODE_HEAT);
    if (features.Mode.Fan)
        traits.add_supported_mode(ClimateMode::CLIMATE_MODE_FAN_ONLY);
    if (features.Mode.Dry)
        traits.add_supported_mode(ClimateMode::CLIMATE_MODE_DRY);
    if (features.Mode.Cool)
        traits.add_supported_mode(ClimateMode::CLIMATE_MODE_COOL);

    // Fan mode / speed
    if (features.FanSpeed.Quiet)
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_QUIET);
    if (features.FanSpeed.Low)
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_LOW);
    if (features.FanSpeed.Medium)
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_MEDIUM);
    if (features.FanSpeed.High)
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_HIGH);
    if (features.FanSpeed.Auto)
        traits.add_supported_fan_mode(ClimateFanMode::CLIMATE_FAN_AUTO);

    // Economy mode
    if (features.EconomyMode)
    {
        traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_NONE);
        traits.add_supported_preset(ClimatePreset::CLIMATE_PRESET_ECO);
    }

    // Swing
    if (features.HorizontalLouvers || features.VerticalLouvers) {
        traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_OFF);
        if (features.HorizontalLouvers)
            traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_HORIZONTAL);
        if (features.VerticalLouvers)
            traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_VERTICAL);
        if (features.HorizontalLouvers && features.VerticalLouvers)
            traits.add_supported_swing_mode(ClimateSwingMode::CLIMATE_SWING_BOTH);
    }

    // Expose feature dependent components
    if (features.SensorSwitching && this->temperature_sensor_ != nullptr)
        this->use_sensor_switch->set_internal(false);
    if (features.HorizontalLouvers)
        this->advance_vertical_louver_button->set_internal(false);
    if (features.VerticalLouvers)
        this->advance_horizontal_louver_button->set_internal(false);
    if (features.FilterTimer) {
        this->filter_sensor->set_internal(false);
        this->reset_filter_button->set_internal(false);
    }

    // Zones
    if (features.Zones) {
        for (auto i = 0; i < this->zone_switches.size(); i++)
            if (zones.EnabledZones.test(i))
                this->zone_switches[i]->set_internal(false);

        this->zone_group_day_switch->set_internal(false);
        this->zone_group_night_switch->set_internal(false);
    }

    this->reinitialize_button->set_internal(false);

    return traits;
}

void FujitsuHalcyonController::control(const climate::ClimateCall& call) {
    using climate::ClimateMode;
    using climate::ClimatePreset;
    using climate::ClimateSwingMode;

    // Target temperature / Setpoint
    if (call.get_target_temperature().has_value())
        this->controller->set_setpoint(call.get_target_temperature().value(), this->ignore_lock_);

    // Economy mode
    if (call.get_preset().has_value())
        this->controller->set_economy(call.get_preset().value() == ClimatePreset::CLIMATE_PRESET_ECO, this->ignore_lock_);

    // Fan mode / speed
    if (call.get_fan_mode().has_value())
        this->controller->set_fan_speed(climate_fan_mode_to_fan_speed(call.get_fan_mode().value()), this->ignore_lock_);

    // Mode / enabled
    if (call.get_mode().has_value()) {
        if (call.get_mode().value() == ClimateMode::CLIMATE_MODE_OFF)
            this->controller->set_enabled(false, this->ignore_lock_);
        else {
            this->controller->set_enabled(true, this->ignore_lock_);
            this->controller->set_mode(climate_mode_to_mode(call.get_mode().value()), this->ignore_lock_);
        }
    }

    // Swing mode
    if (call.get_swing_mode().has_value()) {
        auto swing_mode = climate_swing_mode_to_swing_mode(call.get_swing_mode().value());
        this->controller->set_horizontal_swing(swing_mode.first, this->ignore_lock_);
        this->controller->set_vertical_swing(swing_mode.second, this->ignore_lock_);
    }

    this->publish_state();
}

void FujitsuHalcyonController::update_from_device(const fujitsu_general::airstage::h::Config& data) {
    using climate::ClimateFanMode;
    using climate::ClimateMode;
    using climate::ClimatePreset;
    using climate::ClimateSwingMode;

    auto need_to_publish = false;

    // Error sensor (binary)
    if (!this->error_sensor->has_state())
        this->error_sensor->publish_state(data.IndoorUnit.Error);

    // Error sensor (text)
    if (!this->error_code_sensor->has_state() && !data.IndoorUnit.Error)
        this->error_code_sensor->publish_state("");

    // Standby mode sensor
    // This can indicate defrosting, performing oil recovery, waiting for other units to complete....
    if (!this->standby_sensor->has_state() || data.IndoorUnit.StandbyMode != this->standby_sensor->state)
        this->standby_sensor->publish_state(data.IndoorUnit.StandbyMode);

    // Filter sensor
    if (this->controller->get_features().FilterTimer && (!this->filter_sensor->has_state() || data.IndoorUnit.FilterTimerExpired != this->filter_sensor->state))
        this->filter_sensor->publish_state(data.IndoorUnit.FilterTimerExpired);

    // Target temperature / Setpoint
    if (data.Setpoint != this->target_temperature) {
        this->target_temperature = data.Setpoint;
        need_to_publish = true;
    }

    // Economy mode
    if (data.Economy != (this->preset == ClimatePreset::CLIMATE_PRESET_ECO)) {
        this->preset = data.Economy ? ClimatePreset::CLIMATE_PRESET_ECO : ClimatePreset::CLIMATE_PRESET_NONE;
        need_to_publish = true;
    }

    // Fan mode / speed
    const auto fan_mode = fan_speed_to_climate_fan_mode(data.FanSpeed);
    if (fan_mode != this->fan_mode) {
        this->fan_mode = fan_mode;
        need_to_publish = true;
    }

    // Mode / enabled
    const auto mode = data.Enabled ? mode_to_climate_mode(data.Mode) : ClimateMode::CLIMATE_MODE_OFF;
    if (mode != this->mode) {
        this->mode = mode;
        need_to_publish = true;
    }

    // Swing mode
    const auto swing_mode = swing_mode_to_climate_swing_mode(data.SwingHorizontal, data.SwingVertical);
    if (swing_mode != this->swing_mode) {
        this->swing_mode = swing_mode;
        need_to_publish = true;
    }

    if (need_to_publish)
        this->publish_state();
}

void FujitsuHalcyonController::update_from_device(const fujitsu_general::airstage::h::ZoneConfig& data) {
    for (auto i = 0; i < this->zone_switches.size(); i++)
        this->zone_switches[i]->publish_state(data.ActiveZones[i]);

    this->zone_group_day_switch->publish_state(data.ActiveZoneGroups.Day);
    this->zone_group_night_switch->publish_state(data.ActiveZoneGroups.Night);
}

void FujitsuHalcyonController::update_from_device(const fujitsu_general::airstage::h::Packet& data) {
    using fujitsu_general::airstage::h::PacketTypeEnum;

    // Error packet
    if (data.Type == PacketTypeEnum::Error)
    {
        // Error sensor (boolean)
        if (!data.Error.ErrorCode == this->error_sensor->state)
            this->error_sensor->publish_state(data.Error.ErrorCode);

        // Error sensor (text)
        if (!data.Error.ErrorCode != this->error_code_sensor->get_raw_state().empty())
        {
            if (!data.Error.ErrorCode)
                this->error_code_sensor->publish_state("");
            else
            {
                std::array<uint8_t, 2> errorBytes = { data.SourceAddress, data.Error.ErrorCode };
                this->error_code_sensor->publish_state(format_hex_pretty(errorBytes.data(), errorBytes.size(), ' '));
            }
        }
    }
}

void FujitsuHalcyonController::update_from_device(const fujitsu_general::airstage::h::Function& data) {
    this->function->publish_state(data.Function);
    this->function_value->publish_state(data.Value);
    this->function_unit->publish_state(data.Unit);
}

void FujitsuHalcyonController::update_from_controller(const uint8_t address, const fujitsu_general::airstage::h::Config& data) {
    if (address == this->temperature_controller_address_ && data.Controller.Temperature) {
        // Make remote controllers sensor visible
        if (this->remote_sensor->is_internal()) {
            this->remote_sensor->set_internal(false);

            // Use remote controllers sensor for this components reported temperature if other sensor is not configured
            if (this->temperature_sensor_ == nullptr) {
                this->remote_sensor->add_on_raw_state_callback([this](float temperature) {
                    this->current_temperature = temperature;
                    this->publish_state();
                });
            }
        }

        // Update remote controllers sensor component with remote controllers reported temperature
        if (data.Controller.Temperature != this->remote_sensor->raw_state)
            this->remote_sensor->publish_state(data.Controller.Temperature);
    }
}

constexpr climate::ClimateMode FujitsuHalcyonController::mode_to_climate_mode(const fujitsu_general::airstage::h::ModeEnum mode) {
    using climate::ClimateMode;
    using FujitsuMode = fujitsu_general::airstage::h::ModeEnum;

    switch (mode) {
        case FujitsuMode::Fan:  return ClimateMode::CLIMATE_MODE_FAN_ONLY;
        case FujitsuMode::Dry:  return ClimateMode::CLIMATE_MODE_DRY;
        case FujitsuMode::Cool: return ClimateMode::CLIMATE_MODE_COOL;
        case FujitsuMode::Heat: return ClimateMode::CLIMATE_MODE_HEAT;
        case FujitsuMode::Auto: return ClimateMode::CLIMATE_MODE_HEAT_COOL;

        // Should not get to this point
        default: return ClimateMode::CLIMATE_MODE_FAN_ONLY;
    }
}

constexpr climate::ClimateFanMode FujitsuHalcyonController::fan_speed_to_climate_fan_mode(const fujitsu_general::airstage::h::FanSpeedEnum fan_speed) {
    using climate::ClimateFanMode;
    using FujitsuFanMode = fujitsu_general::airstage::h::FanSpeedEnum;

    switch (fan_speed) {
        case FujitsuFanMode::Auto:   return ClimateFanMode::CLIMATE_FAN_AUTO;
        case FujitsuFanMode::Quiet:  return ClimateFanMode::CLIMATE_FAN_QUIET;
        case FujitsuFanMode::Low:    return ClimateFanMode::CLIMATE_FAN_LOW;
        case FujitsuFanMode::Medium: return ClimateFanMode::CLIMATE_FAN_MEDIUM;
        case FujitsuFanMode::High:   return ClimateFanMode::CLIMATE_FAN_HIGH;

        // Should not get to this point
        default: return ClimateFanMode::CLIMATE_FAN_AUTO;
    }
}

constexpr climate::ClimateSwingMode FujitsuHalcyonController::swing_mode_to_climate_swing_mode(bool horizontal, bool vertical) {
    using climate::ClimateSwingMode;

    if (horizontal && vertical)
        return ClimateSwingMode::CLIMATE_SWING_BOTH;
    else if (horizontal)
        return ClimateSwingMode::CLIMATE_SWING_HORIZONTAL;
    else if (vertical)
        return ClimateSwingMode::CLIMATE_SWING_VERTICAL;
    else
        return ClimateSwingMode::CLIMATE_SWING_OFF;
}

constexpr fujitsu_general::airstage::h::ModeEnum FujitsuHalcyonController::climate_mode_to_mode(climate::ClimateMode mode) {
    using climate::ClimateMode;
    using FujitsuMode = fujitsu_general::airstage::h::ModeEnum;

    switch (mode) {
        case ClimateMode::CLIMATE_MODE_HEAT_COOL: return FujitsuMode::Auto;
        case ClimateMode::CLIMATE_MODE_COOL:      return FujitsuMode::Cool;
        case ClimateMode::CLIMATE_MODE_HEAT:      return FujitsuMode::Heat;
        case ClimateMode::CLIMATE_MODE_FAN_ONLY:  return FujitsuMode::Fan;
        case ClimateMode::CLIMATE_MODE_DRY:       return FujitsuMode::Dry;

        // Should not get to this point if traits is respected
        default: return FujitsuMode::Fan;
    }
} 

constexpr fujitsu_general::airstage::h::FanSpeedEnum FujitsuHalcyonController::climate_fan_mode_to_fan_speed(climate::ClimateFanMode fan_speed) {
    using climate::ClimateFanMode;
    using FujitsuFanMode = fujitsu_general::airstage::h::FanSpeedEnum;

    switch (fan_speed) {
        case ClimateFanMode::CLIMATE_FAN_AUTO:   return FujitsuFanMode::Auto;
        case ClimateFanMode::CLIMATE_FAN_LOW:    return FujitsuFanMode::Low;
        case ClimateFanMode::CLIMATE_FAN_MEDIUM: return FujitsuFanMode::Medium;
        case ClimateFanMode::CLIMATE_FAN_HIGH:   return FujitsuFanMode::High;
        case ClimateFanMode::CLIMATE_FAN_QUIET:  return FujitsuFanMode::Quiet;

        // Should not get to this point if traits is respected
        default: return FujitsuFanMode::Auto;
    }
}

constexpr std::pair<bool, bool> FujitsuHalcyonController::climate_swing_mode_to_swing_mode(climate::ClimateSwingMode swing_mode) {
    using climate::ClimateSwingMode;
    using SwingMode = std::pair<bool, bool>;

    switch (swing_mode) {
        case ClimateSwingMode::CLIMATE_SWING_OFF:        return SwingMode(false, false);
        case ClimateSwingMode::CLIMATE_SWING_BOTH:       return SwingMode(true, true);
        case ClimateSwingMode::CLIMATE_SWING_VERTICAL:   return SwingMode(false, true);
        case ClimateSwingMode::CLIMATE_SWING_HORIZONTAL: return SwingMode(true, false);

        // Should not get to this point
        default: return SwingMode(false, false);
    }
}

constexpr uint8_t FujitsuHalcyonController::uart_data_bits_to_uart_config_data_bits(uart_word_length_t bits) {
    switch (bits) {
        case UART_DATA_5_BITS: return 5;
        case UART_DATA_6_BITS: return 6;
        case UART_DATA_7_BITS: return 7;

        // ESPHome UART only supports 5, 6, 7, 8
        default: return 8;
    }
}

constexpr uint8_t FujitsuHalcyonController::uart_stop_bits_to_uart_config_stop_bits(uart_stop_bits_t bits) {
    switch (bits) {
        case UART_STOP_BITS_1: return 1;

        // ESPHome UART only supports 1 and 2
        default: return 2;
    }
}

constexpr uart::UARTParityOptions FujitsuHalcyonController::uart_parity_to_uart_config_parity(uart_parity_t parity) {
    switch (parity) {
        case UART_PARITY_EVEN:  return uart::UART_CONFIG_PARITY_EVEN;
        case UART_PARITY_ODD:   return uart::UART_CONFIG_PARITY_ODD;
        default:                return uart::UART_CONFIG_PARITY_NONE;
    }
}

}
