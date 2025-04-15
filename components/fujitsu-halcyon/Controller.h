#pragma once

#include <bitset>
#include <functional>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/uart.h>

#include "Packet.h"

namespace fujitsu_halcyon_controller {

constexpr uart_config_t UARTConfig = {
        .baud_rate = 500,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_APB,
};

constexpr uint8_t UARTInterPacketSymbolSpacing = 2;

// Temperatures are in Celcius
constexpr uint8_t MinSetpoint = 16;
constexpr uint8_t MaxSetpoint = 30;
constexpr float MinTemperature = 0.0;
constexpr float MaxTemperature = 60.0;

enum class InitializationStageEnum : uint8_t {
    FeatureRequest,
    ZoneRequestEnabled,
    FindNextControllerTx,
    FindNextControllerRx,
    ZoneRequestActive,
    Complete
};

namespace SettableFields {
    enum {
        Temperature,
        Enabled,
        Economy,
        TestRun,
        Setpoint,
        Mode,
        FanSpeed,
        SwingVertical,
        SwingHorizontal,
        AdvanceVerticalLouver,
        AdvanceHorizontalLouver,
        UseControllerSensor,
        ResetFilterTimer,
        Maintenance,
        MAX
    };
};

namespace ZoneSettableFields {
    enum {
        Zone1Active,
        Zone2Active,
        Zone3Active,
        Zone4Active,
        Zone5Active,
        Zone6Active,
        Zone7Active,
        Zone8Active,
        ZoneGroupDayActive,
        ZoneGroupNightActive,
        MAX
    };
};

class Controller {
    using ConfigCallback = std::function<void(const Config&)>;
    using ErrorCallback  = std::function<void(const Packet&)>;
    using ZoneConfigCallback = std::function<void(const ZoneConfig&)>;
    using ControllerConfigCallback = std::function<void(const uint8_t address, const Config&)>;
    using ReadBytesCallback  = std::function<void(uint8_t *data, size_t len)>;
    using WriteBytesCallback = std::function<void(const uint8_t *data, size_t len)>;

    struct Callbacks {
        ConfigCallback Config;
        ErrorCallback Error;
        ZoneConfigCallback ZoneConfig;
        ControllerConfigCallback ControllerConfig;
        ReadBytesCallback ReadBytes;
        WriteBytesCallback WriteBytes;
    };

    public:
        Controller(uint8_t uart_num, uint8_t controller_address, const Callbacks& callbacks, QueueHandle_t uart_event_queue = nullptr)
            : uart_num(uart_num), controller_address(controller_address), uart_event_queue(uart_event_queue), callbacks(callbacks) {}

        bool start();
        bool is_initialized() const { return this->initialization_stage == InitializationStageEnum::Complete; }
        void reinitialize() { this->initialization_stage = InitializationStageEnum::FeatureRequest; }
        const struct Features& get_features() const { return this->features; }
        const decltype(ZoneFunction::IndoorUnit) get_zones() const { return this->zones; }

        void set_current_temperature(float temperature);
        bool set_enabled(bool enabled, bool ignore_lock = false);
        bool set_economy(bool economy, bool ignore_lock = false);
        bool set_test_run(bool test_run, bool ignore_lock = false);
        bool set_setpoint(uint8_t temperature, bool ignore_lock = false);
        bool set_mode(ModeEnum mode, bool ignore_lock = false);
        bool set_fan_speed(FanSpeedEnum fan_speed, bool ignore_lock = false);
        bool set_vertical_swing(bool swing_vertical, bool ignore_lock = false);
        bool set_horizontal_swing(bool swing_horizontal, bool ignore_lock = false);
        bool advance_vertical_louver(bool ignore_lock = false);
        bool advance_horizontal_louver(bool ignore_lock = false);
        bool use_sensor(bool use_sensor, bool ignore_lock = false);
        bool reset_filter(bool ignore_lock = false);
        bool maintenance(bool ignore_lock = false);

        bool set_zone(uint8_t zone, bool active, bool ignore_lock = false);
        bool set_zone_group_day(bool active, bool ignore_lock = false);
        bool set_zone_group_night(bool active, bool ignore_lock = false);

    protected:
        InitializationStageEnum initialization_stage = InitializationStageEnum::FeatureRequest;
        AddressTypeEnum next_token_destination_type = AddressTypeEnum::IndoorUnit;

        bool is_primary_controller() const { return this->controller_address == PrimaryControllerAddress; }
        void process_packet(const Packet::Buffer& buffer, bool lastPacketOnWire = true);

    private:
        uint8_t uart_num;
        uint8_t controller_address;
        QueueHandle_t uart_event_queue;
        Callbacks callbacks;

        struct Features features = {};
        struct Config current_configuration = {};
        struct Config changed_configuration = {};
        struct ZoneConfig current_zone_configuration = {};
        struct ZoneConfig changed_zone_configuration = {};
        decltype(ZoneFunction::IndoorUnit) zones = {};

        std::bitset<SettableFields::MAX> configuration_changes;
        std::bitset<ZoneSettableFields::MAX> zone_configuration_changes;

        bool last_error_flag = false; // TODO handle errors for multiple indoor units...multiple errors per IU?

        [[noreturn]] void uart_event_task();
        void uart_read_bytes(uint8_t *buf, size_t length);
        void uart_write_bytes(const uint8_t *buf, size_t length);
};

}
