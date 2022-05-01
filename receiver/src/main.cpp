#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include <TZ.h>
#include <FS.h>
#include <LittleFS.h>
#include <CertStoreBearSSL.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <Utils.hpp>

#include "main.hpp"
#include "secrets.h"

// Debug
const bool virtualIRrecv = false;

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * mqttClient;

IRrecv irrecv(RECV_PIN);
decode_results ir_data;


/**
 * Si connette alla rete Wi-Fi.
 * 
 * Macro:
 * - WIFI_SSID: SSID della rete Wi-Fi a cui connettersi
 * - WIFI_PASS: password della rete Wi-Fi
*/
void connect_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);  // modalita' stazione: connettiti ad una rete wireless esistente
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    randomSeed(micros());

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}


/**
 * Imposta la data corretta connettendosi ad un server NTP.
 * 
 * Necessario per validare i certificati SSL.
*/
void setDateTime() {
    configTime(TZ_Europe_Rome, "pool.ntp.org", "time.nist.gov");

    Serial.print("Waiting for NTP time sync: ");
    time_t now = time(nullptr);
    while (now < 8 * 3600 * 2) {
        delay(100);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println();

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    Serial.printf("%s %s", tzname[0], asctime(&timeinfo));
}


/**
 * Si connette al server MQTT.
 * 
 * Se ci riesce manda un messaggio al topic "connected" indicando che il dispositivo si e' connesso correttamente
 * 
 * Macro:
 * - MQTT_CLIENT_ID: identificativo del client MQTT
 * - MQTT_USER: username MQTT
 * - MQTT_PASS: password MQTT
*/
void connect_mqtt() {
    
    while (!mqttClient->connected()) {
        Serial.print("Attempting MQTT connection...");

        // Attempt to connect
        if (mqttClient->connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
            Serial.println(" connected");
            mqttClient->publish("connected", "Transmitter connected successfully");
        } 
        else {
            Serial.print(" failed, rc = ");
            Serial.print(mqttClient->state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}


/**
 * Invia il valore letto dal sensore IR tramite MQTT.
*/
void send_ir_msg(String value){
    Serial.print("Published message: ");
    Serial.println(value);
    mqttClient->publish("signal", value.c_str());
}


/**
 * Configurazione iniziale:
 * - Per debugging apre il monitor seriale e imposta come output il pin del LED sulla scheda.
 * - Abilita il ricevitore di segnali infrarossi
 * - Si connettite al Wi-Fi
 * - Imposta la data, necessaria per validare i Certificati SSL
 * - Inizializza il file system per poter configurare il client MQTT per fargli usare i certificati SSL
 * 
 * Macro:
 * - MQTT_SERVER: url del server MQTT
*/
void setup() {
    
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    irrecv.enableIRIn();

    connect_wifi();
    setDateTime();

    // -- Certificati SSL
    // inizializza il file system
    LittleFS.begin();

    // cerca i certificati nel file system
    int numCerts = certStore.initCertStore(LittleFS, PSTR("/certs.idx"), PSTR("/certs.ar"));
    Serial.printf("Number of CA certs read: %d\n", numCerts);
    if (numCerts == 0) {
        Serial.printf("No certs found. Did you run certs-from-mozilla.py and upload the LittleFS directory before running?\n");
        return;
    }
    
    // configura il client per usare i certificati
    BearSSL::WiFiClientSecure *bear = new BearSSL::WiFiClientSecure();
    bear->setCertStore(&certStore);
    mqttClient = new PubSubClient(*bear);

    // imposta il server MQTT
    mqttClient->setServer(MQTT_SERVER, 8883);

    // imposta dimensione buffer dei messaggi
    mqttClient->setBufferSize(1024);

    // segnala fine del setup
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
    }
}


/**
 * Ripeti all'infinito:
 * - se non si e' connessi al Wi-Fi, connettiti.
 * - se non si e' connessi al Broker MQTT, connettiti.
 * - quando viene letto un valore del sensore IR, invialo tramite MQTT
 *   > Esegui il blink del LED della scheda per indicare che un nuovo valore e' stato letto dal sensore IR
*/
void loop() {

    if (WiFi.status() != WL_CONNECTED) {
        connect_wifi();
    }

    if (!mqttClient->connected()) {
        connect_mqtt();
    }
    mqttClient->loop();

    if (irrecv.decode(&ir_data) || virtualIRrecv) {

        if (virtualIRrecv){
            // debug
            send_ir_msg(String(random(0, 1000)));
            delay(10000);
        }
        else {
            // visualizza informazioni utili sui dati letti
            Serial.print(resultToHumanReadableBasic(&ir_data));
            Serial.println("");
            
            // Ottieni array valori letti in un formato comodo per la funzione sendRaw().
            uint16_t *raw_array = resultToRawArray(&ir_data);
            // Conta il numero di elementi dell'array
            uint16_t length = getCorrectedRawLength(&ir_data);

            // converti l'array in una stringa con formato "<length>|<val1>, <val2>,..." e inviala al broker MQTT
            String data_msg = uint16ArrayToString(raw_array, length);
            send_ir_msg(data_msg);

            // Dealloca memoria allocata da resultToRawArray().
            delete [] raw_array;
        }
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);

        if (!virtualIRrecv){
            irrecv.resume(); // Receive the next value
        }
    }
}
