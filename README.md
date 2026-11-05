# minimon

A small time and weather monitor on an ESP32 and a 16x2 character LCD. It cycles between three screens: the current time and date, live outdoor weather from the Slovenian Environment Agency (ARSO), and indoor temperature and humidity from a DHT11 sensor. The interface is in Slovenian.

## Features

- **Clock and date** synced over NTP, with a Wi-Fi status icon (connected / disconnected)
- **Outdoor weather** from an ARSO XML feed: temperature, humidity, wind direction and wind speed
- **Indoor climate** from a DHT11 sensor: temperature and humidity
- **Auto-rotating screens**, switching every 10 seconds

## Screens

| # | Screen  | Example                                  |
|---|---------|------------------------------------------|
| 0 | Time    | `    14:32:07   ` / `  5. nov 2026   ` |
| 1 | Outdoor | `7.3°C        SV` / `82%      2.1m/s`   |
| 2 | Indoor  | `V sobi: 22.5°C` / `        41.0%`      |

## Hardware

- ESP32 development board
- 16x2 LCD (HD44780-compatible, 4-bit mode)
- DHT11 temperature and humidity sensor
- Potentiometer (about 10 kΩ) for LCD contrast
- Breadboard and jumper wires

### Wiring

| LCD pin | ESP32 GPIO |
|---------|------------|
| RS      | 5          |
| E       | 7          |
| D4      | 9          |
| D5      | 11         |
| D6      | 12         |
| D7      | 16         |

| DHT11 pin | ESP32 GPIO |
|-----------|------------|
| DATA      | 15         |

Connect the LCD's VSS, RW and K to GND, and VDD and A to 5 V (use a resistor on A if your module has none built in). Connect V0 to the potentiometer wiper for contrast. Power the DHT11 from 3.3 V or 5 V and GND.

> **Note:** On the classic ESP32, GPIO 6 to 11 are connected to the internal flash and can't be used for other purposes. The pin numbers above suit boards such as the ESP32-S2 or ESP32-S3. If you're using a different board, change the pins in the code to ones that are available on it:
>
> ```cpp
> LiquidCrystal lcd(5, 7, 9, 11, 12, 16); // RS, E, D4, D5, D6, D7
> #define DHTPIN 15
> ```

## Software requirements

- [Arduino IDE](https://www.arduino.cc/en/software) with the [ESP32 board package](https://github.com/espressif/arduino-esp32)
- Libraries:
  - `LiquidCrystal` (bundled with the Arduino IDE)
  - `DHT sensor library` by Adafruit
  - `Adafruit Unified Sensor` (a dependency of the DHT library)
  - `WiFi`, `HTTPClient` and `time.h` (included with the ESP32 core)

## Configuration

Edit these values at the top of the sketch before uploading:

```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const char* time_zone = "YOUR_TIME_ZONE";
const char* meteoUrl  = "METEO_ARSO_URL.xml";
```

### Time zone

`time_zone` takes a POSIX TZ string. For Slovenia (Central European Time with daylight saving):

```cpp
const char* time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";
```

### ARSO weather URL

`meteoUrl` should point to the XML observation feed for your nearest weather station on [meteo.arso.gov.si](https://meteo.arso.gov.si/). The code reads these tags from the feed:

| Tag              | Meaning                    |
|------------------|----------------------------|
| `<t>`            | Temperature (°C)           |
| `<rh>`           | Relative humidity (%)      |
| `<dd_shortText>` | Wind direction (short text)|
| `<ff_val>`       | Wind speed (m/s)           |

## Getting started

1. Wire up the hardware as described above.
2. Install the required libraries.
3. Open the sketch in the Arduino IDE and fill in the configuration values.
4. Select your ESP32 board and port, then upload.
5. The LCD will show the Wi-Fi connection status, then start cycling through the screens.

## How it works

The `loop()` function runs a set of timers based on `millis()`:

| Task                     | Interval   |
|--------------------------|------------|
| Reconnect Wi-Fi if down  | after 60 s |
| Fetch outdoor weather    | 10 min     |
| Read DHT11 sensor        | 10 s       |
| Switch screen            | 10 s       |
| Redraw display           | 200 ms     |

Weather data is fetched with `HTTPClient`, and the values are pulled out of the XML response with a small string helper (`extractString`) rather than a full XML parser, which keeps memory use low. If a DHT11 read fails, the last good values stay on screen.

## License

This project is licensed under the [MIT License](LICENSE).
