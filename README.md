# Fujitsu AirStage-H component for ESPHome

An ESPHome component to control Fujitsu AirStage-H (product line previously known as Halcyon) units via the three wire (RWB) bus.

```yaml

substitutions:
  name: halcyon-controller
  friendly_name: Halcyon Controller
  halcyon_name: My AC
  esp_board: esp32-poe-iso

packages:
  wifi: !include common/wifi.yaml

external_components:
  - source: github://Omniflux/esphome-fujitsu-halcyon
  - source: github://Omniflux/esphome-tzsp

esphome:
  name: ${name}
  friendly_name: ${friendly_name}

esp32:
  board: ${esp_board}
  framework:
    type: esp-idf

api:

ota:
  password: !secret ota_password

sensor:
- platform: uptime
  name: "Uptime"

button:
- platform: restart
  name: Restart device
- platform: safe_mode
  name: Safe Mode Boot

#logger:
#  level: DEBUG

uart:
  tx_pin: GPIO13  # Device dependent
  rx_pin: GPIO14  # Device dependent
  baud_rate: 500
  parity: EVEN

climate:
- platform: fujitsu-halcyon
  name: ${halcyon_name}
  # Fujitsu devices use 0 and 1, but 2-15 should also work. Must not skip addresses
  controller_address: 1  # 0=Primary, 1=Secondary
  #temperature_controller_address: 0  # Fujitsu controller address to read temperature from
  #temperature_sensor_id: my_temp_sensor  # ESPHome sensor to read temperature from
  #ignore_lock: true  # Ignore feature lock

  # To capture communications for debugging / analysis
  # Use Wireshark with https://github.com/Omniflux/fujitsu-airstage-h-dissector
#  tzsp:
#    ip: 192.168.1.20
#    protocol: 255

```

Configure TZSP and use Wireshark with [fujitsu-airstage-h-dissector](https://github.com/Omniflux/fujitsu-airstage-h-dissector) to debug / decode the Fujitsu serial protocol.

## Related projects
- FOSV's [Fuji-Atom-Interface](https://github.com/FOSV/Fuji-Atom-Interface)
<!-- -->
- Aaron Zhang's [esphome-fujitsu](https://github.com/FujiHeatPump/esphome-fujitsu)
- Jaroslaw Przybylowicz's [fuji-iot](https://github.com/jaroslawprzybylowicz/fuji-iot)
- Raal Goff's [FujiHeatPump](https://github.com/unreality/FujiHeatPump)
- Raal Goff's [FujiHK](https://github.com/unreality/FujiHK)
<!-- -->
- Myles Eftos's [Reverse engineering](https://hackaday.io/project/19473-reverse-engineering-a-fujitsu-air-conditioner-unit)