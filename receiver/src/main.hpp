#ifndef Main_h
#define Main_h

#include <WString.h>

#define RECV_PIN 14           // su Node MCU e' il Pin D5

void connect_wifi();
void setDateTime();
void connect_mqtt();
void send_ir_msg(String value);
void setup();
void loop();

#endif // Main_h
