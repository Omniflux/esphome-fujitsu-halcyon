import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import (
    binary_sensor,
    button,
    climate,
    sensor,
    switch,
    text_sensor,
    tzsp,
    uart
)

from esphome.const import (
    CONF_ID,
    CONF_HUMIDITY_SENSOR,
    CONF_INTERNAL,
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
AUTO_LOAD = ["binary_sensor", "button", "climate", "sensor", "switch", "text_sensor", "tzsp"]

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

BinarySensor = cg.esphome_ns.class_("BinarySensor", cg.Component, binary_sensor.BinarySensor)
TextSensor = cg.esphome_ns.class_("TextSensor", cg.Component, text_sensor.TextSensor)
Sensor = cg.esphome_ns.class_("Sensor", cg.Component, sensor.Sensor)

fujitsu_halcyon_ns = cg.esphome_ns.namespace("fujitsu_halcyon")
CustomButton = fujitsu_halcyon_ns.class_("CustomButton", cg.Component, button.Button)
CustomSwitch = fujitsu_halcyon_ns.class_("CustomSwitch", cg.Component, switch.Switch)
FujitsuHalcyonController = fujitsu_halcyon_ns.class_("FujitsuHalcyonController", cg.Component, climate.Climate, uart.UARTDevice)

CONFIG_SCHEMA = climate.CLIMATE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(FujitsuHalcyonController),
        cv.Optional(CONF_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_TEMPERATURE_CONTROLLER_ADDRESS, default=0): cv.int_range(0, 15),
        cv.Optional(CONF_IGNORE_LOCK, default=False): cv.boolean,
        cv.Optional(CONF_TEMPERATURE_SENSOR): cv.use_id(sensor.Sensor),
        cv.Optional(CONF_HUMIDITY_SENSOR): cv.use_id(sensor.Sensor),
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
        ),
        cv.Optional(CONF_ZONE_1, default={CONF_NAME: "Zone 1", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_2, default={CONF_NAME: "Zone 2", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_3, default={CONF_NAME: "Zone 3", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_4, default={CONF_NAME: "Zone 4", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_5, default={CONF_NAME: "Zone 5", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_6, default={CONF_NAME: "Zone 6", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_7, default={CONF_NAME: "Zone 7", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_8, default={CONF_NAME: "Zone 8", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_GROUP_DAY, default={CONF_NAME: "Zone Group Day", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
        ),
        cv.Optional(CONF_ZONE_GROUP_NIGHT, default={CONF_NAME: "Zone Group Night", CONF_INTERNAL: True}): switch.switch_schema(
            CustomSwitch,
            entity_category=ENTITY_CATEGORY_CONFIG,
            default_restore_mode="RESTORE_DEFAULT_ON"
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
    var = cg.new_Pvariable(config[CONF_ID], await cg.get_variable(config[uart.CONF_UART_ID]), config[CONF_CONTROLLER_ADDRESS])
    await cg.register_component(var, config)
    await climate.register_climate(var, config)
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

    if CONF_TEMPERATURE_SENSOR in config:
        cg.add(var.set_temperature_sensor(await cg.get_variable(config[CONF_TEMPERATURE_SENSOR])))

    if CONF_HUMIDITY_SENSOR in config:
        cg.add(var.set_humidity_sensor(await cg.get_variable(config[CONF_HUMIDITY_SENSOR])))

    varx = cg.Pvariable(config[CONF_ZONE_1][CONF_ID], var.zone_switches[0])
    await switch.register_switch(varx, config[CONF_ZONE_1])

    varx = cg.Pvariable(config[CONF_ZONE_2][CONF_ID], var.zone_switches[1])
    await switch.register_switch(varx, config[CONF_ZONE_2])

    varx = cg.Pvariable(config[CONF_ZONE_3][CONF_ID], var.zone_switches[2])
    await switch.register_switch(varx, config[CONF_ZONE_3])

    varx = cg.Pvariable(config[CONF_ZONE_4][CONF_ID], var.zone_switches[3])
    await switch.register_switch(varx, config[CONF_ZONE_4])

    varx = cg.Pvariable(config[CONF_ZONE_5][CONF_ID], var.zone_switches[4])
    await switch.register_switch(varx, config[CONF_ZONE_5])

    varx = cg.Pvariable(config[CONF_ZONE_6][CONF_ID], var.zone_switches[5])
    await switch.register_switch(varx, config[CONF_ZONE_6])

    varx = cg.Pvariable(config[CONF_ZONE_7][CONF_ID], var.zone_switches[6])
    await switch.register_switch(varx, config[CONF_ZONE_7])

    varx = cg.Pvariable(config[CONF_ZONE_8][CONF_ID], var.zone_switches[7])
    await switch.register_switch(varx, config[CONF_ZONE_8])

    varx = cg.Pvariable(config[CONF_ZONE_GROUP_DAY][CONF_ID], var.zone_group_day_switch)
    await switch.register_switch(varx, config[CONF_ZONE_GROUP_DAY])

    varx = cg.Pvariable(config[CONF_ZONE_GROUP_NIGHT][CONF_ID], var.zone_group_night_switch)
    await switch.register_switch(varx, config[CONF_ZONE_GROUP_NIGHT])
