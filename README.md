# ESP32-FreeRTOS

> A collection of practical ESP32 FreeRTOS examples demonstrating dual-core task scheduling, networking, OTA updates, and RFID access control using the Arduino framework.

This repository started as an exercise in learning FreeRTOS on the ESP32, but it quickly became a set of small, real-world examples showing how to split responsibilities across both cores instead of putting everything inside `loop()`.

Whether you're learning FreeRTOS tasks, experimenting with OTA, or building an IoT project with networking and RFID, these examples provide a good starting point.

---

## What's Inside?

| Example               | Description                                                                                                                 |
| --------------------- | --------------------------------------------------------------------------------------------------------------------------- |
| `ESP32-FreeRTOS-MQTT` | Uses MQTT for communication and OTA triggering. Includes WiFi, MQTT, RFID access control, and OTA updates.                  |
| `ESP32-FreeRTOS-UDP`  | Replaces MQTT with UDP while keeping OTA and RFID functionality intact. Useful for understanding lightweight communication. |

---

## Features

* Dual-core FreeRTOS architecture
* Dynamic task creation and deletion
* WiFi connectivity
* MQTT communication
* UDP communication
* MQTT/UDP-triggered OTA updates
* RFID access control using the MFRC522
* Mutex-protected serial output
* Task handles and priorities
* Practical examples of separating networking and hardware logic

---

## FreeRTOS Layout

### Core 0

* RFID Task
* OTA Task (created only when requested)

### Core 1

#### MQTT Example

* WiFi Task
* MQTT Task

#### UDP Example

* UDP Listener Task

---

## Hardware Required

| Component           | Quantity |
| ------------------- | -------: |
| ESP32 DevKit V1     |        1 |
| MFRC522 RFID Module |        1 |
| RFID Card/Tag       |        1 |
| LED (optional)      |        1 |
| 220Ω Resistor       |        1 |

> Most ESP32 development boards expose an onboard LED on GPIO 2, so an external LED is optional.

---

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

---

## MQTT Example

### Topics

| Topic             | Payload | Description             |
| ----------------- | ------- | ----------------------- |
| `esp32/heartbeat` | `Alive` | Published periodically. |
| `esp32/ota`       | `ON`    | Creates the OTA task.   |

### Workflow

1. ESP32 connects to WiFi.
2. MQTT task connects to the broker.
3. RFID task continuously scans for cards.
4. Publishing `ON` to `esp32/ota` creates the OTA task.
5. Arduino IDE uploads new firmware over the network.

---

## UDP Example

The UDP version listens on:

```cpp
4210
```

This example is useful for understanding:

* Connectionless communication
* Lightweight message passing
* UDP sockets on the ESP32
* Combining networking with FreeRTOS

---

## OTA Configuration

```cpp
Hostname: ESP32-RTOS
Password: 1234
```

### Performing an OTA Update

1. Trigger the OTA task (MQTT or UDP implementation).
2. Wait for "OTA Task Started." in the Serial Monitor.
3. Select the ESP32 network port in Arduino IDE.
4. Upload the firmware.
5. The ESP32 reboots automatically after a successful update.

---

## RFID Access Control

A hardcoded UID is used for demonstration:

```cpp
byte allowedUID[4] = {0x5C, 0xB8, 0x3B, 0x05};
```

When the correct card is scanned:

* Access is granted.
* The LED turns ON.
* The LED automatically turns OFF after a short delay.

Any other UID is rejected.

---

## Libraries

Install these libraries from the Arduino Library Manager:

* PubSubClient (MQTT example)
* MFRC522

Included with the ESP32 Arduino Core:

* WiFi
* WiFiUDP
* ArduinoOTA
* SPI
* FreeRTOS

---

## Why This Repository Exists

Most beginner ESP32 projects eventually become a giant `loop()` full of timers and `if` statements.

These examples take a different approach:

* Networking runs independently.
* RFID runs independently.
* OTA is created only when needed.
* Tasks can be pinned to different cores.
* Shared resources are protected using mutexes.

The result is firmware that's easier to read, maintain, and extend.

---

## License

Licensed under the MIT License.

See the `LICENSE` file for details.
