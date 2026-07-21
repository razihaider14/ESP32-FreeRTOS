# ESP32-FreeRTOS

A demonstration project showcasing how to build a multi-tasking ESP32 application using FreeRTOS, MQTT, OTA updates, and RFID access control.

This project was built using the Arduino framework and is intended as a practical example of organizing an ESP32 application across both cores while leveraging FreeRTOS tasks.

## Features

* Dual-core FreeRTOS architecture
* WiFi connectivity
* MQTT publish/subscribe support
* MQTT-triggered OTA updates
* RFID-based access control using MFRC522
* Dynamic task creation
* Task priorities and task handles
* Mutex-protected serial output
* Periodic MQTT heartbeat messages

## Architecture

### Core 0

* RFID Task
* OTA Task (created only when requested)

### Core 1

* WiFi Task
* MQTT Task

## Hardware

| Component           | Quantity |
| ------------------- | -------: |
| ESP32 DevKit V1     |        1 |
| MFRC522 RFID Module |        1 |
| RFID Card/Tag       |        1 |
| LED (optional)      |        1 |
| 220Ω Resistor       |        1 |

> Note: Many ESP32 development boards expose the onboard LED on GPIO 2, allowing this project to run without an external LED.

## Wiring

### MFRC522

| MFRC522  | ESP32         |
| -------- | ------------- |
| VCC      | 3.3V          |
| GND      | GND           |
| SDA (SS) | GPIO 5        |
| SCK      | GPIO 18       |
| MOSI     | GPIO 23       |
| MISO     | GPIO 19       |
| RST      | GPIO 22       |
| IRQ      | Not Connected |

### LED

| LED         | ESP32      |
| ----------- | ---------- |
| Anode (+)   | GPIO 2     |
| Cathode (-) | 220Ω → GND |

## MQTT Topics

| Topic             | Payload | Description                                          |
| ----------------- | ------- | ---------------------------------------------------- |
| `esp32/heartbeat` | `Alive` | Published every 5 seconds                            |
| `esp32/ota`       | `ON`    | Creates the OTA task and waits for a firmware upload |

## OTA Configuration

| Setting  | Value        |
| -------- | ------------ |
| Hostname | `ESP32-RTOS` |
| Password | `1234`       |

To trigger OTA:

1. Publish `ON` to `esp32/ota`.
2. Wait for the OTA task to start.
3. Select the ESP32 network port in Arduino IDE.
4. Upload the new firmware.
5. The ESP32 will reboot automatically after a successful update.

## RFID Access Control

The project uses a hardcoded UID for demonstration purposes.

```cpp
byte allowedUID[4] = {0x5C, 0xB8, 0x3B, 0x05};
```

When the authorized card is scanned:

* The LED turns ON.
* Access is granted for 3 seconds.
* The LED turns OFF automatically.

Unauthorized cards are rejected.

## Libraries

Install the following libraries from the Arduino Library Manager:

* PubSubClient
* MFRC522

The following libraries are included with the ESP32 Arduino core:

* WiFi
* ArduinoOTA
* SPI
* FreeRTOS

## Why FreeRTOS?

This project intentionally avoids placing all functionality inside `loop()`.

Instead, each responsibility is isolated into its own task:

* Networking
* Messaging
* RFID handling
* OTA updates

This approach makes the application easier to scale and demonstrates how FreeRTOS can be used to structure real-world ESP32 firmware.

## License

This project is licensed under the MIT License. See the `LICENSE` file for more information.
