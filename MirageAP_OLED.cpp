// MirageAP_OLED.cpp
#include "MirageAP_OLED.h"
#include <Wire.h>
#include <ESP8266WiFi.h>

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_SDA 14 // D6
#define OLED_SCL 12 // D5
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306* display;

void initOLED() {
    Wire.begin(OLED_SDA, OLED_SCL);
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println("OLED init failed!");
        while (true);
    }

    // Boot splash
    display->clearDisplay();
    display->setTextSize(1);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    display->println("MirageAP Console");
    display->println("================");
    display->println(" by: aMiscreant ");
    display->display();
}

void showMessage(const String& line1, const String& line2) {
    display->clearDisplay();
    display->setCursor(0,0);
    display->println(line1);
    if(line2.length() > 0) display->println(line2);
    display->display();
}
