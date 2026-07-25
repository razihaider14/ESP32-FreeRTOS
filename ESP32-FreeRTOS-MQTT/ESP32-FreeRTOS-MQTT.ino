#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 5
#define RST_PIN 22
#define LED_PIN 2

const char* ssid = "*****";
const char* password = "*****";
const char* mqtt_server = "*****";

WiFiClient espClient;
PubSubClient client(espClient);

MFRC522 rfid(SS_PIN, RST_PIN);

byte allowedUID[4] = {0x5C, 0xB8, 0x3B, 0x05};

TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t rfidTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;

SemaphoreHandle_t serialMutex;


void safePrint(String msg) {
  xSemaphoreTake(serialMutex, portMAX_DELAY);

  Serial.println(msg);

  xSemaphoreGive(serialMutex);
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
      Serial.printf("OTA Error [%u]\n", error);
    });

  ArduinoOTA.begin();
  safePrint("Waiting for OTA upload...");

  while (true) {
    ArduinoOTA.handle();

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;

  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String topicName = String(topic);

  safePrint("Topic: " + topicName + " | Message: " + message);

  if (topicName == "esp32/ota") {
    if (message == "ON") {
      if (otaTaskHandle == NULL) {
        safePrint("Creating OTA Task...");

        xTaskCreatePinnedToCore(OTATask, "OTA", 8192, NULL, 5, &otaTaskHandle, 0);
      } 
      else {
        safePrint("OTA Task already exists.");
      }
    }
  }
}

void WiFiTask(void *pvParameters) {
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      safePrint("Connecting WiFi...");

      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      safePrint("WiFi Connected!");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void MQTTTask(void *pvParameters) {
  client.setServer(mqtt_server, 1883);

  client.setCallback(callback);

  while (true) {
    if (!client.connected()) {
      safePrint("Connecting MQTT...");

      while (!client.connected()) {
        client.connect("ESP32");

        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      safePrint("MQTT Connected");

      client.subscribe("esp32/ota");
    }
    client.loop();

    client.publish("esp32/heartbeat", "Alive");

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void RFIDTask(void *pvParameters) {
  while (true) {
    if (!rfid.PICC_IsNewCardPresent()) {
      vTaskDelay(pdMS_TO_TICKS(100));
      continue;
    }

    if (!rfid.PICC_ReadCardSerial()) {
      continue;
    }

    bool match = true;

    for (int i = 0; i < 4; i++) {
      if (rfid.uid.uidByte[i] != allowedUID[i]) {
        match = false;
      }
    }

    if (match) {
      safePrint("Authorized!");

      digitalWrite(LED_PIN, HIGH);

      vTaskDelay(pdMS_TO_TICKS(3000));

      digitalWrite(LED_PIN, LOW);
    } 
    else {
      safePrint("Access Denied");
    }

    rfid.PICC_HaltA();

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);

  SPI.begin();

  rfid.PCD_Init();

  serialMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(WiFiTask, "WiFi", 4096, NULL, 2, &wifiTaskHandle, 1);

  xTaskCreatePinnedToCore(MQTTTask, "MQTT", 4096, NULL, 1, &mqttTaskHandle, 1);

  xTaskCreatePinnedToCore(RFIDTask, "RFID", 4096, NULL, 2, &rfidTaskHandle, 0);

  safePrint("System Started.");
}

void loop() {
  vTaskDelete(NULL);
}