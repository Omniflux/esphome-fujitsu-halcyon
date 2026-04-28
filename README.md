# Fujitsu AirStage-H component for ESPHome

An ESPHome component to control Fujitsu AirStage-H (product line previously known as Halcyon) units via the three wire (RWB) bus.

> [!WARNING]
> Requires ESPHome 2026.3.0 or newer.

```yaml
substitutions:
  device_name: halcyon-controller
  friendly_name: Halcyon Controller
  device_description: Atom Lite + FOSV
  esp_board: m5stack-atom

external_components:
#  - source: github://Omniflux/esphome-tzsp
  - source: github://Omniflux/esphome-fujitsu-halcyon

packages:
  wifi: !include common/wifi.yaml

esphome:
  name: ${device_name}
  friendly_name: ${friendly_name}
  comment: ${device_description}

esp32:
  board: ${esp_board}
  framework:
    type: esp-idf

api:

ota:
  - platform: esphome
    password: !secret ota_password

#logger:
#  level: DEBUG

button:
  - platform: restart
    name: Restart
  - platform: safe_mode
    name: Restart (Safe Mode)

sensor:
- platform: uptime
  name: "Uptime"

uart:
  tx_pin: GPIO22  # Device dependent
  rx_pin: GPIO19  # Device dependent
  baud_rate: 500
  parity: EVEN

climate:
- platform: fujitsu-halcyon
  name: None  # Use device friendly_name

  # Fujitsu devices use 0 and 1, but 2-15 should also work. Must not skip addresses
  controller_address: 1  # 0=Primary, 1=Secondary
  #temperature_controller_address: 0  # Fujitsu controller address to read temperature from

  #temperature_sensor_id: my_temperature_sensor  # ESPHome sensor to read temperature from
  #humidity_sensor: my_humidity_sensor  # ESPHome sensor to read humidity from

  #ignore_lock: true  # Ignore child/part/feature lock set on unit or primary/central remote control

  # To capture communications for debugging / analysis
  # Use Wireshark with https://github.com/Omniflux/fujitsu-airstage-h-dissector
  #tzsp:
  #  ip: 192.168.1.20
  #  protocol: 255
```

You can use esphome (or Home Assistant) sensors to report the current temperature and humidity to the Home Assistant climate component

```yaml
sensor:
  - platform: homeassistant # https://esphome.io/components/sensor/homeassistant.html
    id: my_temperature_sensor
    entity_id: sensor.my_temperature_sensor  # Home Assistant entity_id
    unit_of_measurement: "°F"  # unit_of_measurement is lost on import, defaults to °C

  - platform: homeassistant
    id: my_humidity_sensor
    entity_id: sensor.my_humidity_sensor

climate:
  - platform: fujitsu-halcyon
    name: None
    controller_address: 1
    temperature_sensor_id: my_temperature_sensor
    humidity_sensor: my_humidity_sensor
```

If your unit supports sensor switching and has had the function settings set appropriately (see your installation manual, usually settings `42` and `48`), your unit can also be set to use this sensor instead of the sensor in its air intake. When available, a switch will appear in the Home Assistant device page in the Configuration section named `Use Sensor`

Configure TZSP and use Wireshark with [fujitsu-airstage-h-dissector](https://github.com/Omniflux/fujitsu-airstage-h-dissector) to debug / decode the Fujitsu serial protocol.

## Home Assistant entities

The following entities are created automatically in Home Assistant. Feature-dependent entities (louvers, filter, sensor switching) are only exposed once the unit has reported its capabilities.

### Climate
| Entity | Type | Description |
|--------|------|-------------|
| *(friendly name)* | Climate | Main control: mode, fan speed, setpoint, swing, economy preset |

### Diagnostics
| Entity | Type | Default | Description |
|--------|------|---------|-------------|
| Connected | Binary sensor | Enabled | Whether the controller has completed initialization with the indoor unit |
| Standby Mode | Binary sensor | Enabled | Active during defrost, oil recovery, or multi-unit synchronization |
| Error | Binary sensor | Enabled | Indicates an active fault on the indoor unit |
| Error Code | Text sensor | Enabled | Fault code in `AA BB` hex format (unit address + error code) |
| Initialization Stage | Text sensor | Enabled | Current initialization progress, (4/4) indicates complete |
| Supported Features | Text sensor | Enabled | List of features reported by the indoor unit, published once at initialization. Example: `Mode: Auto Heat Cool Dry Fan \| Fan: Auto High Medium Low Quiet \| Economy \| Sensor Switching \| V.Louvers \| H.Louvers` |
| Remote Temperature Sensor | Sensor | Disabled | Temperature reported by another controller on the bus (see `temperature_controller_address`) |
| Filter Timer Expired | Binary sensor | Feature-dependent | Set when the filter maintenance timer has elapsed |

### Configuration
| Entity | Type | Default | Description |
|--------|------|---------|-------------|
| Use Sensor | Switch | Feature-dependent | Route the external temperature sensor reading to the indoor unit (requires unit support and `temperature_sensor_id` configured, see settings `42` and `48`) |
| Reset Filter Timer | Button | Feature-dependent | Reset the filter maintenance timer |
| Advance Vertical Louver | Button | Feature-dependent | Step the vertical louver to the next position |
| Advance Horizontal Louver | Button | Feature-dependent | Step the horizontal louver to the next position |
| Reinitialize | Button | Enabled | Re-run the initialization sequence without rebooting |
| Function / Function Value / Function Unit | Number | Enabled | Raw function register access |
| Function_Read / Function_Write | Button | Enabled / Disabled | Trigger a function register read or write |

## Related projects
- FOSV's [Fuji-Atom-Interface](https://github.com/FOSV/Fuji-Atom-Interface) - Open hardware interface compatible with this component
<!-- -->
- My [esphome-fujitsu-dmmum](https://github.com/Omniflux/esphome-fujitsu-dmmum) - Fujitsu AirStage-H 3-wire Central Controller component for ESPHome
<!-- -->
- Aaron Zhang's [esphome-fujitsu](https://github.com/FujiHeatPump/esphome-fujitsu)
- Jaroslaw Przybylowicz's [fuji-iot](https://github.com/jaroslawprzybylowicz/fuji-iot)
- Raal Goff's [FujiHeatPump](https://github.com/unreality/FujiHeatPump)
- Raal Goff's [FujiHK](https://github.com/unreality/FujiHK)
<!-- -->
- Myles Eftos's [Reverse engineering](https://hackaday.io/project/19473-reverse-engineering-a-fujitsu-air-conditioner-unit)
