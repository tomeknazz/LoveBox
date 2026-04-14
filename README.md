# LoveBox - Smart E-Ink Messaging Display
LoveBox is a fully autonomous, ultra-low-power IoT device based on the ESP32-C3 microcontroller. It serves as a dedicated smart display for two-way communication and personalized widgets (weather, calendar, countdowns) using an E-Ink screen. The project is heavily optimized for energy efficiency, allowing it to run for up to **3 months** on a single 18650 Li-ion battery.
# Key Features
- **Two-way Telegram Communication:** Integrates with the Universal Telegram Bot API to receive messages directly on the screen and configure the device via chat.
- **E-Paper Display (4.2"):** Utilizes a 400x300 (Black/White/Red) matrix that consumes power only during screen refreshes.
- **Advanced Power Management:** - Implements Deep Sleep mode (waking up every 30 mins or via external interrupt). Features the professional TPS63802 Buck-Boost converter for high efficiency across the entire battery voltage range.
- **Interactivity:** A hidden TTP223 capacitive touch sensor under the housing for manual wake-ups and sending feedback notifications to the sender.Dynamic Widgets: Switch seamlessly between weather (OpenWeatherMap API), a monthly calendar, and a custom event countdown.
- **Remote WiFi Configuration:** Built-in state machine allowing WiFi network changes directly from the chat without the need to re-flash the device.
# Technical Specifications
## Hardware
- **Microcontroller:** ESP32-C3
- **Screen:** E-Ink 4.2" (400x300 px) BWR - SSD1683 driver
- **Power:** 18650 Li-ion Battery (3400 mAh). **Charging IC:** TP4056 with DW01A battery protection. **Voltage Regulator:** TPS63802 (Buck-Boost, High-Efficiency, low Iq)
- **Sensor:** TTP223 (Capacitive Touch)
## Software / Libraries
- GxEPD2 - E-paper display driver
- U8g2_for_Adafruit_GFX - Font rendering (supports special characters)
- UniversalTelegramBot - Communication with Telegram servers
- ArduinoJson - Parsing JSON payloads from OpenWeatherMap
- Preferences - Non-volatile storage (NVS) for configuration cache
# Wiring Schematic (ESP32-C3 Pinout)
- **E-Ink SCK:** GPIO 5
- **E-Ink MOSI:** GPIO 6
- **E-Ink CS:** GPIO 7
- **E-Ink DC:** GPIO 8
- **E-Ink RES:** GPIO 9
- **E-Ink BUSY:** GPIO 10
- **TTP223 I/O:** GPIO 4
# Telegram Bot Commands
The device supports dual-user authorization. Below is a list of core commands (kept in their original Polish syntax matching the source code):
- /wifi – Launches an interactive wizard to change the WiFi SSID and password.
- /widget [pogoda/kalendarz/odliczanie/pusty] – Changes the content of the bottom-right panel.
- /odliczanie [YYYY-MM-DD] [Description] – Sets the target date and description for the countdown widget.
- /pogoda1_miasto [City] – Changes the location for the main weather module.
- /naladowano – Resets the battery day counter (smart charging reminder).
- /dodaj_id [ID] – Grants system access to a secondary user (Admin command).
# Power-Saving Optimization
The project implements energy-saving strategies:
- Deep Sleep: The MCU completely powers down after 60 seconds of inactivity.
- GPIO Wakeup: The ESP32-C3 wakes up instantly upon detecting a HIGH state from the touch sensor.
- Timer Wakeup: Automatically wakes up every 30 minutes to fetch new data and refresh the screen.
- NVS Cache: The last received message, widget states, and settings are saved in flash memory, allowing the screen state to be restored flawlessly after power cycles.
# Installation & Setup
- Clone the repository.
- Insert your specific BOTtoken and CHAT_ID_BF in the source code.
- Flash the code using Arduino IDE or PlatformIO (Select ESP32C3 Dev Module).
- Configure WiFi upon first boot via the Telegram bot or hardcode it before flashing.
