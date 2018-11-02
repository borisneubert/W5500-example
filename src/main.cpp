#include <Arduino.h>

// WiFi
#include "ESP8266WiFi.h"
const char *ssid = "ssid";
const char *password = "password";

// W5500,  Arduino Pin 4 = Wemos Pin D2
#include <w5500-lwIP.h>
#define CSPIN 4
Wiznet5500lwIP eth(CSPIN);
byte mac[] = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56};

// asynchronous web server
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

// ntp client
#include <NTPClient.h>
#include <WiFiUdp.h>
// #define NTPSERVER "0.ubuntu.pool.ntp.org"
#define NTPSERVER "192.53.103.108"
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, NTPSERVER, 3600, 20000);
unsigned long long t = 0, t0 = 0;
//
// setup
//
void setup() {

  // starting example
  Serial.begin(115200);
  Serial.println("");
  delay(1000);
  Serial.println("starting example...");

  // starting wifi
  Serial.println("starting wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("wifi connection failed");
    return;
  } else {
    Serial.print("wifi ip address: ");
    Serial.println(WiFi.localIP());
    Serial.print("wifi hostname: ");
    Serial.println(WiFi.hostname());
  }

  // starting ethernet
  Serial.println("starting ethernet...");
  eth.setDefault(); // use ethernet for default route
  int present = eth.begin();// eth.begin(mac);
  // Serial.println("present= " + String(present, HEX));
  if (!present) {
    Serial.println("no W5500 present");
    return;
  }
  Serial.print("connecting ethernet");
  while (!eth.connected()) {
    eth.loop();
    Serial.print(".");
    delay(1000);
  }
  Serial.println();
  Serial.print("ethernet ip address: ");
  Serial.println(eth.localIP());

  // starting web server
  Serial.println("starting web server...");
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world");
  });
  server.begin();

  // starting ntp client
  Serial.println("starting ntp client...");
  ntpClient.begin();

  // ready
  Serial.println("setup complete");
}

void loop() {

  eth.loop();

  ntpClient.update();

  t = ntpClient.getEpochTime();

  if (t != t0) {
    Serial.println(ntpClient.getFormattedTime());
    t0 = t;
  }

}
