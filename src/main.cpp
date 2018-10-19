#include <Arduino.h>
#include "ESP8266WiFi.h"
#include <w5500-lwIP.h>

// Arduino Pin 4 = Wemos Pin D2
#define CSPIN 4

Wiznet5500lwIP ether(CSPIN);

byte mac[] = {0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02};

void setup() {

  Serial.begin(115200);
  Serial.println("");
  Serial.println("start");

  int present = ether.begin(mac);
  Serial.println("foo");
  Serial.println("present= " + String(present, HEX));

  while (!ether.connected()) {
    Serial.print(".");
    delay(1000);
  }
}

void loop() { ether.loop(); }
