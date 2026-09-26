import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv

from esphome.core import CORE

from esphome.components import (
    binary_sensor,
    button,
    climate,
    number,
    sensor,
    switch,
    text_sensor,
    uart
)

try:
    from esphome.components import tzsp
except ImportError:
    TZSP_AVAILABLE = False
else:
    TZSP_AVAILABLE = True

from esphome.const import (
    CONF_DISABLED_BY_DEFAULT,
    CONF_HUMIDITY_SENSOR,
    CONF_MODE,
    CONF_NAME,
    CONF_UART_ID,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_CONNECTIVITY,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
)

from esphome.types import ConfigType

CODEOWNERS = ["@Omniflux"]
DEPENDENCIES = ["uart"]

def AUTO_LOAD(config: ConfigType) -> list[str]:
    load = ["binary_sensor", "button", "climate", "number", "sensor", "switch", "text_sensor"]

    if TZSP_AVAILABLE and config.get(tzsp.CONF_TZSP):
        load += ["tzsp"]

    return load

CONF_CONTROLLER_ADDRESS = "controller_address"
CONF_TEMPERATURE_CONTROLLER_ADDRESS = "temperature_controller_address"
CONF_TEMPERATURE_SENSOR = "temperature_sensor_id"
CONF_USE_SENSOR = "use_sensor"
CONF_IGNORE_LOCK = "ignore_lock"
CONF_SENSOR_TIMEOUT = "sensor_timeout"
CONF_INIT_TIMEOUT = "init_timeout"

# Feature negotiation override options.
# When the indoor unit responds to a FeatureRequest with a Features packet, the
# IU's reported feature set is used and these options are ignored. Use these
# options when (a) the IU does not support feature negotiation (responds with
# Config instead of Features), or (b) you want to disable probing entirely with
# `autoconf: false` for IUs known to misbehave on FeatureRequest. Anything not
# specified keeps the in-code DefaultFeatures value.
CONF_AUTOCONF = "autoconf"
CONF_SUPPORTED_MODES = "supported_modes"
CONF_SUPPORTED_FAN_MODES = "supported_fan_modes"
CONF_SUPPORTED_SWING_MODES = "supported_swing_modes"
CONF_SUPPORTED_FEATURES = "supported_features"

ALLOWED_MODES = {"AUTO", "HEAT", "FAN", "DRY", "COOL"}
ALLOWED_FAN_MODES = {"QUIET", "LOW", "MEDIUM", "HIGH", "AUTO"}
ALLOWED_SWING_MODES = {"VERTICAL", "HORIZONTAL", "BOTH"}
# Same words as the Supported Features diagnostic sensor prints.
ALLOWED_FEATURES = {"FILTER_TIMER", "SENSOR_SWITCHING", "MAINTENANCE", "ECONOMY"}

CONF_STANDBY_MODE = "standby_mode"
CONF_ERROR_CODE = "error_code"
CONF_ERROR_STATE = "error_state"
CONF_INITIALIZATION_STAGE = "initialization_stage"
CONF_REMOTE_SENSOR = "remote_sensor"
CONF_ADVANCE_VERTICAL_LOUVER = "advance_vertical_louver"
CONF_ADVANCE_HORIZONTAL_LOUVER = "advance_horizontal_louver"
CONF_RESET_FILTER_TIMER = "reset_filter_timer"
CONF_FILTER_TIMER_EXPIRED = "filter_timer_expired"
CONF_REINITIALIZE = "reinitialize"
CONF_CONNECTED = "connected"
# The diagnostic text sensor listing what the unit reports. Named apart from the
# supported_features override list above.
CONF_REPORTED_FEATURES = "reported_features"

CONF_ZONE_1 = "zone_1"
CONF_ZONE_2 = "zone_2"
CONF_ZONE_3 = "zone_3"
CONF_ZONE_4 = "zone_4"
CONF_ZONE_5 = "zone_5"
CONF_ZONE_6 = "zone_6"
CONF_ZONE_7 = "zone_7"
CONF_ZONE_8 = "zone_8"
CONF_ZONE_GROUP_DAY = "zone_group_day"
CONF_ZONE_GROUP_NIGHT = "zone_group_night"
ZONE_KEYS = (CONF_ZONE_1, CONF_ZONE_2, CONF_ZONE_3, CONF_ZONE_4,
             CONF_ZONE_5, CONF_ZONE_6, CONF_ZONE_7, CONF_ZONE_8)

CONF_FUNCTION = "function"
CONF_FUNCTION_VALUE = "function_value"
CONF_FUNCTION_UNIT = "function_unit"
CONF_GET_FUNCTION = "get_function"
CONF_SET_FUNCTION = "set_function"

# Feature-dependent entities (louvers, filter, use sensor, remote sensor, zones)
# are declared explicitly in YAML and created only when present (see to_code),
# the standard ESPHome pattern. The entity set is fixed at config time and an
# undeclared entity does not exist. Core diagnostics and the function controls
# stay always present.

fujitsu_general_airstage_h_controller_ns = cg.esphome_ns.namespace("fujitsu_general_airstage_h_controller")
FujitsuHalcyonController = fujitsu_general_airstage_h_controller_ns.class_("FujitsuHalcyonController", cg.Component, climate.Climate, uart.UARTDevice)

# Concrete feature entities created in python only when declared. Each is Parented
# to the controller and calls it to act or to publish unit state.
AdvanceVerticalLouverButton = fujitsu_general_airstage_h_controller_ns.class_("AdvanceVerticalLouverButton", button.Button)
AdvanceHorizontalLouverButton = fujitsu_general_airstage_h_controller_ns.class_("AdvanceHorizontalLouverButton", button.Button)
UseSensorSwitch = fujitsu_general_airstage_h_controller_ns.class_("UseSensorSwitch", switch.Switch)
ResetFilterButton = fujitsu_general_airstage_h_controller_ns.class_("ResetFilterButton", button.Button)
ZoneSwitch = fujitsu_general_airstage_h_controller_ns.class_("ZoneSwitch", switch.Switch)
ZoneGroupDaySwitch = fujitsu_general_airstage_h_controller_ns.class_("ZoneGroupDaySwitch", switch.Switch)
ZoneGroupNightSwitch = fujitsu_general_airstage_h_controller_ns.class_("ZoneGroupNightSwitch", switch.Switch)
ReinitializeButton = fujitsu_general_airstage_h_controller_ns.class_("ReinitializeButton", button.Button)
GetFunctionButton = fujitsu_general_airstage_h_controller_ns.class_("GetFunctionButton", button.Button)
SetFunctionButton = fujitsu_general_airstage_h_controller_ns.class_("SetFunctionButton", button.Button)
FunctionNumber = fujitsu_general_airstage_h_controller_ns.class_("FunctionNumber", number.Number)

PACKET_FRAME_SIZE = 8
UART_INTER_PACKET_SYMBOL_SPACING = 2

COMPONENT_NAME = __name__.split('.')[-2]

def _feature_entity(schema, default_name):
    # A feature entity that is created only when declared. Declaring the key,
    # even empty (`key:`), applies a default name. Use `key: {name: "..."}` to
    # customize it.
    def _normalize(value):
        if value is None:
            value = {}
        elif isinstance(value, dict):
            value = dict(value)
        else:
            raise cv.Invalid(
                "expected either an empty value or a mapping of options, "
                f'for example `{{name: "{default_name}"}}`'
            )
        value.setdefault(CONF_NAME, default_name)
        return value
    return cv.All(_normalize, schema)

CONFIG_SCHEMA = climate.climate_schema(FujitsuHalcyonController).extend(
    {
        cv.Optional(CONF_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_TEMPERATURE_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_IGNORE_LOCK, default=False): cv.boolean,
        # If initialization has not completed after this long, and the unit has
        # answered at least once, restart it automatically (same as the
        # Reinitialize button). 0 disables.
        cv.Optional(CONF_INIT_TIMEOUT, default="30s"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        # After this long without a valid temperature_sensor_id reading, the unit is
        # switched back to its own sensor until readings resume. 0 disables.
        cv.Optional(CONF_SENSOR_TIMEOUT, default="5min"): cv.positive_time_period_milliseconds,
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_AUTOCONF): cv.boolean,
        cv.Optional(CONF_SUPPORTED_MODES): cv.ensure_list(cv.one_of(*ALLOWED_MODES, upper=True)),
        cv.Optional(CONF_SUPPORTED_FAN_MODES): cv.ensure_list(cv.one_of(*ALLOWED_FAN_MODES, upper=True)),
        cv.Optional(CONF_SUPPORTED_SWING_MODES): cv.ensure_list(cv.one_of(*ALLOWED_SWING_MODES, upper=True)),
        cv.Optional(CONF_SUPPORTED_FEATURES): cv.ensure_list(cv.one_of(*ALLOWED_FEATURES, upper=True)),
        cv.Optional(CONF_FUNCTION, default={CONF_NAME: "Function", CONF_MODE: "BOX"}): number.number_schema(
            FunctionNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_FUNCTION_VALUE, default={CONF_NAME: "Function Value", CONF_MODE: "BOX"}): number.number_schema(
            FunctionNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_FUNCTION_UNIT, default={CONF_NAME: "Function Unit", CONF_MODE: "BOX"}): number.number_schema(
            FunctionNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_GET_FUNCTION, default={CONF_NAME: "Function Read"}): button.button_schema(
            GetFunctionButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_SET_FUNCTION, default={CONF_NAME: "Function Write", CONF_DISABLED_BY_DEFAULT: True}): button.button_schema(
            SetFunctionButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        # Feature-dependent entities. Created only when declared. Declaring a key
        # (even empty) uses a default name, override with `key: {name: "..."}`.
        # use_sensor is the one switch whose state is the user's choice rather than
        # the unit's, so it is the only one with a real restore mode (applied by the
        # component, see esphome-fujitsu-halcyon.cpp).
        cv.Optional(CONF_USE_SENSOR): _feature_entity(switch.switch_schema(
            UseSensorSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_OFF"
        ), "Use Sensor"),
        cv.Optional(CONF_REMOTE_SENSOR): _feature_entity(sensor.sensor_schema(
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ), "Remote Temperature Sensor"),
        cv.Optional(CONF_STANDBY_MODE, default={CONF_NAME: "Standby Mode"}): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_ERROR_STATE, default={CONF_NAME: "Error"}): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ),
        cv.Optional(CONF_ERROR_CODE, default={CONF_NAME: "Error Code"}): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_INITIALIZATION_STAGE, default={CONF_NAME: "Initialization Stage"}): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_ADVANCE_VERTICAL_LOUVER): _feature_entity(button.button_schema(
            AdvanceVerticalLouverButton
        ), "Advance Vertical Louver"),
        cv.Optional(CONF_ADVANCE_HORIZONTAL_LOUVER): _feature_entity(button.button_schema(
            AdvanceHorizontalLouverButton
        ), "Advance Horizontal Louver"),
        cv.Optional(CONF_RESET_FILTER_TIMER): _feature_entity(button.button_schema(
            ResetFilterButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ), "Reset Filter Timer"),
        cv.Optional(CONF_FILTER_TIMER_EXPIRED): _feature_entity(binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ), "Filter Timer Expired"),
        cv.Optional(CONF_REINITIALIZE, default={CONF_NAME: "Reinitialize"}): button.button_schema(
            ReinitializeButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_CONNECTED, default={CONF_NAME: "Connected"}): binary_sensor.binary_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_CONNECTIVITY
        ),
        cv.Optional(CONF_REPORTED_FEATURES, default={CONF_NAME: "Supported Features"}): text_sensor.text_sensor_schema(
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        **{
            cv.Optional(key): _feature_entity(switch.switch_schema(
                ZoneSwitch,
                entity_category=ENTITY_CATEGORY_CONFIG,
                # Zone state comes from the indoor unit, nothing to restore.
                default_restore_mode="DISABLED"
            ), f"Zone {i}")
            for i, key in enumerate(ZONE_KEYS, start=1)
        },
        cv.Optional(CONF_ZONE_GROUP_DAY): _feature_entity(switch.switch_schema(
            ZoneGroupDaySwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="DISABLED"
        ), "Zone Group Day"),
        cv.Optional(CONF_ZONE_GROUP_NIGHT): _feature_entity(switch.switch_schema(
            ZoneGroupNightSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="DISABLED"
        ), "Zone Group Night"),
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

if TZSP_AVAILABLE:
    CONFIG_SCHEMA = CONFIG_SCHEMA.extend(tzsp.TZSP_SENDER_SCHEMA)

def _validate_use_sensor(config):
    # The switch tells the unit to use the temperature this component reports to
    # it, so without a sensor to report there is nothing for it to switch to.
    if CONF_USE_SENSOR in config and CONF_TEMPERATURE_SENSOR not in config:
        raise cv.Invalid(
            f"{CONF_USE_SENSOR} needs {CONF_TEMPERATURE_SENSOR} to be set, "
            "otherwise the switch has no temperature to give the unit",
            path=[CONF_USE_SENSOR]
        )

    return config

CONFIG_SCHEMA = cv.All(CONFIG_SCHEMA, _validate_use_sensor)

def check_platform(config):
    # This component relies on the ESP-IDF RS485 half-duplex UART driver
    # (uart_set_mode / driver/uart.h), so it only builds for ESP32 + esp-idf.
    # Fail early with a clear message instead of a wall of compiler errors.
    if not CORE.is_esp32:
        raise cv.Invalid(
            f"Component {COMPONENT_NAME} only supports the ESP32 platform "
            "(it uses the ESP-IDF RS485 half-duplex UART driver)."
        )

    if CORE.target_framework != "esp-idf":
        raise cv.Invalid(
            f"Component {COMPONENT_NAME} requires the esp-idf framework. Set:\n"
            "  esp32:\n    framework:\n      type: esp-idf"
        )

    return config

def final_validate_uart_schema(config):
    def validate_rx_full_threshold(value):
        if not isinstance(value, int) or value < PACKET_FRAME_SIZE * 2:
            raise cv.Invalid(f"Component {COMPONENT_NAME} requires {uart.CONF_RX_FULL_THRESHOLD} >= {PACKET_FRAME_SIZE * 2}  for the uart referenced by {CONF_UART_ID}")
        return value

    def validate_rx_timeout(value):
        if value != UART_INTER_PACKET_SYMBOL_SPACING:
            raise cv.Invalid(f"Component {COMPONENT_NAME} requires {uart.CONF_RX_TIMEOUT} = {UART_INTER_PACKET_SYMBOL_SPACING} for the uart referenced by {CONF_UART_ID}")
        return value

    # This should not be done this way; Not sure of the proper way to do it...
    full_config = fv.full_config.get()
    uart_path = full_config.get_path_for_id(config[CONF_UART_ID])[:-1]
    uart_conf = full_config.get_config_for_path(uart_path)
    if uart.CONF_RX_FULL_THRESHOLD not in uart_conf:
        uart_conf[uart.CONF_RX_FULL_THRESHOLD] = PACKET_FRAME_SIZE * 2

    cv.Schema(
        {
            cv.Required(CONF_UART_ID): fv.id_declaration_match_schema(
                {
                    cv.Optional(uart.CONF_RX_FULL_THRESHOLD, default=PACKET_FRAME_SIZE * 2): validate_rx_full_threshold,
                    cv.Optional(uart.CONF_RX_TIMEOUT, default=UART_INTER_PACKET_SYMBOL_SPACING): validate_rx_timeout,
                },
            ),
        },
        extra=cv.ALLOW_EXTRA,
    )(config)

    return config

FINAL_VALIDATE_SCHEMA = cv.All(
    cv.require_esphome_version(2026, 3, 0),
    check_platform,
    final_validate_uart_schema,
    uart.final_validate_device_schema(
        COMPONENT_NAME,
        require_tx=True,
        require_rx=True,
        baud_rate=500,
        data_bits=8,
        parity="EVEN",
        stop_bits=1,
    ),
)

async def to_code(config: ConfigType) -> None:
    var = await climate.new_climate(config, config[CONF_CONTROLLER_ADDRESS])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if TZSP_AVAILABLE and config.get(tzsp.CONF_TZSP):
        await tzsp.register_tzsp_sender(var, config)
        cg.add_define("USE_TZSP")

    cg.add(var.set_temperature_controller_address(config[CONF_TEMPERATURE_CONTROLLER_ADDRESS]))
    cg.add(var.set_ignore_lock(config[CONF_IGNORE_LOCK]))
    cg.add(var.set_sensor_timeout(config[CONF_SENSOR_TIMEOUT]))
    cg.add(var.set_init_timeout(config[CONF_INIT_TIMEOUT]))

    # Apply feature negotiation overrides. Anything omitted from YAML keeps the
    # in-code DefaultFeatures value.
    if CONF_AUTOCONF in config:
        cg.add(var.set_autoconf(config[CONF_AUTOCONF]))
    if CONF_SUPPORTED_MODES in config:
        modes = set(config[CONF_SUPPORTED_MODES])
        cg.add(var.set_supported_modes(
            "AUTO" in modes, "HEAT" in modes, "FAN" in modes, "DRY" in modes, "COOL" in modes
        ))
    if CONF_SUPPORTED_FAN_MODES in config:
        fan_modes = set(config[CONF_SUPPORTED_FAN_MODES])
        cg.add(var.set_supported_fan_modes(
            "QUIET" in fan_modes, "LOW" in fan_modes, "MEDIUM" in fan_modes,
            "HIGH" in fan_modes, "AUTO" in fan_modes
        ))
    if CONF_SUPPORTED_SWING_MODES in config:
        swing_modes = set(config[CONF_SUPPORTED_SWING_MODES])
        vertical = "VERTICAL" in swing_modes or "BOTH" in swing_modes
        horizontal = "HORIZONTAL" in swing_modes or "BOTH" in swing_modes
        cg.add(var.set_supported_swing_modes(vertical, horizontal))
    if CONF_SUPPORTED_FEATURES in config:
        features = set(config[CONF_SUPPORTED_FEATURES])
        cg.add(var.set_supported_features(
            "FILTER_TIMER" in features, "SENSOR_SWITCHING" in features,
            "MAINTENANCE" in features, "ECONOMY" in features
        ))

    # Always-present diagnostics, created in python and given to the hub via setters.
    s = await binary_sensor.new_binary_sensor(config[CONF_STANDBY_MODE])
    cg.add(var.set_standby_sensor(s))

    s = await binary_sensor.new_binary_sensor(config[CONF_ERROR_STATE])
    cg.add(var.set_error_sensor(s))

    s = await text_sensor.new_text_sensor(config[CONF_ERROR_CODE])
    cg.add(var.set_error_code_sensor(s))

    s = await text_sensor.new_text_sensor(config[CONF_INITIALIZATION_STAGE])
    cg.add(var.set_initialization_sensor(s))

    # Always-present controls.
    b = await button.new_button(config[CONF_GET_FUNCTION])
    await cg.register_parented(b, var)

    b = await button.new_button(config[CONF_SET_FUNCTION])
    await cg.register_parented(b, var)

    b = await button.new_button(config[CONF_REINITIALIZE])
    await cg.register_parented(b, var)

    s = await binary_sensor.new_binary_sensor(config[CONF_CONNECTED])
    cg.add(var.set_connected_sensor(s))

    s = await text_sensor.new_text_sensor(config[CONF_REPORTED_FEATURES])
    cg.add(var.set_supported_features_sensor(s))

    n = await number.new_number(config[CONF_FUNCTION], min_value=0, max_value=255, step=1)
    cg.add(var.set_function_number(n))

    n = await number.new_number(config[CONF_FUNCTION_VALUE], min_value=0, max_value=255, step=1)
    cg.add(var.set_function_value_number(n))

    n = await number.new_number(config[CONF_FUNCTION_UNIT], min_value=0, max_value=15, step=1)
    cg.add(var.set_function_unit_number(n))

    # Feature-dependent entities. Registered only when declared in YAML, so an
    # undeclared entity is never exposed to Home Assistant. These are created
    # here in python, so nothing lives in the header when the entity is not
    # declared. Each is parented to the controller.
    if CONF_ADVANCE_VERTICAL_LOUVER in config:
        b = await button.new_button(config[CONF_ADVANCE_VERTICAL_LOUVER])
        await cg.register_parented(b, var)
        cg.add(var.set_louver_v_declared(True))

    if CONF_ADVANCE_HORIZONTAL_LOUVER in config:
        b = await button.new_button(config[CONF_ADVANCE_HORIZONTAL_LOUVER])
        await cg.register_parented(b, var)
        cg.add(var.set_louver_h_declared(True))

    if CONF_USE_SENSOR in config:
        sw = await switch.new_switch(config[CONF_USE_SENSOR])
        await cg.register_parented(sw, var)
        cg.add(var.set_use_sensor_switch(sw))
        cg.add(var.set_use_sensor_declared(True))

    if CONF_REMOTE_SENSOR in config:
        s = await sensor.new_sensor(config[CONF_REMOTE_SENSOR])
        cg.add(var.set_remote_sensor(s))

    if CONF_RESET_FILTER_TIMER in config:
        b = await button.new_button(config[CONF_RESET_FILTER_TIMER])
        await cg.register_parented(b, var)

    if CONF_FILTER_TIMER_EXPIRED in config:
        bs = await binary_sensor.new_binary_sensor(config[CONF_FILTER_TIMER_EXPIRED])
        cg.add(var.set_filter_sensor(bs))

    for i, zone_key in enumerate(ZONE_KEYS):
        if zone_key in config:
            sw = await switch.new_switch(config[zone_key])
            await cg.register_parented(sw, var)
            cg.add(sw.set_zone_index(i))
            cg.add(var.set_zone_switch(i, sw))

    if CONF_ZONE_GROUP_DAY in config:
        sw = await switch.new_switch(config[CONF_ZONE_GROUP_DAY])
        await cg.register_parented(sw, var)
        cg.add(var.set_zone_group_day_switch(sw))

    if CONF_ZONE_GROUP_NIGHT in config:
        sw = await switch.new_switch(config[CONF_ZONE_GROUP_NIGHT])
        await cg.register_parented(sw, var)
        cg.add(var.set_zone_group_night_switch(sw))

    # Record which feature entities were declared, so the component can warn at
    # startup if the indoor unit does not actually report that feature. (use_sensor
    # and louver flags are set above, next to where those entities are created.)
    if CONF_FILTER_TIMER_EXPIRED in config or CONF_RESET_FILTER_TIMER in config:
        cg.add(var.set_filter_entity_declared(True))
    if any(z in config for z in (*ZONE_KEYS, CONF_ZONE_GROUP_DAY, CONF_ZONE_GROUP_NIGHT)):
        cg.add(var.set_zones_declared(True))

    if CONF_TEMPERATURE_SENSOR in config:
        cg.add(var.set_temperature_sensor(await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])))

    if CONF_HUMIDITY_SENSOR in config:
        cg.add(var.set_humidity_sensor(await cg.get_variable(config[CONF_HUMIDITY_SENSOR])))
