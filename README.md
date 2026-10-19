# Smart Vehicle

Arduino Mega vehicle with a compass, wheel encoders, LIDAR, and an RGB line sensor. An ESP8266 serves a small web UI and forwards commands over UART. The Mega drives the motors.

The program to read and flash is `firmware/`. `archive/` is the coursework it came from.

## Flash

Requires [PlatformIO](https://platformio.org/) (`pip install platformio`).

```bash
cp firmware/esp8266/include/secrets.example.h firmware/esp8266/include/secrets.h
# edit secrets.h: Wi-Fi name, Wi-Fi password, web username, web password

pio run -d firmware/mega -t upload
pio run -d firmware/esp8266 -t upload
pio run -d firmware/esp8266 -t uploadfs
```

Unplug the Mega UART wires before flashing the ESP. `Serial` on the ESP is the link to the Mega, so a connected Mega holds the port.

Open `http://<esp-ip>/` and log in. The address is printed on the ESP's `Serial1` (GPIO2) at 9600. If the module is a NodeMCU or Wemos D1 Mini, set `board` in `firmware/esp8266/platformio.ini` to `nodemcuv2` or `d1_mini` before uploading.

Host tests, no hardware:

```bash
make -C tests test
```

## Wiring

| Mega | ESP8266 |
| --- | --- |
| Serial2 TX, pin 16 | RX, GPIO3, through a level shifter |
| Serial2 RX, pin 17 | TX, GPIO1 |
| GND | GND |

Both UARTs are 9600. Mega TX is 5 V; do not feed it straight into the ESP RX pin. ESP TX at 3.3 V is fine into the Mega.

Pins for the motors, encoders, LCD, and joystick button are in `firmware/mega/include/config.hpp`. The matching drawing is `schematics/mega/mega_schematic.jpg`.

## Using it

1. Power the vehicle and wait until the LCD says `Color 0/3`. The compass offset is taken once at boot, so that heading becomes 0.
2. Put the RGB sensor on red, green, then blue. Press the joystick button once on each color. The LCD should reach `Ready`.
3. Point the vehicle at a clear 20 cm of LIDAR range and press **Calibrate encoder**. The scale is stored in EEPROM and rejected if the encoders report no pulses.
4. **Drive** moves a distance in centimeters. The compass gauge sends `turn:<degrees>`. **Go** starts the maze. **Stop** cuts the motors.

Closing the page logs out. Command routes refuse a request that does not carry the session cookie.

## Command contract

Mega Serial2, one line each. The same parser is in `firmware/lib/vehicle_core` and is what `make -C tests test` checks.

| Direction | Line |
| --- | --- |
| ESP to Mega | `maze`, `calibrate`, `stop`, `turn:<0-360>`, `move:<cm>` |
| Mega to ESP | `LIDAR:<cm>`, `COMP:<deg>`, `RGB:<color>`, `ENC:<mm/pulse or none>`, `ERR:compass` |

A missing compass is `ERR:compass`. It is not reported as `COMP:0`.

## Layout

| Path | What it is |
| --- | --- |
| `firmware/lib/vehicle_core` | Heading, color match, encoder scale, UART lines. No Arduino headers. |
| `firmware/mega` | Sensors, motors, LCD, maze. |
| `firmware/esp8266` | Wi-Fi, session cookie, LittleFS page. |
| `tests/` | Host tests for the shared logic. |
| `schematics/` | Mega and Nano drawings. |
| `archive/` | Weekly labs and the presentation snapshot. Not the program to flash. |

## Parts

- Arduino Mega 2560 Rev3
- 20x4 character LCD
- Joystick module, used here as the color-calibration button
- Pololu DRV8835 dual motor driver
- Two Lego NXT motors
- CMPS14 compass at I2C `0x60`
- Garmin LIDAR-Lite
- TCS34725 RGB sensor
- ESP8266

PlatformIO fetches LIDARLite, Adafruit TCS34725, Adafruit BusIO, LiquidCrystal, and ArduinoJson. `Wire` and `EEPROM` come with the Mega core.

## Limits

- Analog joystick driving from the lab sketches is not in `firmware/mega`. Manual driving is the web page.
- Maze headings are relative to the boot compass offset. Turns give up after 8 s, or after 1.5 s with no heading change. Encoder calibration gives up after 15 s.
- The web UI is HTTP on a private network. One login is active; the next login replaces it.
- Check the motor nets on the schematic against the DRV8835 before rewiring.
- Old lab sketches had Wi-Fi passwords in the tree. Those literals are removed. If this repository was already pushed, rotate those passwords; they can still be in git history.
