# ESP32-FreeRTOS

A collection of ESP32 FreeRTOS examples built around a simple but practical idea:

> Keep networking on one core, perform OTA updates on the other, and leave the main loop free for application logic.

This repository started as an experiment with MQTT and RFID on the ESP32, and gradually grew into a comparison of different OTA mechanisms. Every example uses the same general architecture so you can focus on understanding how each OTA method works without having to relearn the rest of the project.

## What's Included

| Example                  | Description                                                                                                           |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------- |
| `ESP32-FreeRTOS-MQTT`    | MQTT-based project using ArduinoOTA. Trigger OTA via an MQTT topic and upload firmware directly from the Arduino IDE. |
| `ESP32-FreeRTOS-HttpOTA` | HTTP OTA implementation. The ESP32 downloads `firmware.bin` from a server (for example, `python -m http.server`).     |
| `ESP32-FreeRTOS-WebOTA`  | Web OTA implementation. The ESP32 hosts a web page where you can upload a firmware binary from your browser.          |
| `ESP32-FreeRTOS-UDP`     | UDP-triggered ArduinoOTA example. Sends a UDP packet to enable OTA mode on the device.                                |

## Project Architecture

All examples follow roughly the same layout:

```text
Core 0
------
OTA Task

Core 1
------
Networking Task
(MQTT / UDP)

loop()
------
Wi-Fi Service
RFID Service
LED Service
```

The idea is simple:

* Networking runs independently on Core 1.
* OTA updates are executed as dedicated FreeRTOS tasks on Core 0.
* RFID and LED logic remain in the main loop.
* Serial output is protected with a mutex.
* OTA functionality is enabled only when explicitly requested.

## Hardware Used

* ESP32-WROOM-32
* MFRC522 RFID Module
* RFID Card/Tag
* LED
* Jumper Wires
* Breadboard

## OTA Methods

### 1. ArduinoOTA

```text
Arduino IDE
     |
     v
ESP32
```

Upload firmware directly from the Arduino IDE once OTA mode is enabled.

---

### 2. HTTP OTA

```text
ESP32
   |
HTTP GET
   |
firmware.bin
   |
HTTP Server
```

The ESP32 acts as an HTTP client and downloads the firmware from a server.

For testing, a simple Python server works perfectly:

```bash
python -m http.server 8000
```

---

### 3. Web OTA

```text
Browser
   |
HTTP POST
   |
ESP32
```

The ESP32 hosts a small web page that accepts a firmware upload.

---

### 4. UDP OTA Trigger

```text
PC
   |
UDP Packet
   |
ESP32
```

Send a UDP packet containing `ota` to place the ESP32 into OTA mode.

## Exporting Firmware

Arduino IDE:

```text
Sketch
    ->
Export Compiled Binary
```

The IDE generates several files. The only one used for OTA is:

```text
YourSketch.ino.bin
```

For HTTP OTA examples, rename it to:

```text
firmware.bin
```

and serve it using:

```bash
python -m http.server 8000
```

## Repository Structure

```text
ESP32-FreeRTOS/
|
+-- ESP32-FreeRTOS-MQTT/
|   +-- ESP32-FreeRTOS-MQTT.ino
|
+-- ESP32-FreeRTOS-HttpOTA/
|   +-- ESP32-FreeRTOS-HttpOTA.ino
|
+-- ESP32-FreeRTOS-WebOTA/
|   +-- ESP32-FreeRTOS-WebOTA.ino
|
+-- ESP32-FreeRTOS-UDP/
|   +-- ESP32-FreeRTOS-UDP.ino
|
+-- README.md
+-- LICENSE
```

## Why This Repository Exists

Most ESP32 OTA tutorials focus on a single implementation. In practice, you'll eventually encounter several:

* ArduinoOTA
* HTTP/HTTPS OTA
* Web OTA
* UDP-triggered OTA
* MQTT-triggered OTA

This repository puts them side by side using a consistent FreeRTOS design, making it easier to compare approaches and understand the trade-offs between them.

## License

This project is licensed under the MIT License.
