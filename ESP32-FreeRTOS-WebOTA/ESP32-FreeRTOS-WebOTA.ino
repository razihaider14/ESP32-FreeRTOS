#include <WiFi.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include <Update.h>
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

WebServer server(80);

MFRC522 rfid(SS_PIN, RST_PIN);

byte allowedUID[4] = {0x5C, 0xB8, 0x3B, 0x05};

bool ipPrinted = false;

TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t otaTaskHandle = NULL;

unsigned long lastWiFiAttempt = 0;

SemaphoreHandle_t serialMutex;


void safePrint(const String& msg) {
  if (serialMutex != NULL) {
    xSemaphoreTake(serialMutex, portMAX_DELAY);
    Serial.println(msg);
    xSemaphoreGive(serialMutex);
  }
  else {
    Serial.println(msg);
  }
}

void OTATask(void* pvParameters)
{
    safePrint("OTA Task Started.");

    server.on("/", HTTP_GET, []()
    {
        server.send(
            200,
            "text/html",
            "<form method='POST' "
            "action='/update' "
            "enctype='multipart/form-data'>"
            "<input type='file' name='update'>"
            "<input type='submit' value='Upload'>"
            "</form>"
        );
    });

    server.on(
        "/update",
        HTTP_POST,

        []()
        {
            server.send(
                200,
                "text/plain",
                "Update Complete. Rebooting..."
            );

            delay(1000);

            ESP.restart();
        },

        []()
        {
            HTTPUpload& upload =
                server.upload();

            switch (upload.status)
            {

                case UPLOAD_FILE_START:

                    Update.begin(
                        UPDATE_SIZE_UNKNOWN
                    );

                    break;

                case UPLOAD_FILE_WRITE:

                    Update.write(
                        upload.buf,
                        upload.currentSize
                    );

                    break;

                case UPLOAD_FILE_END:

                    Update.end(true);

                    break;

                default:
                    break;
            }
        }
    );

    server.begin();

    safePrint(
        "Web OTA Ready:"
    );

    safePrint(
        "http://" +
        WiFi.localIP().toString()
    );

    while (true)
    {
        server.handleClient();

        vTaskDelay(
            pdMS_TO_TICKS(10)
        );
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

        xTaskCreatePinnedToCore(OTATask, "OTA", 16384, NULL, 5, &otaTaskHandle, 0);
      } 
      else {
        safePrint("OTA Task already exists.");
      }
    }
  }
}

void MQTTTask(void *pvParameters) {

  safePrint("MQTT Task Started.");

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  bool mqttConnected = false;

  while (true) {

    if (WiFi.status() != WL_CONNECTED) {

      if (mqttConnected) {
        client.disconnect();
        mqttConnected = false;
        safePrint("Wi-Fi lost. MQTT disconnected.");
      }

      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!mqttConnected) {

      safePrint("Connecting MQTT...");

      if (client.connect("ESP32")) {

        mqttConnected = true;

        safePrint("MQTT Connected.");

        client.subscribe("esp32/ota");

      } else {

        safePrint(
          "MQTT Connect Failed. State: " +
          String(client.state())
        );

        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }

    client.loop();

    client.publish(
      "esp32/heartbeat",
      "Alive"
    );

    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}


void WiFiFunction() {

  if (WiFi.status() != WL_CONNECTED) {

    if (millis() - lastWiFiAttempt >= 5000) {

      safePrint("Connecting Wi-Fi...");

      WiFi.begin(ssid, password);

      lastWiFiAttempt = millis();

      ipPrinted = false;
    }
  }
  else {

    if (!ipPrinted) {

      safePrint("Wi-Fi Connected.");
      safePrint(
          "ESP32 IP: " +
          WiFi.localIP().toString()
      );

      ipPrinted = true;
    }
  }
}

void LEDFunction() {
  digitalWrite(LED_PIN, HIGH);
  delay(3000);
  digitalWrite(LED_PIN, LOW);
}

void RFIDFunction() {
  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  bool match = true;

  for (int i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != allowedUID[i]) {
      match = false;
    }
  }

  if (match) {
    safePrint("Authorized!");

    LEDFunction();
  } else {
    safePrint("Access Denied");
  }

  rfid.PICC_HaltA();
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

  xTaskCreatePinnedToCore(MQTTTask, "MQTT", 8192, NULL, 2, &mqttTaskHandle, 1);

  safePrint("System Started.");
}

void loop() {
  WiFiFunction();
  RFIDFunction();

  delay(10);
}