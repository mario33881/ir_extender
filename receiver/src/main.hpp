#ifndef Main_h
#define Main_h

#include <WString.h>

#ifdef NODEMCU_ESP8266
    #define RECV_PIN 14  // Pin D5
#else
    #define RECV_PIN 13  // Pin G13
    #ifndef LED_BUILTIN
        #define LED_BUILTIN 2
    #endif
#endif

#define BUFFER_SIZE 1024      // dimensione buffer lettura dati dal sensore IR
#define TIMEOUT 50            // timeout del sensore IR

#define DEFAULT_LED_ACTIVE_LOW true  // vero = led attivo alto, falso = led attivo basso

void connect_wifi();
void connect_mqtt();
void send_ir_msg(String value);
void successBlink();
void setup();
void loop();

#ifdef NODEMCU_ESP8266
void setDateTime();
#endif

#endif // Main_h
