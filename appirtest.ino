/*
Code for ESP-12E PIR WAKE from deep sleep
*/

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define DHTPIN 2

// AP Wi-Fi credentials
const char* ssid = "ESP";
const char* password = "Max0504693150";
bool PIR = false;
const int ProbeID = 1;

// Local ESP web-server address
String serverHost = "http://177.177.177.1/feed";
String data;
// DEEP_SLEEP Timeout interval
long sleepInterval = 5; // in minutes
// DEEP_SLEEP Timeout interval when connecting to AP fails
int failConnectRetryInterval = 10; //in seconds
int counter = 0;

float h;
float t;
// Static network configuration
IPAddress ip(177, 177, 177, 101);
IPAddress gateway(177, 177, 177, 1);
IPAddress subnet(255, 255, 255, 0);

WiFiClient client;

void sendHttpRequest() {
  HTTPClient http;
  http.begin(serverHost);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(data);
 
  String Response = http.getString();
  // http.writeToStream(&Serial);
  
  sleepInterval = Response.toInt();
  Serial.println(sleepInterval);
  http.end();
}


void buildDataStream() {
  data = "pir=";
  data += String(PIR);
  data += "&wifi=";
  data += String(getWifiQuality());
  data += "&vcc=";
  data += String(ESP.getVcc());
  data += "&probe=";
  data += String(ProbeID);
  Serial.println("Data stream: "+data);
}

int8_t getWifiQuality() {
  int32_t dbm = WiFi.RSSI();
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}

void hibernate(int pInterval) {
  WiFi.disconnect();
  ESP.deepSleep(60000000 * pInterval, WAKE_RFCAL);
  delay(100);
}

void setup() {
  int value = digitalRead(D2);
  Serial.begin(74880);
  Serial.println();
  if (value == 1) PIR = true;
  Serial.println(String(value));
  ESP.eraseConfig();
  WiFi.persistent(false);
  
  
 
  Serial.println("Set ESP STA mode");
  WiFi.mode(WIFI_STA);
  Serial.println("Connecting to wifi");
  WiFi.config(ip, gateway, subnet); 
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    if(counter > 40){
       Serial.println("Can't connect, going to sleep");    
       ESP.deepSleep(60e6,WAKE_RFCAL);
    }
    delay(100);
    Serial.print(".");
    counter++;
  }
  
  Serial.println("wifi connected");
  
  Serial.println("build DATA stream string");
  buildDataStream();
  Serial.println("send GET request");
  sendHttpRequest();
  Serial.println();
  Serial.println("got back to sleep");
  
  
  hibernate(sleepInterval);
}



void loop() {}
