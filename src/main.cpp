#include <Arduino.h>

//....................................................
// WiFi
#include "ESP8266WiFi.h"
const char *ssid = "***";
const char *password = "***";

//....................................................
#define CSPIN 4 // GPIO4

#include <W5500lwIP.h>
Wiznet5500lwIP eth(SPI, CSPIN);

//#include <W5100lwIP.h>
//Wiznet5100lwIP eth(SPI, CSPIN);

int present = 0;

#include <osapi.h>
LOCAL os_timer_t eth_timer;
int led = 0;

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
const char* mqtt_server = "192.168.65.43";
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
  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP8266Client-";
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");
    client.publish("outTopic/start", "Hello from W5500!");
    client.subscribe("inTopic/#");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
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
}

//....................................................
void ICACHE_RAM_ATTR eth_loop(void) {
  digitalWrite(LED_BUILTIN, led);
  led = 1 - led;
  //if(present) eth.loop();
}

bool schedule_function_us(const std::function<bool(void)>& fn, uint32_t repeat_ms = 0);

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
  
  Serial.println("starting wifi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    if (i > 20) break;
  }
  Serial.println("");
  if (i > 20) {
    Serial.println("wifi connection failed");
  } else {
    Serial.print("wifi ip address: ");
    Serial.println(WiFi.localIP());
    Serial.print("wifi hostname: ");
    Serial.println(WiFi.hostname());
  }
  

  // starting ethernet
  // enable Ethernet here-------------------
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("starting ethernet...");
  //os_timer_disarm(&eth_timer);
  //os_timer_setfn(&eth_timer, (os_timer_func_t *)eth_loop, NULL);
  //os_timer_arm(&eth_timer, 1, 1);

  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV4); // 4 MHz?
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  eth.setDefault(); // use ethernet for default route
  present = eth.begin();
  if (!present) {
    Serial.println("no ethernet hardware present");
    //return;
  } else {
    Serial.print("connecting ethernet");
    while (!eth.connected()) {
      Serial.print(".");
      delay(1000);
    }
    Serial.println();
    Serial.print("ethernet ip address: ");
    Serial.println(eth.localIP());
  }

  // starting web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    String htmlStr = "";
    htmlStr += "<HTML>";
    htmlStr += "<HEAD>";
    htmlStr += "<meta http-equiv=\"refresh\" content=\"1\">";
    htmlStr += "<TITLE />WiFi/ETH</title>";
    htmlStr += "</head>";
    htmlStr += "<BODY>";
    htmlStr += "WiFi/ETH";
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
  client.setCallback(callback);
  client.subscribe("inTopic");

  // ready
  Serial.println("setup complete");
}

//####################################################
// loop
//####################################################
void loop() {
  while (true)
  {
    client.loop();
    ntpClient.update();

    t = ntpClient.getEpochTime();
    if (t != t0) {
      if (!client.connected()) reconnect();

      Serial.println(ntpClient.getFormattedTime());
      Serial.print("wifi ip address: ");
      Serial.println(WiFi.localIP());
      Serial.print("ethernet ip address: ");
      Serial.println(eth.localIP());
      //Serial.println(eth.begin());

      client.publish("outTopic", ntpClient.getFormattedTime().c_str());
      t0 = t;

      uint32_t mfree;
      uint16_t mmax;
      uint8_t frag;
      ESP.getHeapStats(&mfree, &mmax, &frag);
      Serial.printf("%d %d %d\n", mfree, mmax, frag);

    }
    yield();
  }
}