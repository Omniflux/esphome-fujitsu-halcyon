# Fujitsu AirStage-H component for ESPHome

An ESPHome component to control Fujitsu AirStage-H (product line previously known as Halcyon) units via the three-wire (RWB) bus.

> [!WARNING]
> **Breaking change to the configuration.** Upgrading an existing configuration:
> - Feature entities (filter, louvers, zones, use sensor, remote temperature) are no longer created automatically. Declare the ones you want in the `climate:` block, see [Home Assistant entities](#home-assistant-entities), or they disappear from Home Assistant.
> - `supported_features` is now the list of capabilities to assume when the unit does not report its own, which is what drives the climate controls. It replaces the `filter_timer`, `sensor_switching`, `maintenance` and `economy_mode` booleans.

> [!IMPORTANT]
> Requires ESPHome 2026.3.0 or newer.

## Basic configuration

```yaml
substitutions:
  device_name: halcyon-controller
  friendly_name: Halcyon Controller
  device_description: Atom Lite + FOSV
  esp_board: m5stack-atom

external_components:
#  - source: github://Omniflux/esphome-tzsp
  - source: github://Omniflux/esphome-fujitsu-halcyon

# Add wifi_ssid and wifi_password to your secrets.yaml
wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password

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

logger:

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

  # Fujitsu devices use 0 and 1, but 2-15 should also work. Must not skip addresses.
  # Use 0 if this ESP is the only controller on the bus. Use 1 (or the next free
  # address) if a wired wall controller is installed, since it takes 0.
  controller_address: 1  # 0=Primary, 1=Secondary
  #temperature_controller_address: 0  # Fujitsu controller address to read temperature from

  #temperature_sensor_id: my_temperature_sensor  # ESPHome sensor to read temperature from
  #humidity_sensor: my_humidity_sensor  # ESPHome sensor to read humidity from

  #ignore_lock: true  # Ignore child/part/feature lock set on unit or primary/central remote control

  # Optional feature entities. Uncomment the ones your unit reports (check the
  # Supported Features sensor). Each key uses a default name. Add `name: "..."`
  # under a key to customize it. A feature the unit does not have logs a warning
  # at startup and the entity is created but has no effect. Zones have their own section below.
  #filter_timer_expired:
  #reset_filter_timer:
  #advance_vertical_louver:
  #advance_horizontal_louver:
  #use_sensor:      # also needs temperature_sensor_id and unit function settings 42 and 48
  #remote_sensor:   # needs another controller on the bus, see temperature_controller_address

  # To capture communications for debugging / analysis
  # Use Wireshark with https://github.com/Omniflux/fujitsu-airstage-h-dissector
  #tzsp:
  #  ip: 192.168.1.20
  #  protocol: 255
```

This basic configuration exposes the climate control and the core diagnostics. To add the optional feature entities (filter, louvers, zones, use sensor, remote temperature), declare them as shown in [Home Assistant entities](#home-assistant-entities).

## Reporting temperature and humidity from Home Assistant

You can use ESPHome (or Home Assistant) sensors to report the current temperature and humidity to the Home Assistant climate component.

```yaml
sensor:
  - platform: homeassistant # https://esphome.io/components/sensor/homeassistant.html
    id: my_temperature_sensor
    entity_id: sensor.my_temperature_sensor  # Home Assistant entity_id
    # Sensor value must be °C. Uncomment the filter below if your source is Fahrenheit.
    #filters:
    #  - lambda: return fahrenheit_to_celsius(x);

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

If your unit supports sensor switching and has the function settings configured appropriately (see your installation manual, usually settings `42` and `48`), it can also be set to use this sensor instead of the sensor in its air intake. Declare the `use_sensor` entity (see below) to expose that switch. The switch remembers its state across reboots.

The unit is only told to use the external sensor while a valid reading is available. If the sensor becomes unavailable, or no reading arrives for `sensor_timeout` (default `5min`), the unit is switched back to its own sensor and a warning is logged. As soon as readings resume, the external sensor is handed back automatically. The Use Sensor switch keeps its state in Home Assistant during this time, it reflects your intent. Set `sensor_timeout: 0s` to disable the check and keep sending the last reading.

```yaml
climate:
  - platform: fujitsu-halcyon
    name: None
    controller_address: 1
    temperature_sensor_id: my_temperature_sensor
    sensor_timeout: 5min  # Optional, default 5min. 0s disables.
    use_sensor:
      # restore_mode: RESTORE_DEFAULT_ON  # start ON at first boot, default RESTORE_DEFAULT_OFF
```

## Unit capabilities

The component needs to know what your indoor unit supports (which modes, fan speeds, swing directions, and options such as economy, filter timer, sensor switching, and zones) so it can show the right climate controls and validate the entities you declare. By default it asks the unit directly and uses the answer, so **most users need nothing here**.

That probe is `autoconf: true`, the default. One unit is known to enter a non-recoverable error state when probed, so for such a unit set `autoconf: false` to skip it. A unit that refuses the request, by answering with an empty error, is handled automatically: the component logs a warning naming this option, falls back to its in-code defaults, and finishes starting up. A unit that simply does not answer falls back the same way, without a warning.

If your unit does not answer and the defaults are wrong for it, state its capabilities in YAML so the controls and the declared entities behave correctly. Anything not specified keeps its default value.

```yaml
climate:
  - platform: fujitsu-halcyon
    name: None
    controller_address: 0

    # Skip the FeatureRequest probe entirely. Use this only for units known to
    # enter a non-recoverable error state when probed. Optional, default true.
    autoconf: false

    # Capabilities. Used only when the unit does not report a Features packet.
    # The words are the ones the Supported Features sensor prints.
    supported_modes:       [AUTO, COOL, HEAT, DRY, FAN]     # any subset
    supported_fan_modes:   [AUTO, QUIET, LOW, MEDIUM, HIGH] # any subset
    supported_swing_modes: [VERTICAL]                       # VERTICAL / HORIZONTAL / BOTH
    supported_features:    [FILTER_TIMER, SENSOR_SWITCHING, MAINTENANCE, ECONOMY]  # any subset
```

Behavior matrix:

| `autoconf` | IU sends `Features` | Result |
|---|---|---|
| `true` (default) | yes | IU's reported `Features` wins |
| `true` | no | YAML overrides applied on top of `DefaultFeatures` |
| `false` | (not probed) | YAML overrides applied on top of `DefaultFeatures` |

`IU` is the indoor unit. `DefaultFeatures` is the component's built-in fallback capability set, used when the unit does not report its own.

> [!NOTE]
> These lists state what the unit supports. They do not create entities, see [Home Assistant entities](#home-assistant-entities) for that. Zones cannot be stated here, zone support has to come from the unit itself, so keep `autoconf` on if you use zones. If you declare an entity whose capability is neither detected nor stated here, the component logs a warning at startup and the entity is created but has no effect.

## Home Assistant entities

Core entities (the climate control, the diagnostics, and the function controls) are created automatically. The feature entities are opt-in: declare the ones your unit has and only those are created. They are shown commented in the [Basic configuration](#basic-configuration) example above, and zones have [their own section](#zones). An entity you do not declare simply does not exist.

Not sure what your unit has? Flash the basic configuration first and read the **Supported Features** diagnostic sensor. It lists exactly what the indoor unit reports. Then declare the matching entities.

Each name in that sensor maps to the entities you declare. Some features expose more than one entity, and the entity keys are named for what they do rather than after the reported feature.

| Reported in Supported Features | `supported_features` entry (only if the unit does not report Features) | Entities to declare |
|---|---|---|
| Sensor Switching | `SENSOR_SWITCHING` | `use_sensor` |
| Filter Timer | `FILTER_TIMER` | `filter_timer_expired`, `reset_filter_timer` |
| Vertical Louvers | | `advance_vertical_louver` |
| Horizontal Louvers | | `advance_horizontal_louver` |
| Zones | | `zone_1` to `zone_8`, `zone_group_day`, `zone_group_night` |
| Economy | `ECONOMY` | Eco preset on the climate entity, no separate entity |
| Maintenance | `MAINTENANCE` | diagnostic only, no entity |
| Not reported, needs another wall controller on the bus | | `remote_sensor` (see `temperature_controller_address`) |

If you declare a feature entity that the indoor unit does not actually report, the component logs a warning once at startup (for example, `zone_* declared but this unit does not report zone support`). The entity is still created, but it will not reflect or control that unsupported feature. Declaring `use_sensor` without `temperature_sensor_id` is the one case that fails validation instead, since the switch would have no temperature to give the unit.

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
| Error Code | Text sensor | Enabled | Fault code in `AA BB.CCC` (unit address + error code + extended error code) |
| Initialization Stage | Text sensor | Enabled | Current initialization progress with a label, for example `Complete (7/7)` |
| Supported Features | Text sensor | Enabled | List of features reported by the indoor unit, published once at initialization. Example: `Mode: Auto Heat Cool Dry Fan \| Fan: Auto High Medium Low Quiet \| Economy \| Sensor Switching \| Vertical Louvers \| Horizontal Louvers \| Zones` |
| Remote Temperature Sensor | Sensor | If declared | Temperature reported by another controller on the bus (see `temperature_controller_address`) |
| Filter Timer Expired | Binary sensor | If declared | Set when the filter maintenance timer has elapsed |

### Configuration
| Entity | Type | Default | Description |
|--------|------|---------|-------------|
| Use Sensor | Switch | If declared | Route the external temperature sensor reading to the indoor unit (requires unit support and `temperature_sensor_id` configured, see settings `42` and `48`). Restored across reboots. Falls back to the unit's own sensor while no valid reading is available, see `sensor_timeout` |
| Reset Filter Timer | Button | If declared | Reset the filter maintenance timer |
| Advance Vertical Louver | Button | If declared | Step the vertical louver to the next position |
| Advance Horizontal Louver | Button | If declared | Step the horizontal louver to the next position |
| Reinitialize | Button | Enabled | Re-run the initialization sequence without rebooting. This also happens automatically, see `init_timeout` under [Troubleshooting](#initialization-does-not-complete) |
| Function / Function Value / Function Unit | Number | Enabled | Raw function register access |
| Function Read / Function Write | Button | Enabled / Disabled | Trigger a function register read or write |
| Zone `#` | Switch | If declared | Enable/Disable zone `#` |
| Zone Group Day | Switch | If declared | Enable/Disable zone group Day |
| Zone Group Night | Switch | If declared | Enable/Disable zone group Night |

## Zones

For ducted or zoned units, declare the zones you have and, optionally, the day and night groups. Zone support and the set of enabled zones are read from the indoor unit, so this requires `autoconf` to stay on. There is no capability key for zones, so do not set `autoconf: false` if you use zones. The zone switches always show the state reported by the unit, they are not restored from the ESP's memory after a reboot.

```yaml
climate:
  - platform: fujitsu-halcyon
    name: None
    controller_address: 0

    zone_1:
    zone_2:
    # zone_3 through zone_8 as needed. Add `name: "..."` under a key to customize.
    zone_group_day:
    zone_group_night:
```

## Troubleshooting

View the ESPHome log for the device.

### Verify receiving data

```text
RX: 00 A0 XX XX XX XX XX XX
```

If there are no receive lines in the log, verify your UART I/O pins are correctly configured, and your remote control wires are securely connected.

```yaml
uart:
  tx_pin: GPIO??
  rx_pin: GPIO??
```

### Verify transmitting data

```text
TX: XX XX XX XX XX XX XX XX
```

If there are no transmit lines in the log, this component is not receiving the token allowing it to transmit. The component logs a warning to this effect after about 15 seconds without a token: `Receiving data but no transmit token after 15 s`.

Ensure `controller_address` is configured correctly and, if `controller_address` > `0`, this component is powered on before (or at least simultaneously with) the preceding controllers. Secondary controllers only get one chance to register for the token when the primary (or preceding) controller powers on.

You may want to temporarily disconnect the OEM remote controls and connect only this component with `controller_address: 0` to test without the registration window restriction.

### Initialization does not complete

The **Initialization Stage** sensor shows where the sequence is. It normally reaches `Complete (7/7)` within a few seconds of the first received packet. A missed packet during startup, or a power glitch that reboots the ESP in the middle of the sequence, can leave it stuck at an earlier stage, with **Supported Features** empty and no control from Home Assistant.

The component watches for this. If initialization has not completed after `init_timeout` (default `30s`), and the unit has answered at least once, it restarts the sequence on its own, exactly as the **Reinitialize** button does, and logs a warning:

```text
Initialization stuck at 'Waiting for features (2/7)' for 30 s, restarting the sequence (attempt 1)
```

After three attempts the message drops to debug level so it does not flood the log. If the restarts never succeed, the cause is usually the missing transmit token described above, not the initialization itself. Set `init_timeout: 0s` to disable the watchdog.

```yaml
climate:
  - platform: fujitsu-halcyon
    name: None
    controller_address: 0
    init_timeout: 30s  # Optional, default 30s. 0s disables.
```

### OEM controller displays an error

Ensure `controller_address` is configured correctly. Each address must be unique in a system. If you already have a hardwired OEM controller connected, it will be configured as address `0`. If you have two, they will be addresses `0` and `1`. This component must be configured as the next available address.

Ensure `tx_pin` is configured correctly. If it is not, another component on the ESP device could be transmitting on the remote control bus, disrupting normal communications.

## Debugging / Examining protocol

Configure TZSP and use Wireshark with [fujitsu-airstage-h-dissector](https://github.com/Omniflux/fujitsu-airstage-h-dissector) to debug / decode the Fujitsu serial protocol.

## Related projects
- FOSV's [Fuji-Atom-Interface](https://github.com/FOSV/Fuji-Atom-Interface) - Open hardware interface compatible with this component
- AndrewBoy's [Fujitsu-AC-3-Wire-for-ESPHome-with-MCP2021](https://github.com/AndrewBoyHUN/AndrewBoys-Fujitsu-AC-3-Wire-for-ESPHome-with-MCP2021) - Open hardware interface compatible with this component
- Sam Jam Sam's [Esphome-Fujitsu-Heat-Pump](https://github.com/sam-jam-sam/Esphome-Fujitsu-Heat-Pump) - Open hardware interface compatible with this component
<!-- -->
- My [esphome-fujitsu-dmmum](https://github.com/Omniflux/esphome-fujitsu-dmmum) - Fujitsu AirStage-H 3-wire Central Controller component for ESPHome
- My [fujitsu-airstage-h-dissector](https://github.com/Omniflux/fujitsu-airstage-h-dissector) - Wireshark dissector to debug / decode the Fujitsu serial protocol.
<!-- -->
- Aaron Zhang's [esphome-fujitsu](https://github.com/FujiHeatPump/esphome-fujitsu)
- Jaroslaw Przybylowicz's [fuji-iot](https://github.com/jaroslawprzybylowicz/fuji-iot)
- Raal Goff's [FujiHeatPump](https://github.com/unreality/FujiHeatPump)
- Raal Goff's [FujiHK](https://github.com/unreality/FujiHK)
<!-- -->
- Myles Eftos's [Reverse engineering](https://hackaday.io/project/19473-reverse-engineering-a-fujitsu-air-conditioner-unit)
- Home Assistant [thread](https://community.home-assistant.io/t/fujitsu-ac-heat-pump-integration-via-esphome-esp32/)
<!-- -->
- Sergek's [AC-fujitsu-General-EZ-0001HSEFR-integration](https://github.com/thaserge-primary/AC-fujitsu-General-EZ-0001HSEFR-integration) - For older Fujitsu hardware
- Benas Ragauskas' [FujitsuAC](https://github.com/Benas09/FujitsuAC) - For newer Fujitsu hardware
