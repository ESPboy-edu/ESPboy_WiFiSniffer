// This-->tab == "functions.h"

// Expose Espressif SDK functionality
extern "C" {
#include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}


#include <ESP8266WiFi.h>
#include "./structures.h"
#include "lib/ESPboyTerminalGUI.h"

#define MAX_APS_TRACKED 100
#define MAX_CLIENTS_TRACKED 200

beaconinfo aps_known[MAX_APS_TRACKED];                    // Array to save MACs of known APs
int aps_known_count = 0;                                  // Number of known APs
int nothing_new = 0;
clientinfo clients_known[MAX_CLIENTS_TRACKED];            // Array to save MACs of known CLIENTs
int clients_known_count = 0;                              // Number of known CLIENTs

extern ESPboyTerminalGUI* GUIobj;


String capitaliseString(String str){
  for(uint8_t i = 0; i< str.length(); i++)
    if(str[i] >= 'a' && str[i] <= 'f') str.setCharAt(i,(char)((uint8_t)str[i]-32));
  return (str);
}

int register_beacon(beaconinfo beacon){
  int known = 0;   // Clear known flag
  for (int u = 0; u < aps_known_count; u++)
  {
    if (! memcmp(aps_known[u].bssid, beacon.bssid, ETH_MAC_LEN)) {
      known = 1;
      break;
    }   // AP known => Set known flag
  }
  if (! known)  // AP is NEW, copy MAC to array and return it
  {
    memcpy(&aps_known[aps_known_count], &beacon, sizeof(beacon));
    aps_known_count++;

    if ((unsigned int) aps_known_count >=
      sizeof (aps_known) / sizeof (aps_known[0]) ) {
      GUIobj->printConsole (F("exceeded max aps_known"), TFT_RED, 0, 0);
      aps_known_count = 0;
    }
  }
  return known;
}

int register_client(clientinfo ci){
  int known = 0;   // Clear known flag
  for (int u = 0; u < clients_known_count; u++)
  {
    if (! memcmp(clients_known[u].station, ci.station, ETH_MAC_LEN)) {
      known = 1;
      break;
    }
  }
  if (! known)
  {
    memcpy(&clients_known[clients_known_count], &ci, sizeof(ci));
    clients_known_count++;

    if ((unsigned int) clients_known_count >=
        sizeof (clients_known) / sizeof (clients_known[0]) ) {
        GUIobj->printConsole (F("exceeded max clients_known"), TFT_RED, 0, 0);
        clients_known_count = 0;
    }
  }
  return known;
}

void print_beacon(beaconinfo beacon){
  String toPrint = "";
  if (beacon.err != 0) {
    GUIobj->printConsole (F("Beacon error"), TFT_RED, 0, 0);
  } else {
    toPrint+=(String)beacon.ssid;
    toPrint+=F("                      ");
    toPrint = toPrint.substring(0,20);
    toPrint+=" ";
    if (beacon.channel<10) toPrint+="0";
    toPrint+=beacon.channel;
    toPrint+="  ";
    toPrint+=beacon.rssi;
    GUIobj->printConsole (toPrint, TFT_YELLOW, 0, 0);
    toPrint="";
    for (int i = 0; i < 6; i++) {
      if(beacon.bssid[i]<16) toPrint+="0";
      toPrint+=String(beacon.bssid[i],HEX);}
    GUIobj->printConsole(capitaliseString(toPrint), TFT_BLUE, 0, 0);
  }
}



void print_client(clientinfo ci){
  String toPrint = "", toPrint2 = "";
  int u = 0;
  int known = 0;   // Clear known flag
  if (ci.err != 0) {} 
  else {
    for (u = 0; u < aps_known_count; u++){
      if (!memcmp(aps_known[u].bssid, ci.bssid, ETH_MAC_LEN)) {
        toPrint+=(String)aps_known[u].ssid;
        toPrint+=F("                      ");
        toPrint = toPrint.substring(0,20);
        toPrint+=" ";
        known = 1;     // AP known => Set known flag
        break;
      }
    }

    if (!known)  {
      GUIobj->printConsole (F("Unknown/Malformed packet"), TFT_RED, 0, 0);
      //  for (int i = 0; i < 6; i++) Serial.printf("%02x", ci.bssid[i]);
    } 
    else {
      if(aps_known[u].channel < 10) toPrint += "0";
      toPrint+=(String)aps_known[u].channel;
      toPrint+="  ";
      toPrint+=(String)ci.rssi;

      for (int i = 0; i < 6; i++) {
        if(ci.station[i]<16)toPrint2+="0";
        toPrint2+=String(ci.station[i],HEX);}
      toPrint2+=(" > ");
      for (int i = 0; i < 6; i++) {
        if(ci.ap[i]<16)toPrint2+="0";
        toPrint2+=String(ci.ap[i],HEX);}
    }
  }
 if (toPrint!="")GUIobj->printConsole(capitaliseString(toPrint), TFT_GREEN, 0, 0);
 if (toPrint2!="")GUIobj->printConsole(capitaliseString(toPrint2), TFT_BLUE, 0, 0);
}

void promisc_cb(uint8_t *buf, uint16_t len){
  int i = 0;
  uint16_t seq_n_new = 0;
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
    if (register_beacon(beacon) == 0) {
      print_beacon(beacon);
      nothing_new = 0;
    }
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    //Is data or QOS?
    if ((sniffer->buf[0] == 0x08) || (sniffer->buf[0] == 0x88)) {
      struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
      if (memcmp(ci.bssid, ci.station, ETH_MAC_LEN)) {
        if (register_client(ci) == 0) {
          print_client(ci);
          nothing_new = 0;
        }
      }
    }
  }
}
