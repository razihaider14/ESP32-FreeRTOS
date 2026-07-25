#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN   5
#define RST_PIN  22
#define LED_PIN  2

const char* ssid     = "*****";
const char* password = "*****";

const uint16_t UDP_LISTEN_PORT = 4210;

WiFiUDP udp;
MFRC522 rfid(SS_PIN, RST_PIN);

byte allowedUID[4] = {0x5C, 0xB8, 0x3B, 0x05};

TaskHandle_t udpTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;

SemaphoreHandle_t serialMutex;

unsigned long lastWiFiAttempt = 0;
bool ipPrinted = false;
bool ledActive = false;
unsigned long ledOffAt = 0;

void safePrint(const String& msg) {
  if (serialMutex != NULL) {
    xSemaphoreTake(serialMutex, portMAX_DELAY);
    Serial.println(msg);
    xSemaphoreGive(serialMutex);
  } else {
    Serial.println(msg);
  }
}

void OTATask(void* pvParameters) {
  safePrint("OTA Task Started.");

  ArduinoOTA.setHostname("ESP32-RTOS");
  ArduinoOTA.setPassword("1234");

  ArduinoOTA.onStart([]() {
    safePrint("OTA Update Started.");
  });

  ArduinoOTA.onEnd([]() {
    safePrint("OTA Complete. Rebooting...");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    safePrint("OTA Error: " + String((int)error));
  });

  ArduinoOTA.begin();
  safePrint("OTA service is ready.");

  while (true) {
    ArduinoOTA.handle();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void UDPTask(void* pvParameters) {
  safePrint("UDP Task Started.");

  bool udpStarted = false;
  char packetBuffer[256];

  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      if (udpStarted) {
        udp.stop();
        udpStarted = false;
        safePrint("Wi-Fi lost. UDP socket closed.");
      }

      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!udpStarted) {
      if (udp.begin(UDP_LISTEN_PORT)) {
        udpStarted = true;
        safePrint("UDP listening on " + WiFi.localIP().toString() + ":" + String(UDP_LISTEN_PORT));
      } else {
        safePrint("Failed to start UDP listener.");
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    int packetSize = udp.parsePacket();

    if (packetSize > 0) {
      int len = udp.read(packetBuffer, sizeof(packetBuffer) - 1);

      if (len < 0) {
        len = 0;
      }

      packetBuffer[len] = '\0';

      String message = String(packetBuffer);
      message.trim();

      String senderIP = udp.remoteIP().toString();
      uint16_t senderPort = udp.remotePort();

      safePrint("UDP from " + senderIP + ":" + String(senderPort) + " -> " + message);

      String lower = message;
      lower.toLowerCase();

      if (lower.indexOf("ota") >= 0) {
        if (otaTaskHandle == NULL) {
          safePrint("OTA trigger received. Creating OTA task...");
          xTaskCreatePinnedToCore(OTATask, "OTA", 8192, NULL, 5, &otaTaskHandle, 0);
        } else {
          safePrint("OTA task already running.");
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void wifiService() {
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - lastWiFiAttempt >= 5000) {
      safePrint("Connecting Wi-Fi...");
      WiFi.begin(ssid, password);
      lastWiFiAttempt = millis();
      ipPrinted = false;
    }
  } else {
    if (!ipPrinted) {
      safePrint("Wi-Fi Connected.");
      safePrint("ESP32 IP: " + WiFi.localIP().toString());
      safePrint("UDP Port: " + String(UDP_LISTEN_PORT));
      safePrint("Send UDP packets to this IP and port.");
      ipPrinted = true;
    }
  }
}

void rfidService() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  bool match = true;

  if (rfid.uid.size == 4) {
    for (int i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != allowedUID[i]) {
        match = false;
        break;
      }
    }
  } else {
    match = false;
  }

  if (match) {
    safePrint("Authorized!");
    digitalWrite(LED_PIN, HIGH);
    ledActive = true;
    ledOffAt = millis() + 3000;
  } else {
    safePrint("Access Denied");
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void ledService() {
  if (ledActive && millis() >= ledOffAt) {
    digitalWrite(LED_PIN, LOW);
    ledActive = false;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  SPI.begin();
  rfid.PCD_Init();

  serialMutex = xSemaphoreCreateMutex();
  if (serialMutex == NULL) {
    while (true) {
    }
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  xTaskCreatePinnedToCore(UDPTask, "UDP", 4096, NULL, 2, &udpTaskHandle, 1);

  safePrint("System Started.");
}

void loop() {
  wifiService();
  rfidService();
  ledService();
  delay(10);
}