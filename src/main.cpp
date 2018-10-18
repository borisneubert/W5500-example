//#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <w5500-lwIP.h>

#define CSPIN 15

Wiznet5500lwIP ether(CSPIN);

void setup() { ether.begin(); }
void loop() { ether.loop(); }
