#define MSG_BUFFER_SIZE (500)
#define RECV_PIN 14           // su Node MCU e' il Pin D5

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

// Update these with values suitable for your network.
const char* wifi_ssid = "<Wi-Fi SSID>";
const char* wifi_pwd = "<Wi-Fi password>";
const char* mqtt_server_url = "<HiveMQ MQTT URL>";
const char* mqtt_server_user = "<HiveMQ MQTT username>";
const char* mqtt_server_pwd = "<HiveMQ MQTT password>";

// Debug
const bool virtualIRrecv = false;

// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;

WiFiClientSecure espClient;
PubSubClient * mqttClient;

char msg[MSG_BUFFER_SIZE+1];

IRrecv irrecv(RECV_PIN);
decode_results ir_data;


/**
 * Si connette alla rete Wi-Fi.
 * 
 * Variabili globali:
 * - wifi_ssid: SSID della rete Wi-Fi a cui connettersi
 * - wifi_pwd: password della rete Wi-Fi
*/
void connect_wifi() {
    delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);

    WiFi.mode(WIFI_STA);  // modalita' stazione: connettiti ad una rete wireless esistente
    WiFi.begin(wifi_ssid, wifi_pwd);

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
*/
void connect_mqtt() {
    
    while (!mqttClient->connected()) {
        Serial.print("Attempting MQTT connection...");

        // Attempt to connect
        if (mqttClient->connect("IREXTENDER - Transmitter", mqtt_server_user, mqtt_server_pwd)) {
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
void send_ir_msg(uint64_t value){
    snprintf(msg, MSG_BUFFER_SIZE, "%llu", value);
    Serial.print("Raw value: ");
    Serial.println(value);
    Serial.print("Published message: ");
    Serial.println(msg);
    mqttClient->publish("signal", msg);
}


/**
 * Configurazione iniziale:
 * - Per debugging apre il monitor seriale e imposta come output il pin del LED sulla scheda.
 * - Abilita il ricevitore di segnali infrarossi
 * - Si connettite al Wi-Fi
 * - Imposta la data, necessaria per validare i Certificati SSL
 * - Inizializza il file system per poter configurare il client MQTT per fargli usare i certificati SSL
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
    mqttClient->setServer(mqtt_server_url, 8883);
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
            send_ir_msg(random(0, 1000));
            delay(10000);
        }
        else {
            send_ir_msg(ir_data.value);
        }
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);

        if (!virtualIRrecv){
            irrecv.resume(); // Receive the next value
        }
    }
}
