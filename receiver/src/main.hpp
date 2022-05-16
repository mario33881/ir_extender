#ifndef Main_h
#define Main_h

#include <WString.h>

#define RECV_PIN 14           // su Node MCU e' il Pin D5
#define BUFFER_SIZE 1024      // dimensione buffer lettura dati dal sensore IR
#define TIMEOUT 50            // timeout del sensore IR

#define DEFAULT_LED_ACTIVE_LOW true  // vero = led attivo alto, falso = led attivo basso

void connect_wifi();
void setDateTime();
void connect_mqtt();
void send_ir_msg(String value);
void successBlink();
void setup();
void loop();

#endif // Main_h
