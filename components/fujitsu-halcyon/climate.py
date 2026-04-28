import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv

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
    CONF_ID,
    CONF_DISABLED_BY_DEFAULT,
    CONF_HUMIDITY_SENSOR,
    CONF_INTERNAL,
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
CONF_SUPPORTED_FEATURES = "supported_features"

CONF_FUNCTION = "function"
CONF_FUNCTION_VALUE = "function_value"
CONF_FUNCTION_UNIT = "function_unit"
CONF_GET_FUNCTION = "get_function"
CONF_SET_FUNCTION = "set_function"

BinarySensor = cg.esphome_ns.class_("BinarySensor", cg.Component, binary_sensor.BinarySensor)
TextSensor = cg.esphome_ns.class_("TextSensor", cg.Component, text_sensor.TextSensor)
Sensor = cg.esphome_ns.class_("Sensor", cg.Component, sensor.Sensor)

custom_ns = cg.esphome_ns.namespace("custom")
CustomButton = custom_ns.class_("CustomButton", cg.Component, button.Button)
CustomNumber = custom_ns.class_("CustomNumber", cg.Component, number.Number)
CustomSwitch = custom_ns.class_("CustomSwitch", cg.Component, switch.Switch)
fujitsu_general_airstage_h_controller_ns = cg.esphome_ns.namespace("fujitsu_general_airstage_h_controller")
FujitsuHalcyonController = fujitsu_general_airstage_h_controller_ns.class_("FujitsuHalcyonController", cg.Component, climate.Climate, uart.UARTDevice)

PACKET_FRAME_SIZE = 8
UART_INTER_PACKET_SYMBOL_SPACING = 2

COMPONENT_NAME = __name__.split('.')[-2]

CONFIG_SCHEMA = climate.climate_schema(FujitsuHalcyonController).extend(
    {
        cv.Optional(CONF_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_TEMPERATURE_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_IGNORE_LOCK, default=False): cv.boolean,
        cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_FUNCTION, default={CONF_NAME: "Function", CONF_MODE: "BOX"}): number.number_schema(
            CustomNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_FUNCTION_VALUE, default={CONF_NAME: "Function Value", CONF_MODE: "BOX"}): number.number_schema(
            CustomNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_FUNCTION_UNIT, default={CONF_NAME: "Function Unit", CONF_MODE: "BOX"}): number.number_schema(
            CustomNumber,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_GET_FUNCTION, default={CONF_NAME: "Function_Read"}): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_SET_FUNCTION, default={CONF_NAME: "Function_Write", CONF_DISABLED_BY_DEFAULT: True}): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_USE_SENSOR, default={CONF_NAME: "Use Sensor", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_OFF"
        ),
        cv.Optional(CONF_REMOTE_SENSOR, default={CONF_NAME: "Remote Temperature Sensor", CONF_INTERNAL: True}): sensor.sensor_schema(
            Sensor,
            unit_of_measurement=UNIT_CELSIUS,
            device_class=DEVICE_CLASS_TEMPERATURE,
            state_class=STATE_CLASS_MEASUREMENT,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_STANDBY_MODE, default={CONF_NAME: "Standby Mode"}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_ERROR_STATE, default={CONF_NAME: "Error"}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ),
        cv.Optional(CONF_ERROR_CODE, default={CONF_NAME: "Error Code"}): text_sensor.text_sensor_schema(
            TextSensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_INITIALIZATION_STAGE, default={CONF_NAME: "Initialization Stage"}): text_sensor.text_sensor_schema(
            TextSensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
        cv.Optional(CONF_ADVANCE_VERTICAL_LOUVER, default={CONF_NAME: "Advance Vertical Louver", CONF_INTERNAL: True}): button.button_schema(
            CustomButton
        ),
        cv.Optional(CONF_ADVANCE_HORIZONTAL_LOUVER, default={CONF_NAME: "Advance Horizontal Louver", CONF_INTERNAL: True}): button.button_schema(
            CustomButton
        ),
        cv.Optional(CONF_RESET_FILTER_TIMER, default={CONF_NAME: "Reset Filter Timer", CONF_INTERNAL: True}): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG
        ),
        cv.Optional(CONF_FILTER_TIMER_EXPIRED, default={CONF_NAME: "Filter Timer Expired", CONF_INTERNAL: True}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_PROBLEM
        ),
        cv.Optional(CONF_REINITIALIZE, default={CONF_NAME: "Reinitialize"}): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        ),
        cv.Optional(CONF_CONNECTED, default={CONF_NAME: "Connected"}): binary_sensor.binary_sensor_schema(
            BinarySensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC,
            device_class=DEVICE_CLASS_CONNECTIVITY
        ),
        cv.Optional(CONF_SUPPORTED_FEATURES, default={CONF_NAME: "Supported Features"}): text_sensor.text_sensor_schema(
            TextSensor,
            entity_category=ENTITY_CATEGORY_DIAGNOSTIC
        ),
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA)

if TZSP_AVAILABLE:
    CONFIG_SCHEMA = CONFIG_SCHEMA.extend(tzsp.TZSP_SENDER_SCHEMA)

def check_esphome_version(config):
    if cv.parse_esphome_version() < (2026, 3, 0):
        raise cv.Invalid(f"Component {COMPONENT_NAME} requires ESPHome 2026.3.0 or newer.")

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
    check_esphome_version,
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
    var = await climate.new_climate(config, await cg.get_variable(config[uart.CONF_UART_ID]), config[CONF_CONTROLLER_ADDRESS])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if TZSP_AVAILABLE and config.get(tzsp.CONF_TZSP):
        await tzsp.register_tzsp_sender(var, config)
        cg.add_define("USE_TZSP")

    cg.add(var.set_temperature_controller_address(config[CONF_TEMPERATURE_CONTROLLER_ADDRESS]))
    cg.add(var.set_ignore_lock(config[CONF_IGNORE_LOCK]))

    varx = cg.Pvariable(config[CONF_STANDBY_MODE][CONF_ID], var.standby_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_STANDBY_MODE])

    varx = cg.Pvariable(config[CONF_ERROR_STATE][CONF_ID], var.error_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_ERROR_STATE])

    varx = cg.Pvariable(config[CONF_ERROR_CODE][CONF_ID], var.error_code_sensor)
    await text_sensor.register_text_sensor(varx, config[CONF_ERROR_CODE])

    varx = cg.Pvariable(config[CONF_INITIALIZATION_STAGE][CONF_ID], var.initialization_sensor)
    await text_sensor.register_text_sensor(varx, config[CONF_INITIALIZATION_STAGE])

    varx = cg.Pvariable(config[CONF_USE_SENSOR][CONF_ID], var.use_sensor_switch)
    await switch.register_switch(varx, config[CONF_USE_SENSOR])

    varx = cg.Pvariable(config[CONF_GET_FUNCTION][CONF_ID], var.get_function)
    await button.register_button(varx, config[CONF_GET_FUNCTION])

    varx = cg.Pvariable(config[CONF_SET_FUNCTION][CONF_ID], var.set_function)
    await button.register_button(varx, config[CONF_SET_FUNCTION])

    varx = cg.Pvariable(config[CONF_ADVANCE_VERTICAL_LOUVER][CONF_ID], var.advance_vertical_louver_button)
    await button.register_button(varx, config[CONF_ADVANCE_VERTICAL_LOUVER])

    varx = cg.Pvariable(config[CONF_ADVANCE_HORIZONTAL_LOUVER][CONF_ID], var.advance_horizontal_louver_button)
    await button.register_button(varx, config[CONF_ADVANCE_HORIZONTAL_LOUVER])

    varx = cg.Pvariable(config[CONF_RESET_FILTER_TIMER][CONF_ID], var.reset_filter_button)
    await button.register_button(varx, config[CONF_RESET_FILTER_TIMER])

    varx = cg.Pvariable(config[CONF_FILTER_TIMER_EXPIRED][CONF_ID], var.filter_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_FILTER_TIMER_EXPIRED])

    varx = cg.Pvariable(config[CONF_REINITIALIZE][CONF_ID], var.reinitialize_button)
    await button.register_button(varx, config[CONF_REINITIALIZE])

    varx = cg.Pvariable(config[CONF_CONNECTED][CONF_ID], var.connected_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_CONNECTED])

    varx = cg.Pvariable(config[CONF_SUPPORTED_FEATURES][CONF_ID], var.supported_features_sensor)
    await text_sensor.register_text_sensor(varx, config[CONF_SUPPORTED_FEATURES])

    varx = cg.Pvariable(config[CONF_REMOTE_SENSOR][CONF_ID], var.remote_sensor)
    await sensor.register_sensor(varx, config[CONF_REMOTE_SENSOR])

    varx = cg.Pvariable(config[CONF_FUNCTION][CONF_ID], var.function)
    await number.register_number(
        varx,
        config[CONF_FUNCTION],
        min_value=0,
        max_value=255,
        step=1
    )

    varx = cg.Pvariable(config[CONF_FUNCTION_VALUE][CONF_ID], var.function_value)
    await number.register_number(
        varx,
        config[CONF_FUNCTION_VALUE],
        min_value=0,
        max_value=255,
        step=1
    )

    varx = cg.Pvariable(config[CONF_FUNCTION_UNIT][CONF_ID], var.function_unit)
    await number.register_number(
        varx,
        config[CONF_FUNCTION_UNIT],
        min_value=0,
        max_value=15,
        step=1
    )

    if CONF_TEMPERATURE_SENSOR in config:
        cg.add(var.set_temperature_sensor(await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])))

    if CONF_HUMIDITY_SENSOR in config:
        cg.add(var.set_humidity_sensor(await cg.get_variable(config[CONF_HUMIDITY_SENSOR])))
