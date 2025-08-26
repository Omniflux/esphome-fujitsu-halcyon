#include <cmath>

#include "Packet.h"

namespace fujitsu_general::airstage::h {

Packet::Packet(Buffer buffer) {
    auto getField = [&buffer](const ByteMaskShiftData& bms) -> uint8_t {
        return (buffer[bms.byte] & bms.mask) >> bms.shift;
    };

    this->invert_buffer(buffer);

    this->SourceType = static_cast<AddressTypeEnum>(getField(BMS.SourceType));
    this->SourceAddress = getField(BMS.SourceAddress);

    this->TokenDestinationType = static_cast<AddressTypeEnum>(getField(BMS.TokenDestinationType));
    this->TokenDestinationAddress = getField(BMS.TokenDestinationAddress);

    this->Type = static_cast<PacketTypeEnum>(getField(BMS.Type));

    switch (this->Type) {
        case PacketTypeEnum::Config:
            if (this->SourceType == AddressTypeEnum::IndoorUnit) {
                this->Config.IndoorUnit.StandbyMode = getField(BMS.Config.IndoorUnit.StandbyMode);
                this->Config.IndoorUnit.Error = getField(BMS.Config.IndoorUnit.Error);

                this->Config.IndoorUnit.SeenController.Primary = getField(BMS.Config.IndoorUnit.SeenController.Primary);
                this->Config.IndoorUnit.SeenController.Secondary = getField(BMS.Config.IndoorUnit.SeenController.Secondary);

                this->Config.IndoorUnit.Lock.All = getField(BMS.Config.IndoorUnit.Lock.All);
                this->Config.IndoorUnit.Lock.Timer = getField(BMS.Config.IndoorUnit.Lock.Timer);
                this->Config.IndoorUnit.Lock.Mode = getField(BMS.Config.IndoorUnit.Lock.Mode);
                this->Config.IndoorUnit.Lock.Enabled = getField(BMS.Config.IndoorUnit.Lock.Enabled);
                this->Config.IndoorUnit.Lock.ResetFilterTimer = getField(BMS.Config.IndoorUnit.Lock.ResetFilterTimer);

                this->Config.IndoorUnit.FilterTimerExpired = getField(BMS.Config.IndoorUnit.FilterTimerExpired);

                this->Config.IndoorUnit.UnknownFlags = getField(BMS.Config.IndoorUnit.UnknownFlags);
            } else {
                this->Config.Controller.Write = getField(BMS.Config.Controller.Write);

                this->Config.Controller.AdvanceVerticalLouver = getField(BMS.Config.Controller.AdvanceVerticalLouver);
                this->Config.Controller.AdvanceHorizontalLouver = getField(BMS.Config.Controller.AdvanceHorizontalLouver);

                auto temperature = getField(BMS.Config.Controller.Temperature);
                this->Config.Controller.Temperature = (temperature >> 1) + (temperature & 1) / 2.0;
                this->Config.Controller.UseControllerSensor = getField(BMS.Config.Controller.UseControllerSensor);

                this->Config.Controller.Maintenance = getField(BMS.Config.Controller.Maintenance);
                this->Config.Controller.ResetFilterTimer = getField(BMS.Config.Controller.ResetFilterTimer);
            }

            this->Config.Mode = static_cast<ModeEnum>(getField(BMS.Config.Mode));
            this->Config.FanSpeed = static_cast<FanSpeedEnum>(getField(BMS.Config.FanSpeed));

            this->Config.Enabled = getField(BMS.Config.Enabled);
            this->Config.Economy = getField(BMS.Config.Economy);
            this->Config.Setpoint = getField(BMS.Config.Setpoint);
            this->Config.TestRun = getField(BMS.Config.TestRun);

            this->Config.SwingVertical = getField(BMS.Config.SwingVertical);
            this->Config.SwingHorizontal = getField(BMS.Config.SwingHorizontal);
            break;

        case PacketTypeEnum::Error:
            this->Error.ErrorCode = getField(BMS.Error.ErrorCode);
            break;

        case PacketTypeEnum::Features:
            if (this->SourceType == AddressTypeEnum::IndoorUnit) {
                this->Features.Mode.Cool = getField(BMS.Features.Mode.Cool);
                this->Features.Mode.Dry = getField(BMS.Features.Mode.Dry);
                this->Features.Mode.Fan = getField(BMS.Features.Mode.Fan);
                this->Features.Mode.Heat = getField(BMS.Features.Mode.Heat);
                this->Features.Mode.Auto = getField(BMS.Features.Mode.Auto);

                this->Features.FanSpeed.Auto = getField(BMS.Features.FanSpeed.Auto);
                this->Features.FanSpeed.High = getField(BMS.Features.FanSpeed.High);
                this->Features.FanSpeed.Medium = getField(BMS.Features.FanSpeed.Medium);
                this->Features.FanSpeed.Low = getField(BMS.Features.FanSpeed.Low);
                this->Features.FanSpeed.Quiet = getField(BMS.Features.FanSpeed.Quiet);

                this->Features.EconomyMode = getField(BMS.Features.EconomyMode);
                this->Features.FilterTimer = getField(BMS.Features.FilterTimer);
                this->Features.Maintenance = getField(BMS.Features.Maintenance);
                this->Features.SensorSwitching = getField(BMS.Features.SensorSwitching);

                this->Features.VerticalLouvers = getField(BMS.Features.VerticalLouvers);
                this->Features.HorizontalLouvers = getField(BMS.Features.HorizontalLouvers);
            }
            break;

        case PacketTypeEnum::Function:
            if (this->SourceType == AddressTypeEnum::Controller)
                this->Function.Controller.Write = getField(BMS.Function.Controller.Write);

            this->Function.Function = getField(BMS.Function.Function);
            this->Function.Value = getField(BMS.Function.Value);
            this->Function.Unit = getField(BMS.Function.Unit);
            break;

        case PacketTypeEnum::Status:
            break;
    }
};

Packet::Buffer Packet::to_buffer() const {
    Buffer buffer {};

    auto setField = [&buffer](const ByteMaskShiftData& bms, const auto value) {
        buffer[bms.byte] |= (value << bms.shift) & bms.mask;
    };

    setField(BMS.SourceType, static_cast<uint8_t>(this->SourceType));
    setField(BMS.SourceAddress, this->SourceAddress);

    setField(BMS.TokenDestinationType, static_cast<uint8_t>(this->TokenDestinationType));
    setField(BMS.TokenDestinationAddress, this->TokenDestinationAddress);

    buffer[1] |= 0b10000000; // Unknown bit set in all captured packets

    setField(BMS.Type, static_cast<uint8_t>(this->Type));

    switch (this->Type) {
        case PacketTypeEnum::Config:
            if (this->SourceType == AddressTypeEnum::IndoorUnit) {
                setField(BMS.Config.IndoorUnit.StandbyMode, this->Config.IndoorUnit.StandbyMode);
                setField(BMS.Config.IndoorUnit.Error, this->Config.IndoorUnit.Error);

                setField(BMS.Config.IndoorUnit.SeenController.Primary, this->Config.IndoorUnit.SeenController.Primary);
                setField(BMS.Config.IndoorUnit.SeenController.Secondary, this->Config.IndoorUnit.SeenController.Secondary);

                setField(BMS.Config.IndoorUnit.Lock.All, this->Config.IndoorUnit.Lock.All);
                setField(BMS.Config.IndoorUnit.Lock.Timer, this->Config.IndoorUnit.Lock.Timer);
                setField(BMS.Config.IndoorUnit.Lock.Mode, this->Config.IndoorUnit.Lock.Mode);
                setField(BMS.Config.IndoorUnit.Lock.Enabled, this->Config.IndoorUnit.Lock.Enabled);
                setField(BMS.Config.IndoorUnit.Lock.ResetFilterTimer, this->Config.IndoorUnit.Lock.ResetFilterTimer);

                setField(BMS.Config.IndoorUnit.FilterTimerExpired, this->Config.IndoorUnit.FilterTimerExpired);

                // setField(BMS.Config.IndoorUnit.UnknownFlags, this->Config.IndoorUnit.UnknownFlags);
                buffer[5] |= 0b10100000; // Unknown bits set in all captured config packets from indoor unit
                buffer[7] |= 0b00100000; // Unknown bit set in all captured config packets from indoor unit
            } else {
                setField(BMS.Config.Controller.Write, this->Config.Controller.Write);

                setField(BMS.Config.Controller.AdvanceVerticalLouver, this->Config.Controller.AdvanceVerticalLouver);
                setField(BMS.Config.Controller.AdvanceHorizontalLouver, this->Config.Controller.AdvanceHorizontalLouver);

                setField(BMS.Config.Controller.Temperature,
                    (int(this->Config.Controller.Temperature) << 1) +
                    int(std::fmod(std::round(this->Config.Controller.Temperature * 2), 2))
                );

                setField(BMS.Config.Controller.UseControllerSensor, this->Config.Controller.UseControllerSensor);
                setField(BMS.Config.Controller.Maintenance, this->Config.Controller.Maintenance);
                setField(BMS.Config.Controller.ResetFilterTimer, this->Config.Controller.ResetFilterTimer);

                if (this->SourceAddress != PrimaryAddress)
                    buffer[5] |= 0b00100000; // Unknown bit set in all captured config packets from secondary controller
            }

            setField(BMS.Config.Mode, static_cast<uint8_t>(this->Config.Mode));
            setField(BMS.Config.FanSpeed, static_cast<uint8_t>(this->Config.FanSpeed));

            setField(BMS.Config.Economy, this->Config.Economy);
            setField(BMS.Config.Enabled, this->Config.Enabled);
            setField(BMS.Config.Setpoint, this->Config.Setpoint);
            setField(BMS.Config.TestRun, this->Config.TestRun);

            setField(BMS.Config.SwingVertical, this->Config.SwingVertical);
            setField(BMS.Config.SwingHorizontal, this->Config.SwingHorizontal);

            break;

        case PacketTypeEnum::Error:
            setField(BMS.Error.ErrorCode, this->Error.ErrorCode);
            break;

        case PacketTypeEnum::Features:
            if (SourceType == AddressTypeEnum::IndoorUnit) {
                setField(BMS.Features.Mode.Cool, this->Features.Mode.Cool);
                setField(BMS.Features.Mode.Dry, this->Features.Mode.Dry);
                setField(BMS.Features.Mode.Fan, this->Features.Mode.Fan);
                setField(BMS.Features.Mode.Heat, this->Features.Mode.Heat);
                setField(BMS.Features.Mode.Auto, this->Features.Mode.Auto);

                setField(BMS.Features.FanSpeed.Auto, this->Features.FanSpeed.Auto);
                setField(BMS.Features.FanSpeed.High, this->Features.FanSpeed.High);
                setField(BMS.Features.FanSpeed.Medium, this->Features.FanSpeed.Medium);
                setField(BMS.Features.FanSpeed.Low, this->Features.FanSpeed.Low);
                setField(BMS.Features.FanSpeed.Quiet, this->Features.FanSpeed.Quiet);

                setField(BMS.Features.EconomyMode, this->Features.EconomyMode);
                setField(BMS.Features.FilterTimer, this->Features.FilterTimer);
                setField(BMS.Features.Maintenance, this->Features.Maintenance);
                setField(BMS.Features.SensorSwitching, this->Features.SensorSwitching);

                setField(BMS.Features.VerticalLouvers, this->Features.VerticalLouvers);
                setField(BMS.Features.HorizontalLouvers, this->Features.HorizontalLouvers);

                buffer[6] |= 0b00000001; // Unknown bit set in all captured features packets from indoor unit
            }
            break;

        case PacketTypeEnum::Function:
            if (SourceType == AddressTypeEnum::Controller)
                setField(BMS.Function.Controller.Write, this->Function.Controller.Write);

            setField(BMS.Function.Function, this->Function.Function);
            setField(BMS.Function.Value, this->Function.Value);
            setField(BMS.Function.Unit, this->Function.Unit);
            break;

        case PacketTypeEnum::Status:
            break;
    }

    this->invert_buffer(buffer);

    return buffer;
};

}
