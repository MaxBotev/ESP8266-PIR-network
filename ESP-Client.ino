/*
Code for ESP-12E
*/
// #include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#define DHTPIN 2

// AP Wi-Fi credentials
const char* ssid = "ESP";
const char* password = "Max0504693150";
bool PIR = false;
const int ProbeID = 2;

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
IPAddress ip(177, 177, 177, 105);
IPAddress gateway(177, 177, 177, 1);
IPAddress subnet(255, 255, 255, 0);

//DHT dht(DHTPIN, DHT22);
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

void readParametric() {
  /*delay(200);
  h = 1.00; // dht.readHumidity();
  t = 2.00; //dht.readTemperature();
  //if (isnan(h) || isnan(t)) {
  //  t = 0.00;
  //  h = 0.00;
  //}
  Serial.println("WiFi quality : "+String(getWifiQuality()) + "dB");
  Serial.println("VCC : "+String(ESP.getVcc()) +"V");*/
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
  Serial.println("- data stream: "+data);
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
  ESP.eraseConfig();
  WiFi.persistent(false);
  Serial.begin(74880);
  Serial.println();
  
  //delay(500);
  Serial.println("- set ESP STA mode");
  WiFi.mode(WIFI_STA);
  Serial.println("- connecting to wifi");
  WiFi.config(ip, gateway, subnet); 
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    if(counter > 40){
       Serial.println("- can't connect, going to sleep");    
       hibernate(0.16);
    }
    delay(100);
    Serial.print(".");
    counter++;
  }
  
  Serial.println("- wifi connected");
  
  Serial.println("- build DATA stream string");
  buildDataStream();
  Serial.println("- send GET request");
  sendHttpRequest();
  Serial.println();
  Serial.println("- got back to sleep");
  
  
  hibernate(sleepInterval);
}



void loop() {}
