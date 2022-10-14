#include <Arduino.h>

#include <PubSubClient.h>

#ifdef NODEMCU_ESP8266
    #include <ESP8266WiFi.h>
    #include <time.h>
    #include <TZ.h>
    #include <FS.h>
    #include <LittleFS.h>
    #include <CertStoreBearSSL.h>
#else 
    #ifdef NODEMCU_ESP32
        #include <WiFi.h>
        #include <WiFiClientSecure.h>
    #else
        #error Board not supported
    #endif
#endif

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include <Utils.hpp>

#include "main.hpp"
#include "secrets.h"

// Debug
const bool virtualIRrecv = false;

#ifdef NODEMCU_ESP8266
// A single, global CertStore which can be used by all connections.
// Needs to stay live the entire time any of the WiFiClientBearSSLs
// are present.
BearSSL::CertStore certStore;
#endif

WiFiClientSecure espClient;
PubSubClient * mqttClient;

IRrecv irrecv(RECV_PIN, BUFFER_SIZE, TIMEOUT, false);
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


#ifdef NODEMCU_ESP8266
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
#endif


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
            mqttClient->publish("connected", "Receiver connected successfully");
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
 * - Abilita il ricevitore di segnali infrarossi e imposta un threshold per ridurre rumore rilevabile dal sensore IR
 * - Si connettite al Wi-Fi
 * - Imposta la data, necessaria per validare i Certificati SSL
 *   > solo su ESP8266
 * - Inizializza il file system per poter configurare il client MQTT per fargli usare i certificati SSL
 *   > solo su ESP8266
 * - Configura il client MQTT (URL e porta server, dimensione buffer messaggi) e lo usa per connettersi al broker MQTT
 *
 * Macro:
 * - BUFFER_SIZE: dimensione buffer letti dal sensore IR
 * - MQTT_SERVER: url del server MQTT
*/
void setup() {

    Serial.begin(115200);
    pinMode(LED_BUILTIN, OUTPUT);

    irrecv.setUnknownThreshold(BUFFER_SIZE+1);  // riduce probabilita' di leggere rumore
    irrecv.enableIRIn();

    connect_wifi();

    #ifdef NODEMCU_ESP8266

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
    #else
    espClient.setInsecure();
    mqttClient = new PubSubClient(espClient);
    #endif

    // imposta il server MQTT
    mqttClient->setServer(MQTT_SERVER, 8883);

    // imposta dimensione buffer dei messaggi
    mqttClient->setBufferSize(1024);

    // connettiti al broker MQTT
    connect_mqtt();

    // segnala fine del setup
    for (int i = 0; i < 3; i++) {
        successBlink();
        delay(500);
    }
}


/**
 * Esegui un blink per indicare "operazione eseguita con successo".
*/
void successBlink(){
    #if DEFAULT_LED_ACTIVE_LOW == true
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);
    #else
    digitalWrite(LED_BUILTIN, LOW);
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    #endif
}

/**
 * Ripeti all'infinito:
 * - se non si e' connessi al Wi-Fi, connettiti.
 * - se non si e' connessi al Broker MQTT, connettiti.
 * - quando viene letto un valore del sensore IR, invialo tramite MQTT
 *   > Esegui il blink del LED della scheda per indicare che un nuovo valore e' stato letto dal sensore IR e trasmesso al broker MQTT
 *
 * I dati letti possono avere protocolli diversi:
 * - protocollo sconosciuto: non e' stato possibile riconoscere il protocollo. Verra' inviato un messaggio generico
 *
 *   Formato messaggio: "-1|<length>|<value1>,<value2>,...|-1"
 *   Dove:
 *   + "-1" indica che il protocollo non e' stato riconosciuto
 *   + "<length>" e' il numero di valori "<valueX>"
 *   + "<valueX>" sono valori raw letti dal sensore IR. Questi valori verranno trasmessi dal metodo sendRaw() con il trasmettitore IR
 *   + "-1" (size) indica "non applicabile" poiche' il protocollo non richiede state[]
 *
 * - protocollo con state: sono protocolli piu' complessi (ad esempio per i condizionatori)
 *
 *   Formato messaggio: "<protocol>|<length>|<value1>,<value2>,...|<size>"
 *   Dove:
 *   + "<protocol>" e' un identificativo del protocollo rilevato dal sensore IR
 *   + "<length>" e' il numero di valori "<valueX>"
 *   + "<valueX>" sono valori raw letti dal sensore IR
 *   + "<size>" (in bit) verra' usato dal metodo send() con il trasmettitore IR
 *
 * - protocollo semplice: il protocollo e' composto da un singolo valore
 *
 *   Formato messaggio: "<protocol>|1|<value>|<size>"
 *   Dove:
 *   + "<protocol>" e' un identificativo del protocollo rilevato dal sensore IR
 *   + "1" indica che c'e' un solo valore letto
 *   + "<value>" e' il valore letto. Verra' usato dal metodo send() con il trasmettitore IR
 *   + "<size>" (in bit) verra' usato dal metodo send() con il trasmettitore IR
*/
void loop() {

    if (WiFi.status() != WL_CONNECTED) {
        connect_wifi();
    }

    if (!mqttClient->connected()) {
        connect_mqtt();
    }
    mqttClient->loop();

    if (irrecv.decode(&ir_data)) {
        // visualizza informazioni utili sui dati letti
        Serial.print(resultToHumanReadableBasic(&ir_data));
        Serial.print("Overflow? ");
        Serial.println(ir_data.overflow);
        decode_type_t protocol = ir_data.decode_type;

        if (protocol == decode_type_t::UNKNOWN && ir_data.overflow == 0) {
            Serial.println("Protocollo sconosciuto");

            // Ottieni array valori letti in un formato comodo per la funzione sendRaw().
            uint16_t *raw_array = resultToRawArray(&ir_data);
            // Conta il numero di elementi dell'array
            uint16_t length = getCorrectedRawLength(&ir_data);

            // converti l'array in una stringa con formato "<length>|<val1>, <val2>,..." e invia al broker MQTT specificando che il protocollo e' sconosciuto
            String data_msg = uint16ArrayToString(raw_array, length);
            send_ir_msg(String(protocol) + "|" + data_msg + "|-1");

            // Dealloca memoria allocata da resultToRawArray().
            delete [] raw_array;

            successBlink();
        }
        else if (hasACState(protocol) && ir_data.overflow == 0) {
            // Il messaggio necessita dell'utilizzo di state[]
            Serial.print("Protocollo state: protocol = ");
            Serial.print(protocol);
            Serial.print(" | size = ");
            Serial.println(ir_data.bits);

            String data_msg = uint8ArrayToString(ir_data.state, sizeof(ir_data.state));
            send_ir_msg(String(protocol) + "|" + data_msg + "|" + String(ir_data.bits));
            successBlink();
        }
        else if (ir_data.overflow == 0) {
            // procollo semplice (esempio <= 64 bit)
            Serial.print("Protocollo semplice: Protocol = ");
            Serial.println(protocol);
            String value = uint64ToString(ir_data.value);
            send_ir_msg(String(protocol) + "|1|" + value + "|" + String(ir_data.bits));
            successBlink();
        }

        Serial.println("------------------------------------------------");
        irrecv.resume(); // Receive the next value
    }
    else if (virtualIRrecv){
        // debug
        send_ir_msg(String(random(0, 1000)));
        delay(2000);
        successBlink();
        Serial.println("------------------------------------------------");
    }
}
