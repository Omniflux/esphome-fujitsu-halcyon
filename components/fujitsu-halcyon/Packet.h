#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <limits>

namespace fujitsu_halcyon_controller {

constexpr uint8_t PrimaryControllerAddress = 0;

enum class AddressTypeEnum : uint8_t {
    IndoorUnit,
    Controller
};

enum class PacketTypeEnum : uint8_t {
    Config,
    Error,
    Features,
    Function,
    Status
};

enum class FanSpeedEnum : uint8_t {
    Auto,
    Quiet,
    Low,
    Medium,
    High
};

enum class ModeEnum : uint8_t {
    Fan = 1,
    Dry,
    Cool,
    Heat,
    Auto
};

struct Config {
    struct {
        struct {
            bool ResetFilterTimer;
            bool Enabled;
            bool Mode;
            bool Timer;
            bool All;
        } Lock;

        struct {
            bool Secondary;
            bool Primary;
        } SeenController;

        bool StandbyMode;
        bool Error;
        bool FilterTimerExpired;
    } IndoorUnit;

    struct {
        uint8_t Temperature;
        bool Write;
        bool UseControllerSensor;
        bool AdvanceHorizontalLouver;
        bool AdvanceVerticalLouver;
        bool ResetFilterTimer;
        bool Maintenance;
    } Controller;

    FanSpeedEnum FanSpeed;
    ModeEnum Mode;
    uint8_t Setpoint;
    bool Enabled;
    bool Economy;
    bool TestRun;
    bool SwingHorizontal;
    bool SwingVertical;
};

struct Error {
    uint8_t ErrorCode;
};

struct Features {
    struct { 
        bool Auto;
        bool Heat;
        bool Fan;
        bool Dry;
        bool Cool;
    } Mode;

    struct { 
        bool Quiet;
        bool Low;
        bool Medium;
        bool High;
        bool Auto;
    } FanSpeed;

    bool FilterTimer;
    bool SensorSwitching;
    bool Maintenance;
    bool EconomyMode;
    bool HorizontalLouvers;
    bool VerticalLouvers;
};

struct Function {
    struct {
        bool Write;
    } Controller;

    uint8_t Function;
    uint8_t Value;
    uint8_t Unit;
};

struct Status {};

struct ByteMaskShiftData {
    constexpr ByteMaskShiftData(uint8_t byte, uint8_t mask) : byte(byte), mask(mask), shift(std::countr_zero(mask)) {};

    const uint8_t byte;
    const uint8_t mask;
    const uint8_t shift;
};

constexpr struct BMS {
    constexpr static auto SourceType                  = ByteMaskShiftData(0, 0b00100000);
    constexpr static auto SourceAddress               = ByteMaskShiftData(0, 0b00001111);

    constexpr static auto TokenDestinationType        = ByteMaskShiftData(1, 0b00100000);
    constexpr static auto TokenDestinationAddress     = ByteMaskShiftData(1, 0b00001111);

    constexpr static auto Type                        = ByteMaskShiftData(2, 0b01110000);

    constexpr static struct Config_ {
        constexpr static struct IndoorUnit_ {
            constexpr static struct Lock_ {
                constexpr static auto ResetFilterTimer      = ByteMaskShiftData(6, 0b10000000);
                constexpr static auto Enabled               = ByteMaskShiftData(6, 0b01000000);
                constexpr static auto Mode                  = ByteMaskShiftData(6, 0b00100000);
                constexpr static auto Timer                 = ByteMaskShiftData(6, 0b00001000);
                constexpr static auto All                   = ByteMaskShiftData(6, 0b00000100);
            } Lock = {};

            constexpr static struct SeenController_ {
                constexpr static auto Secondary             = ByteMaskShiftData(6, 0b00000010);
                constexpr static auto Primary               = ByteMaskShiftData(6, 0b00000001);
            } SeenController = {};

            constexpr static auto StandbyMode             = ByteMaskShiftData(2, 0b00001000);
            constexpr static auto Error                   = ByteMaskShiftData(3, 0b10000000);
            constexpr static auto FilterTimerExpired      = ByteMaskShiftData(7, 0b01000000);
        } IndoorUnit = {};

        constexpr static struct Controller_ {
            constexpr static auto Write                   = ByteMaskShiftData(2, 0b00001000);
            constexpr static auto UseControllerSensor     = ByteMaskShiftData(5, 0b10000000);
            constexpr static auto AdvanceHorizontalLouver = ByteMaskShiftData(5, 0b00001000);
            constexpr static auto AdvanceVerticalLouver   = ByteMaskShiftData(5, 0b00000010);
            constexpr static auto Temperature             = ByteMaskShiftData(6, 0b01111110);
            constexpr static auto ResetFilterTimer        = ByteMaskShiftData(7, 0b01000000);
            constexpr static auto Maintenance             = ByteMaskShiftData(7, 0b00100000);
        } Controller = {};

        constexpr static auto FanSpeed                  = ByteMaskShiftData(3, 0b01110000);
        constexpr static auto Mode                      = ByteMaskShiftData(3, 0b00001110);
        constexpr static auto Enabled                   = ByteMaskShiftData(3, 0b00000001);
        constexpr static auto Economy                   = ByteMaskShiftData(4, 0b10000000);
        constexpr static auto TestRun                   = ByteMaskShiftData(4, 0b01000000);
        constexpr static auto Setpoint                  = ByteMaskShiftData(4, 0b00011111);
        constexpr static auto SwingHorizontal           = ByteMaskShiftData(5, 0b00010000);
        constexpr static auto SwingVertical             = ByteMaskShiftData(5, 0b00000100);
    } Config {};

    constexpr static struct Error_ {
        constexpr static auto ErrorCode                 = ByteMaskShiftData(4, 0b11111111);
    } Error {};

    constexpr static struct Features_ {
        constexpr static struct Mode_{ 
            constexpr static auto Auto                    = ByteMaskShiftData(3, 0b00010000);
            constexpr static auto Heat                    = ByteMaskShiftData(3, 0b00001000);
            constexpr static auto Fan                     = ByteMaskShiftData(3, 0b00000100);
            constexpr static auto Dry                     = ByteMaskShiftData(3, 0b00000010);
            constexpr static auto Cool                    = ByteMaskShiftData(3, 0b00000001);
        } Mode {};

        constexpr static struct FanSpeed_ { 
            constexpr static auto Quiet                   = ByteMaskShiftData(4, 0b00010000);
            constexpr static auto Low                     = ByteMaskShiftData(4, 0b00001000);
            constexpr static auto Medium                  = ByteMaskShiftData(4, 0b00000100);
            constexpr static auto High                    = ByteMaskShiftData(4, 0b00000010);
            constexpr static auto Auto                    = ByteMaskShiftData(4, 0b00000001);
        } FanSpeed {};

        constexpr static auto FilterTimer               = ByteMaskShiftData(5, 0b10000000);
        constexpr static auto SensorSwitching           = ByteMaskShiftData(5, 0b01000000);
        constexpr static auto Maintenance               = ByteMaskShiftData(5, 0b00001000);
        constexpr static auto EconomyMode               = ByteMaskShiftData(5, 0b00000100);
        constexpr static auto HorizontalLouvers         = ByteMaskShiftData(5, 0b00000010);
        constexpr static auto VerticalLouvers           = ByteMaskShiftData(5, 0b00000001);
    } Features {};

    constexpr static struct Function_ {
        constexpr static struct Controller_ {
            constexpr static auto Write                   = ByteMaskShiftData(2, 0b00001000);
        } Controller {};

        constexpr static auto Function                  = ByteMaskShiftData(4, 0b11111111);
        constexpr static auto Value                     = ByteMaskShiftData(5, 0b11111111);
        constexpr static auto Unit                      = ByteMaskShiftData(7, 0b00001111);
    } Function {};

    constexpr static struct Status_ {} Status {};
} BMS;
static_assert(BMS.Type.shift == 4 && BMS.Features.FanSpeed.Low.shift == 3, "Shift values calculated incorrectly");

class Packet {
    public:
        static constexpr uint8_t FrameSize = 8;
        using Buffer = std::array<uint8_t, FrameSize>;

        Packet() : SourceType {}, SourceAddress {}, TokenDestinationType {}, TokenDestinationAddress {}, Type {} {};
        Packet(Buffer buffer);
        Buffer to_buffer() const;

        AddressTypeEnum SourceType;
        uint8_t         SourceAddress;

        AddressTypeEnum TokenDestinationType;
        uint8_t         TokenDestinationAddress;

        PacketTypeEnum Type;

        struct Config Config {};
        struct Error Error {};
        struct Function Function {};
        struct Features Features {};
        struct Status Status {};

        static void invert_buffer(Buffer& buffer) { *reinterpret_cast<uint64_t*>(buffer.data()) = ~*reinterpret_cast<uint64_t*>(buffer.data()); };
};

}