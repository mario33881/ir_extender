#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <IRremote.hpp>

#include <Utils.hpp>

#include "secrets.h"

#define IR_SEND_PIN 12
#define IR_FREQUENCY 38


WiFiClientSecure espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length);
void connect_wifi();
void connect_mqtt();

void setup() {
  Serial.begin(9600);
  IrSender.begin(IR_SEND_PIN);

  connect_wifi();

  espClient.setInsecure();
  // espClient.setCACert(root_ca); // Use certificate for server verification

  client.setServer(MQTT_SERVER, 8883);
  client.setBufferSize(1024);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected())
    connect_mqtt();

  client.loop();
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in [");
  Serial.print(topic);
  Serial.println("] ");

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  String message_parts[2];
  split(message, '|', message_parts, 2, 1);

  int size = message_parts[0].toInt();
  String rawData_str[size];
  split(message_parts[1], ',', rawData_str, size);

  uint16_t rawData[size];
  for (int i = 0; i < size; i++) {
    rawData[i] = rawData_str[i].toInt();
  }

  Serial.print("rawData[");
  Serial.print(size);
  Serial.print("] = {");
  for (int i = 0; i < size; i++) {
    Serial.print(rawData[i]);
    if (i < size-1)
      Serial.print(", ");
  }
  Serial.println("}");

  // Send IR signal
  IrSender.sendRaw(rawData, length, IR_FREQUENCY);
}

void connect_wifi() {
  Serial.println();
  Serial.print("Connecting to WiFI");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" connected");
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());
}

void connect_mqtt() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT... ");

    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      client.publish("connected", "Receiver connected successfully");

      client.subscribe("signal");
    } else {
      Serial.print(" failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
