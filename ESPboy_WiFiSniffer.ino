/*
ESPboy WiFi sniffer
for www.espboy.com project by RomanS
based on ESP8266 mini-sniff by Ray Burnette http://www.hackster.io/rayburne/projects
*/


#include <ESP8266WiFi.h>
#include "./functions.h"
#include <TFT_eSPI.h>          //to draw at LCD TFT
#include <Adafruit_MCP23017.h> //to control buttons
#include <Adafruit_MCP4725.h>  //to control the LCD display backlit

#include "ESPboyLogo.h"
#include "ESPboyGUI.h"
#include "ESPboyOTA.h"
#include "ESPboy_LED.h"

#define disable 0
#define enable  1

#define PAD_LEFT        0x01
#define PAD_UP          0x02
#define PAD_DOWN        0x04
#define PAD_RIGHT       0x08
#define PAD_ACT         0x10
#define PAD_ESC         0x20
#define PAD_LFT         0x40
#define PAD_RGT         0x80
#define PAD_ANY         0xff

//PINS
#define LEDPIN         D4
#define SOUNDPIN       D3
#define LEDLOCK        9
#define CSTFTPIN       8 //Chip Select pin for LCD (it's on the MCP23017 GPIO expander GPIO8)

#define MCP23017address 0 // actually it's 0x20 but in <Adafruit_MCP23017.h> lib there is (x|0x20) :)
#define MCP4725address  0x60

Adafruit_MCP4725 dac;
Adafruit_MCP23017 mcp;
ESPboyLED myled;
TFT_eSPI tft;
ESPboyGUI* GUIobj = NULL;
ESPboyOTA* OTAobj = NULL;

uint8_t channel = 1;


void setup() {
  Serial.begin(115200); //serial init

//DAC init and backlit off
  dac.begin(MCP4725address);
  delay (100);
  dac.setVoltage(0, false);

//mcp23017 init for buttons, LED LOCK and TFT Chip Select pins
  mcp.begin(MCP23017address);
  delay(100);
  
  for (int i=0;i<8;i++){  
     mcp.pinMode(i, INPUT);
     mcp.pullUp(i, HIGH);}

//LED init
  mcp.pinMode(LEDLOCK, OUTPUT);
  mcp.digitalWrite(LEDLOCK, HIGH); 
  myled.begin();

//sound init and test
  pinMode(SOUNDPIN, OUTPUT);
  tone(SOUNDPIN, 200, 100); 
  delay(100);
  tone(SOUNDPIN, 100, 100);
  delay(100);
  noTone(SOUNDPIN);
  
//LCD TFT init
  mcp.pinMode(CSTFTPIN, OUTPUT);
  mcp.digitalWrite(CSTFTPIN, LOW);
  tft.begin();
  delay(100);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

//draw ESPboylogo  
  tft.drawXBitmap(30, 24, ESPboyLogo, 68, 64, TFT_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(30,102);
  tft.print ("WiFi sniffer");

//LCD backlit fading on
  for (uint16_t bcklt=0; bcklt<4095; bcklt+=20){
    dac.setVoltage(bcklt, false);
    delay(10);}

//clear TFT and backlit on high
  dac.setVoltage(4095, false);
  tft.fillScreen(TFT_BLACK);

//OTA init
  GUIobj = new ESPboyGUI(&tft, &mcp);
  if (GUIobj->getKeys()) OTAobj = new ESPboyOTA(GUIobj);  

//Init WiFi
  GUIobj -> toggleDisplayMode(1); 

  wifi_set_opmode(STATION_MODE);            // Promiscuous works only with station mode
  wifi_set_channel(channel);
  wifi_promiscuous_enable(disable);
  wifi_set_promiscuous_rx_cb(promisc_cb);   // Set up promiscuous callback
  wifi_promiscuous_enable(enable);

  GUIobj->printConsole (F("------------------------------"), TFT_WHITE, 0, 0);
  GUIobj->printConsole (F("SSID                Chan RSSI"), TFT_WHITE, 0, 0);
  GUIobj->printConsole (F("------------------------------"), TFT_WHITE, 0, 0);
}


void loop() {
  channel = 1;
  wifi_set_channel(channel);
  while (true) {
    nothing_new++;                          // Array is not finite, check bounds and adjust if required
    if (nothing_new > 200) {
      nothing_new = 0;
      channel++;
      if (channel == 15) break;             // Only scan channels 1 to 14
      wifi_set_channel(channel);
    }
    delay(1); 
    if (GUIobj->getKeys()) {
      GUIobj->printConsole (F("------------------------------"), TFT_WHITE, 0, 0);
      GUIobj->printConsole (F("SSID                Chan RSSI"), TFT_WHITE, 0, 0);
      GUIobj->printConsole (F("------------------------------"), TFT_WHITE, 0, 0);
      for (int u = 0; u < clients_known_count; u++) {
        if(GUIobj->getKeys())  
          while(GUIobj->getKeys()) delay(100);  
        print_client(clients_known[u]);
        delay(1);}
      for (int u = 0; u < aps_known_count; u++){ 
        if(GUIobj->getKeys())  
          while(GUIobj->getKeys()) delay(100); 
        print_beacon(aps_known[u]);
        delay(1);}
      GUIobj->printConsole (F("------------------------------"), TFT_WHITE, 0, 0);
    }
  }
}
