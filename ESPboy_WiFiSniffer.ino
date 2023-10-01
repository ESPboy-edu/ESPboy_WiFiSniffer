/*
ESPboy WiFi sniffer
for www.espboy.com project by RomanS
based on ESP8266 mini-sniff by Ray Burnette http://www.hackster.io/rayburne/projects
*/


#include <ESP8266WiFi.h>
#include "./functions.h"
#include "lib/ESPboyInit.h"
#include "lib/ESPboyInit.cpp"
#include "lib/ESPboyTerminalGUI.h"
#include "lib/ESPboyTerminalGUI.cpp"
//#include "lib/ESPboyOTA2.h"
//#include "lib/ESPboyOTA2.cpp"


#define disable 0
#define enable  1

ESPboyInit myESPboy;
ESPboyTerminalGUI* GUIobj = NULL;
//ESPboyTerminalGUI *terminalGUIobj = NULL;
//ESPboyOTA2 *OTA2obj = NULL;

uint8_t channel = 1;


void setup() {
  myESPboy.begin("WiFi sniffer");
/*
  //Check OTA2
  if (myESPboy.getKeys()&PAD_ACT || myESPboy.getKeys()&PAD_ESC) { 
     terminalGUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);
     OTA2obj = new ESPboyOTA2(terminalGUIobj);
  }
*/
  GUIobj = new ESPboyTerminalGUI(&myESPboy.tft, &myESPboy.mcp);

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
