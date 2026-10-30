# Third-party notices

The idkOS firmware binary includes the following third-party software. Each
component is used unmodified, under its own license. Full license texts are in
[`licenses/`](licenses/). idkOS's own code is under the [MIT License](LICENSE).

| Component | Version | License | License text |
|---|---|---|---|
| [ESP-IDF](https://github.com/espressif/esp-idf) | 4.4.7 | Apache-2.0 | [ESP-IDF-Apache-2.0.txt](licenses/ESP-IDF-Apache-2.0.txt) |
| [Arduino core for ESP32](https://github.com/espressif/arduino-esp32) | 2.0.17 (PlatformIO espressif32 7.1.3) | LGPL-2.1 | [arduino-esp32-LGPL-2.1.txt](licenses/arduino-esp32-LGPL-2.1.txt) |
| [LibSSH-ESP32](https://github.com/ewpa/LibSSH-ESP32) (libssh) | 5.10.0 | LGPL-2.1; parts BSD-2-Clause | [LibSSH-ESP32-LGPL-2.1.txt](licenses/LibSSH-ESP32-LGPL-2.1.txt), [LibSSH-ESP32-BSD.txt](licenses/LibSSH-ESP32-BSD.txt) |
| [M5Cardputer](https://github.com/m5stack/M5Cardputer) | 1.1.1 | MIT (SPDX header in source; © 2025 M5Stack Technology Co., Ltd.) | — |
| [M5Unified](https://github.com/m5stack/M5Unified) | 0.2.25 | MIT | [M5Unified-LICENSE.txt](licenses/M5Unified-LICENSE.txt) |
| [M5GFX](https://github.com/m5stack/M5GFX) | 0.2.32 | MIT | [M5GFX-LICENSE.txt](licenses/M5GFX-LICENSE.txt) |
| [ArduinoJson](https://github.com/bblanchon/ArduinoJson) | 6.21.6 | MIT | [ArduinoJson-LICENSE.txt](licenses/ArduinoJson-LICENSE.txt) |
| [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) (M5Cardputer dependency) | as resolved by PlatformIO | MIT | [IRremote-LICENSE.txt](licenses/IRremote-LICENSE.txt) |

## LGPL-2.1 components

The Arduino core for ESP32 and LibSSH-ESP32 are licensed under the GNU Lesser
General Public License 2.1 and are statically linked into the firmware. Their
complete source code is available from the upstream links above, at the
versions listed. The complete idkOS source code is in this repository and
builds with PlatformIO (`pio run`), so you can modify those libraries,
or replace them with other versions, and relink the firmware.

## Data and services

- Weather data by [Open-Meteo.com](https://open-meteo.com/) (CC BY 4.0) and the US
  [National Weather Service](https://www.weather.gov/). Approximate location by [ipapi.co](https://ipapi.co/).
- Crypto data provided by [CoinGecko](https://www.coingecko.com/).


`data/cert/x509_crt_bundle.bin` holds public root CA certificates taken from a
standard system trust store (Mozilla CA list) so that HTTPS can be verified.
It contains no private keys.
