import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    binary_sensor,
    button,
    climate,
    number,
    sensor,
    switch,
    text_sensor,
    tzsp,
    uart
)

from esphome.const import (
    CONF_ID,
    CONF_DISABLED_BY_DEFAULT,
    CONF_HUMIDITY_SENSOR,
    CONF_INTERNAL,
    CONF_MODE,
    CONF_NAME,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_PROBLEM,
    ENTITY_CATEGORY_CONFIG,
    ENTITY_CATEGORY_DIAGNOSTIC,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS
)

CODEOWNERS = ["@Omniflux"]
DEPENDENCIES = ["tzsp", "uart"]
AUTO_LOAD = ["binary_sensor", "button", "climate", "number", "sensor", "switch", "text_sensor", "tzsp"]

CONF_CONTROLLER_ADDRESS = "controller_address"
CONF_TEMPERATURE_CONTROLLER_ADDRESS = "temperature_controller_address"
CONF_TEMPERATURE_SENSOR = "temperature_sensor_id"
CONF_USE_SENSOR = "use_sensor"
CONF_IGNORE_LOCK = "ignore_lock"

CONF_STANDBY_MODE = "standby_mode"
CONF_ERROR_CODE = "error_code"
CONF_ERROR_STATE = "error_state"
CONF_REMOTE_SENSOR = "remote_sensor"
CONF_ADVANCE_VERTICAL_LOUVER = "advance_vertical_louver"
CONF_ADVANCE_HORIZONTAL_LOUVER = "advance_horizontal_louver"
CONF_RESET_FILTER_TIMER = "reset_filter_timer"
CONF_FILTER_TIMER_EXPIRED = "filter_timer_expired"
CONF_REINITIALIZE = "reinitialize"

CONF_FUNCTION = "function"
CONF_FUNCTION_VALUE = "function_value"
CONF_FUNCTION_UNIT = "function_unit"
CONF_GET_FUNCTION = "get_function"
CONF_SET_FUNCTION = "set_function"

BinarySensor = cg.esphome_ns.class_("BinarySensor", cg.Component, binary_sensor.BinarySensor)
TextSensor = cg.esphome_ns.class_("TextSensor", cg.Component, text_sensor.TextSensor)
Sensor = cg.esphome_ns.class_("Sensor", cg.Component, sensor.Sensor)

fujitsu_halcyon_ns = cg.esphome_ns.namespace("fujitsu_halcyon")
CustomButton = fujitsu_halcyon_ns.class_("CustomButton", cg.Component, button.Button)
CustomNumber = fujitsu_halcyon_ns.class_("CustomNumber", cg.Component, number.Number)
CustomSwitch = fujitsu_halcyon_ns.class_("CustomSwitch", cg.Component, switch.Switch)
FujitsuHalcyonController = fujitsu_halcyon_ns.class_("FujitsuHalcyonController", cg.Component, climate.Climate, uart.UARTDevice)

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
        cv.Optional(CONF_REINITIALIZE, default={CONF_NAME: "Reinitialize", CONF_INTERNAL: True}): button.button_schema(
            CustomButton,
            entity_category=ENTITY_CATEGORY_CONFIG,
        )
    }
).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA).extend(tzsp.TZSP_SENDER_SCHEMA)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "fujitsu_halcyon",
    require_tx=True,
    require_rx=True,
    baud_rate=500,
    data_bits=8,
    parity="EVEN",
    stop_bits=1
)

async def to_code(config):
    var = await climate.new_climate(config, await cg.get_variable(config[uart.CONF_UART_ID]), config[CONF_CONTROLLER_ADDRESS])
    await cg.register_component(var, config)
    await tzsp.register_tzsp_sender(var, config)
    await uart.register_uart_device(var, config)

    cg.add(var.set_temperature_controller_address(config[CONF_TEMPERATURE_CONTROLLER_ADDRESS]))
    cg.add(var.set_ignore_lock(config[CONF_IGNORE_LOCK]))

    varx = cg.Pvariable(config[CONF_STANDBY_MODE][CONF_ID], var.standby_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_STANDBY_MODE])

    varx = cg.Pvariable(config[CONF_ERROR_STATE][CONF_ID], var.error_sensor)
    await binary_sensor.register_binary_sensor(varx, config[CONF_ERROR_STATE])

    varx = cg.Pvariable(config[CONF_ERROR_CODE][CONF_ID], var.error_code_sensor)
    await text_sensor.register_text_sensor(varx, config[CONF_ERROR_CODE])

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
