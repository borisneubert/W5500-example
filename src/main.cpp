#include <Arduino.h>

//....................................................
// WiFi
#include "ESP8266WiFi.h"
const char *ssid = "Pf@nne-NET";
const char *password = "Pf@nneNETwlan_ACCESS";

//....................................................
// W5500,  Arduino Pin 4 = Wemos Pin D2
#include <w5500-lwIP.h>
#define CSPIN 4
Wiznet5500lwIP eth(CSPIN);
byte mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xAA};

//....................................................
// asynchronous web server
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80);

//....................................................
// ntp client
#include <NTPClient.h>
#include <WiFiUdp.h>

#define NTPSERVER "91.202.42.83"
//#define NTPSERVER "0.ubuntu.pool.ntp.org"
//#define NTPSERVER "192.53.103.108"
WiFiUDP ntpUDP;
NTPClient ntpClient(ntpUDP, NTPSERVER, 3600, 20000);
unsigned long long t = 0, t0 = 0;

//....................................................
// mqtt client
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);
const char* mqtt_server = "192.168.1.11";
long lastMsg = 0;
char msg[50];
int value = 0;
String mqttMsg;

// mqtt callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("MQTT incomming subcribe: ");
  mqttMsg = topic;
  mqttMsg += " - ";
  for (int i = 0; i < length; i++) {
    mqttMsg += (char)payload[i];
  }
  Serial.println(mqttMsg);
}
// mqtt reconnect
void reconnect() {
  // Loop until we're reconnected

  //int i = 0;
  //while (!client.connected()) {
    //i++;
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    //clientId += String(random(0xffff), HEX);
    // Attempt to connect
    eth.loop();
    if (client.connect(clientId.c_str())) {
      eth.loop();
    Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic/start", "Hello World");
      // ... and resubscribe
      client.subscribe("inTopic/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      //delay(1000);
      //if (i>5) break;
    }
  /*
   -4 : MQTT_CONNECTION_TIMEOUT - the server didn't respond within the keepalive time
   -3 : MQTT_CONNECTION_LOST - the network connection was broken
   -2 : MQTT_CONNECT_FAILED - the network connection failed
   -1 : MQTT_DISCONNECTED - the client is disconnected cleanly
    0 : MQTT_CONNECTED - the client is connected
    1 : MQTT_CONNECT_BAD_PROTOCOL - the server doesn't support the requested version of MQTT
    2 : MQTT_CONNECT_BAD_CLIENT_ID - the server rejected the client identifier
    3 : MQTT_CONNECT_UNAVAILABLE - the server was unable to accept the connection
    4 : MQTT_CONNECT_BAD_CREDENTIALS - the username/password were rejected
    5 : MQTT_CONNECT_UNAUTHORIZED - the client was not authorized to connect
  */

  //}
}
//####################################################
// setup
//####################################################
void setup() {

// starting example
  Serial.begin(115200);
  Serial.println("");
  delay(1000);
  Serial.println("starting example...");

// starting wifi
//WiFi.mode(WIFI_OFF);

  Serial.println("starting wifi...");
  WiFi.mode(WIFI_STA);
  //WiFi.mode(WIFI_OFF);
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.print(".");
     i++;
     if (i>20) break;
  }
  Serial.println("");
  //if (WiFi.waitForConnectResult() != WL_CONNECTED) {
  if (i>20) {
    Serial.println("wifi connection failed");
    //return;
  } else {
    Serial.print("wifi ip address: ");
    Serial.println(WiFi.localIP());
    Serial.print("wifi hostname: ");
    Serial.println(WiFi.hostname());
  }

// starting ethernet

// enable Ethernet here-------------------
/*
  Serial.println("starting ethernet...");
  eth.setDefault(); // use ethernet for default route
  int present = eth.begin(mac);// eth.begin(mac);
  if (!present) {
    Serial.println("no W5500 present");
    //return;
  } else {
    Serial.print("connecting ethernet");
    while (!eth.connected()) {
      eth.loop();
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
    Serial.print("ethernet ip address: ");
    Serial.println(eth.localIP());
  }
*/
// starting web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String htmlStr = "";
    htmlStr += "<HTML>";
    htmlStr += "<HEAD>";
    htmlStr += "<meta http-equiv=\"refresh\" content=\"1\">";
    htmlStr += "<TITLE />d-a-v WiFi/ETH</title>";
    htmlStr += "</head>";
    htmlStr += "<BODY>";
    htmlStr += "d-a-v WiFi/ETH";
    htmlStr += "<br />";
    htmlStr += "WiFi IP: ";
    htmlStr += WiFi.localIP().toString();
    htmlStr += "<br />";
    htmlStr += "ETH IP: ";
    htmlStr += eth.localIP().toString();
    htmlStr += "<br />";
    htmlStr += "Time: ";
    htmlStr += ntpClient.getFormattedTime();
    htmlStr += "<br />";
    htmlStr += "MQTT: ";
    htmlStr += mqttMsg;
    htmlStr += "<br />";
    htmlStr += "</BODY>";
    htmlStr += "</HTML>";
    request->send(200, "text/html", htmlStr);
  });
  server.begin();

// starting ntp client
  Serial.println("starting ntp client...");
  ntpClient.begin();

// starting mqtt client
  client.setServer(mqtt_server, 1883);
  eth.loop();
  client.setCallback(callback);
  eth.loop();
  client.subscribe("inTopic");
  eth.loop();

// ready
  Serial.println("setup complete");
}

//####################################################
// loop
//####################################################
void loop() {
  eth.loop();
  client.loop();
  eth.loop();

  ntpClient.update();
  eth.loop();
  t = ntpClient.getEpochTime();
  if (t != t0) {
    eth.loop();
    if (!client.connected()) reconnect();
    eth.loop();

    Serial.println(ntpClient.getFormattedTime());
    Serial.print("wifi ip address: ");
    Serial.println(WiFi.localIP());
    Serial.print("ethernet ip address: ");
    Serial.println(eth.localIP());
    t0 = t;

    eth.loop();
    client.publish("outTopic", ntpClient.getFormattedTime().c_str());
    eth.loop();
  }
}
