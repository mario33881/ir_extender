#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

#include <Utils.hpp>

#include "secrets.h"

#define IR_SEND_PIN 12
#define IR_FREQUENCY 38000


WiFiClientSecure espClient;
PubSubClient client(espClient);
IRsend IrSender(IR_SEND_PIN);


void callback(char* topic, byte* payload, unsigned int payload_length);
void connect_wifi();
void connect_mqtt();

void setup() {
  Serial.begin(9600);
  IrSender.begin();

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

void callback(char* topic, byte* payload, unsigned int payload_length) {
  Serial.print("Message arrived in [");
  Serial.print(topic);
  Serial.println("] ");

  String message = "";
  for (int i = 0; i < payload_length; i++) {
    message += (char)payload[i];
  }

  String message_parts[4];
  split(message, '|', message_parts, 4);

  decode_type_t protocol = (decode_type_t)message_parts[0].toInt();
  int length = message_parts[1].toInt();
  String values(message_parts[2]);
  int size = message_parts[3].toInt();

  Serial.print("protocol = ");
  Serial.print(protocol);
  Serial.print("; values[");
  Serial.print(length);
  Serial.print("] = {");
  Serial.print(values);
  Serial.print("}");
  Serial.print("; size = ");
  Serial.print(size);
  Serial.println("");

  // Send IR signal
  if (protocol == decode_type_t::UNKNOWN) {
    String* values_array{ new String[length]{} };
    split(values, ',', values_array, length);

    uint16_t* rawData{ new uint16_t[length]{} };
    for (int i = 0; i < length; i++) {
      rawData[i] = (uint64_t)(values_array[i].toDouble());
    }

    IrSender.sendRaw(rawData, length, IR_FREQUENCY);
    Serial.println("IR sent via rawData");

    delete[] values_array;
    delete[] rawData;
  }
  else if (hasACState(protocol)) {
    uint64_t data = (uint64_t)(values.toDouble());
    IrSender.send(protocol, data, size / 8);
    Serial.println("IR sent with ACState");
  }
  else {
    uint64_t data = (uint64_t)(values.toDouble());
    IrSender.send(protocol, data, size);
    Serial.println("IR sent");
  }
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
      client.publish("connected", "Transmitter connected");

      client.subscribe("signal");
    } else {
      Serial.print(" failed, rc = ");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
