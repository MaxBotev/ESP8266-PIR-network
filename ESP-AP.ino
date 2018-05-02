/*
Code for a WEMOS D1 Mini Pro
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "settings.h"
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include "TouchControllerWS.h"
#include <MiniGrafx.h>
#include <ILI9341_SPI.h>
#include "ArialRounded.h"

#define MINI_BLACK 0
#define MINI_WHITE 1
#define MINI_YELLOW 2
#define MINI_RED 3

int SCREEN_WIDTH = 240;
int SCREEN_HEIGHT = 320;
// Limited to 4 colors due to memory constraints
int BITS_PER_PIXEL = 2; // 2^2 =  4 colors

uint16_t palette[] = {ILI9341_BLACK, // 0
                      ILI9341_WHITE, // 1
                      ILI9341_GREEN, // 2
                      //0x7E3C
                      ILI9341_RED
                      }; //3

// Server ESP credentials
const char* ssid = "ESP";
const char* password = "Max0504693150";
IPAddress    apIP(177, 177, 177, 1);
String t;
String h;
String p;
String Probe1 = "offline";
String Probe2 = "offline";
String Probe3 = "offline";
String Probe4 = "offline";
String Probe5 = "offline";
long P1Wifi = 0;
long P2Wifi = 0;
long P3Wifi = 0;
long P4Wifi = 0;
long P5Wifi = 0;
String P1VCC = "0";
String P2VCC = "0";
String P3VCC = "0";
String P4VCC = "0";
String P5VCC = "0";
unsigned long P1Alive = 0;
unsigned long P2Alive = 0;
unsigned long P3Alive = 0;
unsigned long P4Alive = 0;
unsigned long P5Alive = 0;
unsigned long P1Alarm = 0;
unsigned long P2Alarm = 0;
unsigned long P3Alarm = 0;
unsigned long P4Alarm = 0;
unsigned long P5Alarm = 0;
bool P1PIR;
bool P2PIR;
bool P3PIR;
bool P4PIR;
bool P5PIR;
int P1Counts = 0;
int P2Counts = 0;
int P3Counts = 0;
int P4Counts = 0;
int P5Counts = 0;
bool ShowCounts = false;
bool MapStyle = false;
bool Snooze = false;
int SnoozeTime = 0;
unsigned long SnoozeStart = 0;

String PIRStatus;

ADC_MODE(ADC_VCC);


ILI9341_SPI tft = ILI9341_SPI(TFT_CS, TFT_DC);
MiniGrafx gfx = MiniGrafx(&tft, BITS_PER_PIXEL, palette);

XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
TouchControllerWS touchController(&ts);

void calibrationCallback(int16_t x, int16_t y);
CalibrationCallback calibration = &calibrationCallback;
  
void drawProgress(uint8_t percentage, String text);
void drawTime();
void drawLabelValue(uint8_t line, String label, String value);
void drawAbout();
void drawSeparator(uint16_t y);

//int frameCount = 3;

// how many different screens do we have?
int screenCount = 3;
uint16_t screen = 0;
long timerPress;
bool canBtnPress;
int DefaultProbeSleep = 30;



ESP8266WebServer server(80);

void setup() {
  pinMode(D0, OUTPUT);
  Serial.begin(74880);
  
  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);    // HIGH to Turn on;

  
  gfx.init();
  gfx.fillBuffer(MINI_BLACK);
  gfx.commit();

  ts.begin();

  bool isFSMounted = SPIFFS.begin();
  if (!isFSMounted) {
    SPIFFS.format();
  }
  //SPIFFS.remove("/calibration.txt");
  boolean isCalibrationAvailable = touchController.loadCalibration();
  if (!isCalibrationAvailable) {
    Serial.println("Calibration not available");
    touchController.startCalibration(&calibration);
    while (!touchController.isCalibrationFinished()) {
      gfx.fillBuffer(0);
      gfx.setColor(MINI_YELLOW);
      gfx.setTextAlignment(TEXT_ALIGN_CENTER);
      gfx.drawString(120, 160, "Please calibrate\ntouch screen by\ntouch point");
      touchController.continueCalibration();
      gfx.commit();
      yield();
    }
    touchController.saveCalibration();
  }

  timerPress = millis();
  canBtnPress = true;

  
  WiFi.mode(WIFI_AP_STA);
  setupAccessPoint();
}

long lastDrew = 0;
bool btnClick;


// Handling the / root web page from my server
void handle_index() {
  server.send(200, "text/plain", "Get the fuck out from my server!");
}

// Handling the /feed page from my server
void handle_feed() {
  t = server.arg("wifi");
  h = server.arg("vcc");
  p = server.arg("probe");
  PIRStatus = server.arg("pir");
  Serial.println("Probe: "+p+" Wifi: "+t+" VCC: "+h+" PIR: "+PIRStatus);
  server.send(200, "text/plain", String(DefaultProbeSleep)); // Tell the probe for how long it has to sleep 
  if (p == "1") { Probe1 = "online"; P1VCC = h; P1Wifi = t.toInt(); P1Alive=millis(); if (PIRStatus == "0") P1PIR=0; else {P1Alarm = millis();P1PIR = 1 ; P1Counts ++; if (!Snooze) Alarm();};} // reset probe's online timer
  if (p == "2") { Probe2 = "online"; P2VCC = h; P2Wifi = t.toInt(); P2Alive=millis(); if (PIRStatus == "0") P2PIR=0; else {P2Alarm = millis();P2PIR = 1 ; P2Counts ++; if (!Snooze) Alarm();};} 
  if (p == "3") { Probe3 = "online"; P3VCC = h; P3Wifi = t.toInt(); P3Alive=millis(); if (PIRStatus == "0") P3PIR=0; else {P3Alarm = millis();P3PIR = 1 ; P3Counts ++; if (!Snooze) Alarm();};} 
  if (p == "4") { Probe4 = "online"; P4VCC = h; P4Wifi = t.toInt(); P4Alive=millis(); if (PIRStatus == "0") P4PIR=0; else {P4Alarm = millis();P4PIR = 1 ; P4Counts ++; if (!Snooze) Alarm();};}
  if (p == "5") { Probe5 = "online"; P5VCC = h; P5Wifi = t.toInt(); P5Alive=millis(); if (PIRStatus == "0") P5PIR=0; else {P5Alarm = millis();P5PIR = 1 ; P5Counts ++; if (!Snooze) Alarm();};} 
  
  // Update screen 
  
}
void Alarm()
{
 tone(D0, 800); delay(100); noTone(D0); 
}

void setupAccessPoint(){
  Serial.println("** SETUP ACCESS POINT **");
  WiFi.disconnect();
  Serial.println("Start ap with SID: "+ String(ssid));
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  WiFi.softAP(ssid, password);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("- AP IP address is :");
  Serial.println(myIP);
  setupServer();

}


void setupServer(){
  Serial.println("Starting web server");
  server.on("/", handle_index);
  server.on("/feed", handle_feed);
  server.begin();
};



void loop() {
  server.handleClient();

  if ((millis()-SnoozeStart) > 1 * 1000 *60) { SnoozeTime --; SnoozeStart = millis();}
    if (SnoozeTime <= 0) {SnoozeTime = 0; Snooze = false;};
    
  if (touchController.isTouched(500)) {
    TS_Point p = touchController.getPoint();
    Serial.println(String(p.x)+"  "+String(p.y));

    
   
    if (screen == 0 && p.x > 160 && p.x < 220 && p.y > 230 && p.y < 280) {
      // Plus pressed;
      Serial.println("+ pressed");
      DefaultProbeSleep = DefaultProbeSleep + 1;
      if (DefaultProbeSleep == 71) { DefaultProbeSleep = 70; }
    } 
    else if 
    (screen ==0 && p.x > 30 && p.x < 120 && p.y > 230 && p.y < 280) {
      // Minus pressed;
      Serial.println("- pressed");
      DefaultProbeSleep = DefaultProbeSleep - 1;
      if (DefaultProbeSleep == 0) { DefaultProbeSleep = 1; }
    } 

   else if 
    (screen == 2 && p.x > 160 && p.x < 240 && p.y > 0 && p.y < 100) {
      // Counts pressed;
      
      ShowCounts = !ShowCounts;
      Serial.println("Counts button pressed ");
    } 
  else if 
    (screen == 2 && p.x > 80 && p.x < 160 && p.y > 0 && p.y < 100) {
      // Style pressed;
      
      MapStyle = !MapStyle;
      Serial.println("Style button pressed ");
    }   

 else if 
    (screen == 2 && p.x > 0 && p.x < 80 && p.y > 0 && p.y < 100) {
      // Snooze pressed;
      if (!Snooze) Snooze = true;
      if (SnoozeTime < 30) {SnoozeTime = SnoozeTime +5;} else SnoozeTime = SnoozeTime + 30;
      if (SnoozeTime > 5*60) { SnoozeTime = 0; Snooze = false;};
      
      SnoozeStart = millis();
      
      Serial.println("Snooze button pressed ");
    }   
    else 
    {
      screen = (screen + 1) % screenCount;
    }
  }

gfx.fillBuffer(MINI_BLACK);
  if (screen == 0) {
    drawTime();
   /* int remainingTimeBudget = carousel.update();
    if (remainingTimeBudget > 0) {
      // You can do some work here
      // Don't do stuff if you are below your
      // time budget.
      delay(remainingTimeBudget);
    }
    */
   
  } else if (screen == 1) {
    drawAbout();
  } else if (screen == 2)
    drawMap();
  gfx.commit();
 if (SLEEP_INTERVAL_SECS && millis() - timerPress >= SLEEP_INTERVAL_SECS * 1000){ // after 2 minutes go to sleep
    drawProgress(25,"Going to Sleep!");
    delay(1000);
    drawProgress(50,"Going to Sleep!");
    delay(1000);
    drawProgress(75,"Going to Sleep!");
    delay(1000);    
    drawProgress(100,"Going to Sleep!");
    // go to deepsleep for xx minutes or 0 = permanently
    ESP.deepSleep(0,  WAKE_RF_DEFAULT);                       // 0 delay = permanently to sleep
  }  
  
}

void drawProgress(uint8_t percentage, String text) {
  //gfx.fillBuffer(MINI_BLACK);
  //gfx.drawPalettedBitmapFromPgm(23, 30, SquixLogo);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(MINI_WHITE);
  gfx.drawString(120, 80, "Max Botev");
  gfx.setColor(MINI_YELLOW);

  gfx.drawString(120, 146, text);
  gfx.setColor(MINI_WHITE);
  gfx.drawRect(10, 168, 240 - 20, 15);
  gfx.setColor(MINI_RED);
  gfx.fillRect(12, 170, 216 * percentage / 100, 11);

  gfx.commit();
}

void drawMap() {

if ( Probe1 == "online" && P1PIR && (millis()-P1Alive) > 10000 ) P1PIR = 0;  
if ( Probe2 == "online" && P2PIR && (millis()-P2Alive) > 10000 ) P2PIR = 0;  
if ( Probe3 == "online" && P3PIR && (millis()-P3Alive) > 10000 ) P3PIR = 0;  
if ( Probe4 == "online" && P4PIR && (millis()-P4Alive) > 10000 ) P4PIR = 0;  
if ( Probe5 == "online" && P5PIR && (millis()-P5Alive) > 10000 ) P5PIR = 0;  

if (!MapStyle)
{
  gfx.setColor(MINI_WHITE);
  gfx.drawRect(1,270,78,30);
  gfx.drawRect(80,270,80,30);
  gfx.drawRect(160,270,78,30);
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER); 

 if (Snooze) gfx.drawString(120,240,"Snooze time: "+String(SnoozeTime)+"min"); 
  
  if (!ShowCounts) gfx.drawString(40,275,"COUNTS"); else gfx.drawString(40,275,"PROBE ID");
  gfx.drawString(120,275,"STYLE");
  gfx.drawString(200,275,"SNOOZE");
  
  gfx.setColor(MINI_YELLOW);
  gfx.drawCircle(120,120,100);
  gfx.drawCircle(120,120,70);
  gfx.drawCircle(120,120,40);
  gfx.drawRect(10,119,220,2);
  gfx.drawRect(119,10,2,220);





   
  if (Probe1 == "online" && !P1PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(120,50,20); gfx.setColor(MINI_RED); if (!ShowCounts) gfx.drawString(120,45,"1"); else gfx.drawString(120,45,String(P1Counts));}
  else if
     (Probe1 == "online" && P1PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(120,50,20);gfx.setColor(MINI_WHITE); if (!ShowCounts) gfx.drawString(120,45,"1"); else gfx.drawString(120,45,String(P1Counts));}

 if (Probe2 == "online" && !P2PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(55,95,20);gfx.setColor(MINI_RED); if (!ShowCounts) gfx.drawString(55,90,"2"); else gfx.drawString(55,90,String(P2Counts));}
  else if
     (Probe2 == "online" && P2PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(55,95,20);gfx.setColor(MINI_WHITE); if (!ShowCounts) gfx.drawString(55,90,"2"); else gfx.drawString(55,90,String(P2Counts));}  

if (Probe3 == "online" && !P3PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(185,95,20);gfx.setColor(MINI_RED); if (!ShowCounts) gfx.drawString(185,90,"3"); else gfx.drawString(185,90,String(P3Counts));}
  else if
     (Probe3 == "online" && P3PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(185,95,20);gfx.setColor(MINI_WHITE); if (!ShowCounts) gfx.drawString(185,90,"3"); else gfx.drawString(185,90,String(P3Counts));}

if (Probe4 == "online" && !P4PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(70,170,20);gfx.setColor(MINI_RED); if (!ShowCounts) gfx.drawString(70,165,"4"); else gfx.drawString(70,165,String(P4Counts));}
  else if
     (Probe4 == "online" && P4PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(70,170,20);gfx.setColor(MINI_WHITE); if (!ShowCounts) gfx.drawString(70,165,"4"); else gfx.drawString(70,165,String(P4Counts));}

if (Probe5 == "online" && !P5PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(170,170,20);gfx.setColor(MINI_RED); if (!ShowCounts) gfx.drawString(170,165,"5"); else gfx.drawString(170,165,String(P5Counts));}
  else if
     (Probe5 == "online" && P5PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(170,170,20);gfx.setColor(MINI_WHITE); if (!ShowCounts) gfx.drawString(170,165,"5"); else gfx.drawString(170,165,String(P5Counts));}    
}
else
{
  // Another map style
  gfx.setColor(MINI_WHITE);
  gfx.drawRect(130,260,90,30); 
  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER); 
  gfx.drawString(175,265,"STYLE");

   if (Probe1 == "online" && !P1PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(30,30,20); gfx.setColor(MINI_RED);  gfx.drawString(30,25,"1");}
  else if
     (Probe1 == "online" && P1PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(30,30,20);gfx.setColor(MINI_WHITE); gfx.drawString(30,25,"1"); }

 if (Probe2 == "online" && !P2PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(30,80,20);gfx.setColor(MINI_RED);  gfx.drawString(30,75,"2");}
  else if
     (Probe2 == "online" && P2PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(30,80,20);gfx.setColor(MINI_WHITE); gfx.drawString(30,75,"2");}  

if (Probe3 == "online" && !P3PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(30,130,20);gfx.setColor(MINI_RED);  gfx.drawString(30,125,"3");}
  else if
     (Probe3 == "online" && P3PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(30,130,20);gfx.setColor(MINI_WHITE); gfx.drawString(30,125,"3");}

if (Probe4 == "online" && !P4PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(30,180,20);gfx.setColor(MINI_RED);  gfx.drawString(30,175,"4");}
  else if
     (Probe4 == "online" && P4PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(30,180,20);gfx.setColor(MINI_WHITE); gfx.drawString(30,175,"4");}

if (Probe5 == "online" && !P5PIR) {gfx.setColor(MINI_YELLOW); gfx.fillCircle(30,230,20);gfx.setColor(MINI_RED);  gfx.drawString(30,225,"5");}
  else if
     (Probe5 == "online" && P5PIR) {gfx.setColor(MINI_RED);gfx.fillCircle(30,230,20);gfx.setColor(MINI_WHITE);  gfx.drawString(30,225,"5"); }    

 // Draw stats for online and "lost signal" probes
  gfx.setColor(MINI_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  
  
  
  
  if ( Probe1 != "offline") {
    
    
    uint8_t minutes = (millis()-P1Alarm) / 1000 / 60;  
    
    gfx.drawString(60,25,"Counts: "+String(P1Counts)+" Last: "+String(minutes)+" min ago"); 
    
  } 
  
  if ( Probe2 != "offline") {
  
  uint8_t minutes = (millis()-P2Alarm) / 1000 / 60;  
    
    gfx.drawString(60,75,"Counts: "+String(P2Counts)+" Last: "+String(minutes)+" min ago"); 
  }
  if ( Probe3 != "offline") {
  uint8_t minutes = (millis()-P3Alarm) / 1000 / 60;  
    
    gfx.drawString(60,125,"Counts: "+String(P3Counts)+" Last: "+String(minutes)+" min ago"); 
  }
  if ( Probe4 != "offline") {
  uint8_t minutes = (millis()-P4Alarm) / 1000 / 60;  
    
    gfx.drawString(60,175,"Counts: "+String(P4Counts)+" Last: "+String(minutes)+" min ago"); 
  }
  if ( Probe5 != "offline") {
    uint8_t minutes = (millis()-P5Alarm) / 1000 / 60;  
    
    gfx.drawString(60,225,"Counts: "+String(P5Counts)+" Last: "+String(minutes)+" min ago"); 
  }
  }
     
 
}
void drawTime() {
 //gfx.drawString(10,10, "Probe status update time: "+String(DefaultProbeSleep)+" min");
 gfx.setFont(ArialRoundedMTBold_14);
 drawLabelValue(0, "Update time: ", String(DefaultProbeSleep)+"min");
 DrawPlus(50,70);
 DrawMinus(150,70);
 drawLabelValue(7, "Probe 1 status:", Probe1);
 drawLabelValue(9, "Probe 2 status:", Probe2);
 drawLabelValue(11, "Probe 3 status:", Probe3);
 drawLabelValue(13, "Probe 4 status:", Probe4);
 drawLabelValue(15, "Probe 5 status:", Probe5);
 gfx.setFont(ArialMT_Plain_10);
 

 
 if ( Probe1 == "online" ) { drawWifiQuality (7*15+25,P1Wifi); if ( (millis()-P1Alive) > DefaultProbeSleep * 60000 + 20000 ) {Probe1 = "lost signal";};}  
 if ( Probe2 == "online" ) { drawWifiQuality (9*15+25,P2Wifi); if ( (millis()-P2Alive) > DefaultProbeSleep * 60000 + 20000 ) {Probe2 = "lost signal";};} 
 if ( Probe3 == "online" ) { drawWifiQuality (11*15+25,P3Wifi); if ( (millis()-P3Alive) > DefaultProbeSleep * 60000 + 20000 ) {Probe3 = "lost signal";};} 
 if ( Probe4 == "online" ) { drawWifiQuality (13*15+25,P4Wifi); if ( (millis()-P4Alive) > DefaultProbeSleep * 60000 + 20000 ) {Probe4 = "lost signal";};} 
 if ( Probe5 == "online" ) { drawWifiQuality (15*15+25,P5Wifi); if ( (millis()-P5Alive) > DefaultProbeSleep * 60000 + 20000 ) {Probe5 = "lost signa";};} 
  }

void DrawPlus(uint8_t x, uint8_t y)

{

 
 gfx.drawRect(x,y,40,40);
 gfx.drawLine(x+10,y+20,x+30,y+20);
 gfx.drawLine(x+20,y+10,x+20,y+30);

  
}

void DrawMinus(uint8_t x, uint8_t y)

{

 
 gfx.drawRect(x,y,40,40);
 gfx.drawLine(x+10,y+20,x+30,y+20);

  
}
void drawLabelValue(uint8_t line, String label, String value) {
  const uint8_t labelX = 15;
  const uint8_t valueX = 150;
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(labelX, 30 + line * 15, label);
  gfx.setColor(MINI_WHITE);
  gfx.drawString(valueX, 30 + line * 15, value);
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

void drawWifiQuality(int probeYoffset, int quality) {
  //int8_t quality = getWifiQuality();
  gfx.setColor(MINI_WHITE);
  gfx.setTextAlignment(TEXT_ALIGN_RIGHT);  
  gfx.drawString(228, 9+probeYoffset, String(quality) + "%");
  for (int8_t i = 0; i < 4; i++) {
    for (int8_t j = 0; j < 2 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        gfx.setPixel(230 + 2 * i, 18 - j + probeYoffset);
      }
    }
  }
}

void drawAbout() {
  gfx.fillBuffer(MINI_BLACK);
//  gfx.drawPalettedBitmapFromPgm(23, 30, SquixLogo);

  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  gfx.setColor(MINI_WHITE);
  gfx.drawString(120, 80, "HondaMafia");

  gfx.setFont(ArialRoundedMTBold_14);
  gfx.setTextAlignment(TEXT_ALIGN_CENTER);
  drawLabelValue(7, "Heap Mem:", String(ESP.getFreeHeap() / 1024)+"kb");
  drawLabelValue(8, "Flash Mem:", String(ESP.getFlashChipRealSize() / 1024 / 1024) + "MB");
  drawLabelValue(9, "WiFi Strength:", String(WiFi.RSSI()) + "dB");
  drawLabelValue(10, "Chip ID:", String(ESP.getChipId()));
  drawLabelValue(11, "VCC: ", String(ESP.getVcc() / 1024.0) +"V");
  drawLabelValue(12, "CPU Freq.: ", String(ESP.getCpuFreqMHz()) + "MHz");
  char time_str[15];
  const uint32_t millis_in_day = 1000 * 60 * 60 * 24;
  const uint32_t millis_in_hour = 1000 * 60 * 60;
  const uint32_t millis_in_minute = 1000 * 60;
  uint8_t days = millis() / (millis_in_day);
  uint8_t hours = (millis() - (days * millis_in_day)) / millis_in_hour;
  uint8_t minutes = (millis() - (days * millis_in_day) - (hours * millis_in_hour)) / millis_in_minute;
  sprintf(time_str, "%2dd%2dh%2dm", days, hours, minutes);
  drawLabelValue(13, "Uptime: ", time_str);
  gfx.setTextAlignment(TEXT_ALIGN_LEFT);
  gfx.setColor(MINI_YELLOW);
  gfx.drawString(15, 250, "Last Reset: ");
  gfx.setColor(MINI_WHITE);
  gfx.drawStringMaxWidth(15, 265, 240 - 2 * 15, ESP.getResetInfo());
}

void calibrationCallback(int16_t x, int16_t y) {
  gfx.setColor(1);
  gfx.fillCircle(x, y, 10);
}


  

