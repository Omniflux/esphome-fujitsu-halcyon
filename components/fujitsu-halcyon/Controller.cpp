#include "Controller.h"

#include <algorithm>
#include <cmath>

//#include <esp_log.h>
// Log through esphome instead of standard esp logging
#include <esphome/core/log.h>
using esphome::esp_log_printf_;

namespace fujitsu_general::airstage::h {

static const char* TAG = "fujitsu_general::airstage::h::Controller";

bool Controller::start() {
    int err;
    auto uart_config = UARTConfig;
    constexpr int intr_alloc_flags = 0;
    constexpr uint32_t queue_size = 20; // ?
    constexpr uint32_t stack_depth = 4096; // ?
    constexpr UBaseType_t task_priority = 12; // ?

    // User should have called uart_set_pin before this point if necessary

    if (this->uart_event_queue == nullptr && uart_is_driver_installed(this->uart_num)) {
        err = uart_driver_delete(this->uart_num);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete UART driver: %s", esp_err_to_name(err));
            return false;
        }
    }

    if (!uart_is_driver_installed(this->uart_num)) {
        const auto buffer_size = UART_HW_FIFO_LEN(this->uart_num) * 2;
        err = uart_driver_install(this->uart_num, buffer_size, buffer_size, queue_size, &this->uart_event_queue, intr_alloc_flags);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(err));
            return false;
        }
    }

    err = uart_param_config(this->uart_num, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART: %s", esp_err_to_name(err));
        return false;
    }

    err = uart_set_mode(this->uart_num, UART_MODE_RS485_HALF_DUPLEX);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART mode: %s", esp_err_to_name(err));
        return false;
    }

    // Ensure large enough not to trigger mid frame, resynchronize from rx_timeout instead
    err = uart_set_rx_full_threshold(this->uart_num, Packet::FrameSize * 4);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART RX full threshold: %s", esp_err_to_name(err));
        return false;
    }

    // Default timeout (time to transmit 10 characters at 500bps) is too long
    // If we wait for default timeout the transmit window is over before processing even begins.
    err = uart_set_rx_timeout(this->uart_num, UARTInterPacketSymbolSpacing);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART RX timeout: %s", esp_err_to_name(err));
        return false;
    }

    xTaskCreate([](void* o){ static_cast<Controller*>(o)->uart_event_task(); }, "UART_Event", stack_depth, this, task_priority, NULL);

    return true;
}

void Controller::uart_event_task() {
    uart_event_t event;

    for(;;) {
        if (xQueueReceive(this->uart_event_queue, &event, portMAX_DELAY)) {
            switch(event.type) {
                [[likely]] case UART_DATA:
                    Packet::Buffer buffer;
                    size_t buffer_len;

                    // Discard partial frame
                    uart_get_buffered_data_len(this->uart_num, &buffer_len);
                    if (auto discard = buffer_len % buffer.size()) {
                        this->uart_read_bytes(buffer.data(), discard);
                        ESP_LOGW(TAG, "Discarded %d bytes", discard);
                    }

                    // For each frame
                    for (auto i = 0; i < event.size / buffer.size(); i++) {
                        this->uart_read_bytes(buffer.data(), buffer.size());
                        uart_get_buffered_data_len(this->uart_num, &buffer_len);
                        this->process_packet(buffer, buffer_len == 0 /* Indicates final packet on wire */);
                    }

                    break;

                case UART_BREAK:
                    // TODO Why rx break after tx?
                    ESP_LOGD(TAG, "UART break!");
                    break;

                case UART_BUFFER_FULL:
                    // Something went wrong - don't try to catch up,
                    // just discard all pending data and start over
                    ESP_LOGW(TAG, "UART ring buffer full!");
                    uart_flush_input(this->uart_num);
                    xQueueReset(this->uart_event_queue);
                    break;

                case UART_FIFO_OVF:
                    // Something went wrong - don't try to catch up,
                    // just discard all pending data and start over
                    ESP_LOGW(TAG, "UART FIFO Overflow!");
                    uart_flush_input(this->uart_num);
                    xQueueReset(this->uart_event_queue);
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGW(TAG, "UART parity error");
                    break;

                case UART_FRAME_ERR:
                    ESP_LOGW(TAG, "UART frame error");
                    break;

                default:
                    ESP_LOGW(TAG, "Unhandled UART event type: %d", event.type);
                    break;
            }
        }   
    }
}

void Controller::uart_read_bytes(uint8_t *buf, size_t length) {
    if (this->callbacks.ReadBytes)
        callbacks.ReadBytes(buf, length);
    else
        ::uart_read_bytes(this->uart_num, buf, length, portMAX_DELAY);
}

void Controller::uart_write_bytes(const uint8_t *buf, size_t length) {
    if (this->callbacks.WriteBytes)
        callbacks.WriteBytes(buf, length);
    else
        ::uart_write_bytes(this->uart_num, buf, length);
}

void Controller::set_initialization_stage(const InitializationStageEnum stage) {
    this->initialization_stage = stage;

    // This callback is not deferred; may need to redesign if it causes a delay beyond the transmit window
    if (this->callbacks.InitializationStage)
        callbacks.InitializationStage(stage);
}

void Controller::process_packet(const Packet::Buffer& buffer, bool lastPacketOnWire) {
    bool error_flag_changed = false;
    std::function<void()> deferred_callback;

    // Parse buffer
    Packet packet(buffer);

    // Save token destination
    if (this->initialization_stage == InitializationStageEnum::FindNextControllerRx) {
        // Controller with address > configured did not transmit
        if (packet.SourceType != AddressTypeEnum::Controller)
            this->next_token_destination_type = AddressTypeEnum::IndoorUnit;

        // Fujitsu RC1 checks for next controller twice (in case of slow booting controller?), but we are only checking once
        this->set_initialization_stage(this->features.Zones ? InitializationStageEnum::ZoneRequestActive : InitializationStageEnum::Complete);
    }

    // Process packets from Indoor Units
    if (packet.SourceType == AddressTypeEnum::IndoorUnit) {
        switch (packet.Type) {
            [[likely]] case PacketTypeEnum::Config:
                if (this->initialization_stage == InitializationStageEnum::DetectFeatureSupport) {
                    if (packet.Config.IndoorUnit.UnknownFlags == 2) { // Guessing this means no feature support among other things
                        this->features = DefaultFeatures;
                        this->set_initialization_stage(InitializationStageEnum::FindNextControllerTx);
                    } else
                        this->set_initialization_stage(InitializationStageEnum::FeatureRequest);
                }

                if (this->last_error_flag != packet.Config.IndoorUnit.Error)
                    error_flag_changed = true;

                this->last_error_flag = packet.Config.IndoorUnit.Error;
                this->current_configuration = packet.Config;

                // Include the state of these fields not returned from the Indoor Unit in the callback data
                this->current_configuration.Controller.Temperature = this->changed_configuration.Controller.Temperature;
                this->current_configuration.Controller.UseControllerSensor = this->changed_configuration.Controller.UseControllerSensor;

                if (this->callbacks.Config)
                    deferred_callback = [this](){ this->callbacks.Config(this->current_configuration); };
                break;

            case PacketTypeEnum::Error:
                if (this->callbacks.Error)
                    deferred_callback = [&](){ this->callbacks.Error(packet); };
                break;

            case PacketTypeEnum::Features:
                this->features = packet.Features;
                this->set_initialization_stage(this->features.Zones ? InitializationStageEnum::ZoneRequestEnabled : InitializationStageEnum::FindNextControllerTx);
                break;

            case PacketTypeEnum::Function:
                if (this->callbacks.Function)
                    deferred_callback = [&](){ this->callbacks.Function(packet.Function); };
                break;

            case PacketTypeEnum::Status:
                break;

            case PacketTypeEnum::ZoneConfig:
                this->current_zone_configuration = packet.ZoneConfig;

                if (this->initialization_stage == InitializationStageEnum::ZoneRequestActive)
                    this->set_initialization_stage(InitializationStageEnum::Complete);

                if (this->callbacks.ZoneConfig)
                    deferred_callback = [&](){ this->callbacks.ZoneConfig(this->current_zone_configuration); };
                break;

            case PacketTypeEnum::ZoneFunction:
                this->zones = packet.ZoneFunction.IndoorUnit;

                if (this->initialization_stage == InitializationStageEnum::ZoneRequestEnabled)
                    this->set_initialization_stage(InitializationStageEnum::FindNextControllerTx);
                break;
        }
    } else {
        switch (packet.Type) {
            // Config packet from another controller
            [[likely]] case PacketTypeEnum::Config:
                if (this->callbacks.ControllerConfig)
                    deferred_callback = [&](){ this->callbacks.ControllerConfig(packet.SourceAddress, packet.Config); };
                break;
            default:
                break;
        }
    }

    // Emit a packet if given the token and not processing old packets
    if (lastPacketOnWire && packet.TokenDestinationType == AddressTypeEnum::Controller && packet.TokenDestinationAddress == this->controller_address) {
        Packet tx_packet;
        tx_packet.SourceType = AddressTypeEnum::Controller;
        tx_packet.SourceAddress = this->controller_address;

        if (this->initialization_stage == InitializationStageEnum::FindNextControllerTx) {
            this->next_token_destination_type = AddressTypeEnum::Controller;
            this->set_initialization_stage(InitializationStageEnum::FindNextControllerRx);
        }

        tx_packet.TokenDestinationType = this->next_token_destination_type;
        tx_packet.TokenDestinationAddress = this->next_token_destination_type == AddressTypeEnum::Controller ? this->controller_address + 1 : 1;

        if (this->initialization_stage == InitializationStageEnum::FeatureRequest)
            tx_packet.Type = PacketTypeEnum::Features;
        else if (this->initialization_stage == InitializationStageEnum::ZoneRequestEnabled)
            tx_packet.Type = PacketTypeEnum::ZoneFunction;
        else if (this->initialization_stage == InitializationStageEnum::ZoneRequestActive)
            tx_packet.Type = PacketTypeEnum::ZoneConfig;
        else if ((error_flag_changed && this->is_primary_controller()) ||
                 (packet.Type == PacketTypeEnum::Error && !this->is_primary_controller()))
            tx_packet.Type = PacketTypeEnum::Error;
        else if (this->zone_configuration_changes.any()) {
            tx_packet.Type = PacketTypeEnum::ZoneConfig;
            tx_packet.ZoneConfig = this->current_zone_configuration;
            tx_packet.ZoneConfig.Controller.Write = true;

            for (size_t i = ZoneSettableFields::Zone1Active; i <= ZoneSettableFields::Zone8Active; i++)
                if (this->zone_configuration_changes[i])
                    tx_packet.ZoneConfig.ActiveZones[i] = this->changed_zone_configuration.ActiveZones[i];

            if (this->zone_configuration_changes[ZoneSettableFields::ZoneGroupDayActive])
                tx_packet.ZoneConfig.ActiveZoneGroups.Day = this->changed_zone_configuration.ActiveZoneGroups.Day;

            if (this->zone_configuration_changes[ZoneSettableFields::ZoneGroupNightActive])
                tx_packet.ZoneConfig.ActiveZoneGroups.Night = this->changed_zone_configuration.ActiveZoneGroups.Night;

            this->zone_configuration_changes.reset();
        } else if (!this->function_queue.empty()) {
            tx_packet.Type = PacketTypeEnum::Function;
            tx_packet.Function = this->function_queue.front();
            this->function_queue.pop();
        } else {
            // First CONFIG packet sent from Fujitsu controller has write flag set, but we do not restore state at this time
            tx_packet.Type = PacketTypeEnum::Config;
            tx_packet.Config = this->current_configuration;
            tx_packet.Config.Controller.Temperature = this->changed_configuration.Controller.Temperature;
            tx_packet.Config.Controller.UseControllerSensor = this->changed_configuration.Controller.UseControllerSensor;

            if (this->configuration_changes.any()) {
                tx_packet.Config.Controller.Write = true;

                // Overwrite fields received from Indoor Unit
                if (this->configuration_changes[SettableFields::Enabled])
                    tx_packet.Config.Enabled = this->changed_configuration.Enabled;

                if (this->configuration_changes[SettableFields::Economy])
                    tx_packet.Config.Economy = this->changed_configuration.Economy;

                if (this->configuration_changes[SettableFields::Setpoint])
                    tx_packet.Config.Setpoint = this->changed_configuration.Setpoint;

                if (this->configuration_changes[SettableFields::TestRun])
                    tx_packet.Config.TestRun = this->changed_configuration.TestRun;

                if (this->configuration_changes[SettableFields::Mode])
                    tx_packet.Config.Mode = this->changed_configuration.Mode;

                if (this->configuration_changes[SettableFields::FanSpeed])
                    tx_packet.Config.FanSpeed = this->changed_configuration.FanSpeed;

                if (this->configuration_changes[SettableFields::SwingVertical])
                    tx_packet.Config.SwingVertical = this->changed_configuration.SwingVertical;

                if (this->configuration_changes[SettableFields::SwingHorizontal])
                    tx_packet.Config.SwingHorizontal = this->changed_configuration.SwingHorizontal;

                // Set fields not returned from Indoor Unit
                if (this->configuration_changes[SettableFields::AdvanceVerticalLouver])
                    tx_packet.Config.Controller.AdvanceVerticalLouver = this->changed_configuration.Controller.AdvanceVerticalLouver;

                if (this->configuration_changes[SettableFields::AdvanceHorizontalLouver])
                    tx_packet.Config.Controller.AdvanceHorizontalLouver = this->changed_configuration.Controller.AdvanceHorizontalLouver;

                if (this->configuration_changes[SettableFields::ResetFilterTimer])
                    tx_packet.Config.Controller.ResetFilterTimer = this->changed_configuration.Controller.ResetFilterTimer;

                if (this->configuration_changes[SettableFields::Maintenance])
                    tx_packet.Config.Controller.Maintenance = this->changed_configuration.Controller.Maintenance;
            }

            this->configuration_changes.reset();

            // Some fields need to be written clear in next tx packet
            if (tx_packet.Config.Controller.ResetFilterTimer) {
                this->changed_configuration.Controller.ResetFilterTimer = false;
                this->configuration_changes[SettableFields::ResetFilterTimer] = true;
            }

            if (tx_packet.Config.Controller.Maintenance) {
                this->changed_configuration.Controller.Maintenance = false;
                this->configuration_changes[SettableFields::Maintenance] = true;
            }
        }

        Packet::Buffer b = tx_packet.to_buffer();
        // TODO Should check have not missed tx window before tx...
        // Need to use RX_TIMEOUT interrupt to get accurate rx timestamp?
        // Can drop lastPacketOnWire check if implemented
        this->uart_write_bytes(b.data(), b.size());
    }

    // Have now (hopefully) transmitted on time so call pending callback
    if (deferred_callback) {
        deferred_callback();
    }
}

void Controller::set_current_temperature(float temperature) {
    this->changed_configuration.Controller.Temperature = std::clamp(std::isfinite(temperature) ? temperature : 0, MinTemperature, MaxTemperature);
    // Do not set configuration_changed flag - does not require write bit set
}

bool Controller::set_enabled(bool enabled, bool ignore_lock) {
    if (!ignore_lock && (this->current_configuration.IndoorUnit.Lock.All || this->current_configuration.IndoorUnit.Lock.Enabled))
        return false;

    this->changed_configuration.Enabled = enabled;
    this->configuration_changes[SettableFields::Enabled] = true;
    return true;
}

bool Controller::set_economy(bool economy, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    this->changed_configuration.Economy = economy;
    this->configuration_changes[SettableFields::Economy] = true;
    return true;
}

bool Controller::set_test_run(bool test_run, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    this->changed_configuration.TestRun = test_run;
    this->configuration_changes[SettableFields::TestRun] = true;
    return true;
}

bool Controller::set_setpoint(uint8_t temperature, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (temperature < MinSetpoint || temperature > MaxSetpoint)
        return false;

    this->changed_configuration.Setpoint = temperature;
    this->configuration_changes[SettableFields::Setpoint] = true;
    return true;
}

bool Controller::set_mode(ModeEnum mode, bool ignore_lock) {
    if (!ignore_lock && (this->current_configuration.IndoorUnit.Lock.All || this->current_configuration.IndoorUnit.Lock.Mode))
        return false;

    switch (mode) {
        case ModeEnum::Fan:
            if (!this->features.Mode.Fan)
                return false;
            break;

        case ModeEnum::Dry:
            if (!this->features.Mode.Dry)
                return false;
            break;

        case ModeEnum::Cool:
            if (!this->features.Mode.Cool)
                return false;
            break;

        case ModeEnum::Heat:
            if (!this->features.Mode.Heat)
                return false;
            break;

        case ModeEnum::Auto:
            if (!this->features.Mode.Auto)
                return false;
            break;
    }

    this->changed_configuration.Mode = mode;
    this->configuration_changes[SettableFields::Mode] = true;
    return true;
}

bool Controller::set_fan_speed(FanSpeedEnum fan_speed, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    switch (fan_speed) {
        case FanSpeedEnum::Auto:
            if (!this->features.FanSpeed.Auto)
                return false;
            break;

        case FanSpeedEnum::Quiet:
            if (!this->features.FanSpeed.Quiet)
                return false;
            break;

        case FanSpeedEnum::Low:
            if (!this->features.FanSpeed.Low)
                return false;
            break;

        case FanSpeedEnum::Medium:
            if (!this->features.FanSpeed.Medium)
                return false;
            break;

        case FanSpeedEnum::High:
            if (!this->features.FanSpeed.High)
                return false;
            break;
    }

    this->changed_configuration.FanSpeed = fan_speed;
    this->configuration_changes[SettableFields::FanSpeed] = true;
    return true;
}

bool Controller::set_vertical_swing(bool swing_vertical, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.VerticalLouvers)
        return false;

    this->changed_configuration.SwingVertical = swing_vertical;
    this->configuration_changes[SettableFields::SwingVertical] = true;
    return true;
}

bool Controller::set_horizontal_swing(bool swing_horizontal, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.HorizontalLouvers)
        return false;

    this->changed_configuration.SwingHorizontal = swing_horizontal;
    this->configuration_changes[SettableFields::SwingHorizontal] = true;
    return true;
}

bool Controller::advance_vertical_louver(bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.VerticalLouvers)
        return false;

    this->changed_configuration.Controller.AdvanceVerticalLouver = true;
    this->configuration_changes[SettableFields::AdvanceVerticalLouver] = true;
    return true;
}

bool Controller::advance_horizontal_louver(bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.HorizontalLouvers)
        return false;

    this->changed_configuration.Controller.AdvanceHorizontalLouver = true;
    this->configuration_changes[SettableFields::AdvanceHorizontalLouver] = true;
    return true;
}

bool Controller::use_sensor(bool use_sensor, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.SensorSwitching)
        return false;

    this->changed_configuration.Controller.UseControllerSensor = use_sensor;
    // Do not set configuration_changed flag - does not require write bit set
    return true;
}

bool Controller::reset_filter(bool ignore_lock) {
    if (!ignore_lock && (this->current_configuration.IndoorUnit.Lock.All || this->current_configuration.IndoorUnit.Lock.ResetFilterTimer))
        return false;

    if (!this->features.FilterTimer)
        return false;

    this->changed_configuration.Controller.ResetFilterTimer = true;
    this->configuration_changes[SettableFields::ResetFilterTimer] = true;
    return true;
}

bool Controller::maintenance(bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->features.Maintenance)
        return false;

    this->changed_configuration.Controller.Maintenance = true;
    this->configuration_changes[SettableFields::Maintenance] = true;
    return true;
}

bool Controller::set_zone(uint8_t zone, bool active, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    if (!this->zones.EnabledZones.test(zone))
        return false;

    // Ensure at least one outlet is open
    if (!active && !this->zones.ZoneCommon &&
        this->changed_zone_configuration.ActiveZones.count() == 1 &&
        this->changed_zone_configuration.ActiveZones.test(zone))
        return false;

    // Invalidate active zone groups
    this->set_zone_group_day(false, true);
    this->set_zone_group_night(false, true);

    this->changed_zone_configuration.ActiveZones[zone] = active;
    this->zone_configuration_changes[zone] = true;
    return true;
}

bool Controller::set_zone_group_day(bool active, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    this->changed_zone_configuration.ActiveZoneGroups.Day = active;
    this->zone_configuration_changes[ZoneSettableFields::ZoneGroupDayActive] = true;
    return true;
}

bool Controller::set_zone_group_night(bool active, bool ignore_lock) {
    if (!ignore_lock && this->current_configuration.IndoorUnit.Lock.All)
        return false;

    this->changed_zone_configuration.ActiveZoneGroups.Night = active;
    this->zone_configuration_changes[ZoneSettableFields::ZoneGroupNightActive] = true;
    return true;
}

}
